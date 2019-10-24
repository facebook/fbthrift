<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @package thrift.transport
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocket.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class TSocketPool extends TSocket {
  private $fakeAPCFetch_ = null;
  private $fakeAPCStore_ = null;
  private $apcLogger_ = null;
  private $servers_ = array();
  private $numRetries_ = 1;
  private $retryInterval_ = 60;
  private $maxConsecutiveFailures_ = 1;
  private $randomize_ = true;
  private $alwaysTryLast_ = true;
  private $alwaysRetryForTransientFailure_ = false;
  const ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE = 11;
  public function __construct(
    $hosts = array("localhost"),
    $ports = array(9090),
    $persist = false,
    $debugHandler = null
  ) {
    parent::__construct("", 0, $persist, $debugHandler);
    foreach ($hosts as $key => $host) {
      $this->servers_[] = array((string) $host, (int) $ports[$key]);
    }
  }
  public function addServer($host, $port) {
    $this->servers_[] = array($host, $port);
  }
  public function setNumRetries($numRetries) {
    $this->numRetries_ = $numRetries;
  }
  public function setRetryInterval($retryInterval) {
    $this->retryInterval_ = $retryInterval;
  }
  public function setMaxConsecutiveFailures($maxConsecutiveFailures) {
    $this->maxConsecutiveFailures_ = $maxConsecutiveFailures;
  }
  public function setRandomize($randomize) {
    $this->randomize_ = $randomize;
  }
  public function setAlwaysTryLast($alwaysTryLast) {
    $this->alwaysTryLast_ = $alwaysTryLast;
  }
  public function setAlwaysRetryForTransientFailure($alwaysRetry) {
    $this->alwaysRetryForTransientFailure_ = $alwaysRetry;
  }
  public function open() {
    if (\hacklib_cast_as_boolean($this->randomize_)) {
      $n = count($this->servers_);
      $s = $this->servers_;
      for ($i = 1; $i < $n; $i++) {
        $j = mt_rand(0, $i);
        $tmp = $s[$i];
        $s[$i] = $s[$j];
        $s[$j] = $tmp;
      }
      $this->servers_ = $s;
    }
    $numServers = count($this->servers_);
    $has_conn_errors = false;
    $fail_reason = array();
    for ($i = 0; $i < $numServers; ++$i) {
      list($host, $port) = $this->servers_[$i];
      $failtimeKey = TSocketPool::getAPCFailtimeKey($host, $port);
      $lastFailtime = (int) $this->apcFetch($failtimeKey);
      $this->apcLog(
        "TSocketPool: host ".
        $host.
        ":".
        $port.
        " last fail time: ".
        $lastFailtime
      );
      $retryIntervalPassed = false;
      if ($lastFailtime > 0) {
        $elapsed = time() - $lastFailtime;
        if ($elapsed > $this->retryInterval_) {
          $retryIntervalPassed = true;
          if (\hacklib_cast_as_boolean($this->debug_) &&
              ($this->debugHandler_ !== null)) {
            $dh = $this->debugHandler_;
            $dh(
              "TSocketPool: retryInterval ".
              "(".
              $this->retryInterval_.
              ") ".
              "has passed for host ".
              $host.
              ":".
              $port
            );
          }
        }
      }
      $isLastServer = false;
      if (\hacklib_cast_as_boolean($this->alwaysTryLast_)) {
        $isLastServer = \hacklib_equals($i, $numServers - 1);
      }
      if (($lastFailtime === 0) ||
          \hacklib_cast_as_boolean($isLastServer) ||
          (($lastFailtime > 0) &&
           \hacklib_cast_as_boolean($retryIntervalPassed))) {
        $this->host_ = $host;
        $this->port_ = $port;
        for ($attempt = 0; $attempt < $this->numRetries_; $attempt++) {
          try {
            parent::open();
            if ($lastFailtime > 0) {
              $this->apcStore($failtimeKey, 0);
            }
            return;
          } catch (TException $tx) {
            $errstr = $this->getErrStr();
            $errno = $this->getErrNo();
            if (($errstr !== null) || ($errno !== null)) {
              $fail_reason[$i] = "(".$errstr."[".$errno."])";
            } else {
              $fail_reason[$i] = "(?)";
            }
          }
        }
        if (\hacklib_cast_as_boolean(
              $this->alwaysRetryForTransientFailure_
            ) &&
            \hacklib_cast_as_boolean(
              $this->isTransientConnectFailure($this->getErrNo())
            )) {
          continue;
        }
        $dh =
          \hacklib_cast_as_boolean($this->debug_)
            ? $this->debugHandler_
            : null;
        $has_conn_errors = $this->recordFailure(
          $host,
          $port,
          $this->maxConsecutiveFailures_,
          $this->retryInterval_,
          $dh
        );
      } else {
        $fail_reason[$i] = "(cached-down)";
      }
    }
    $error = "TSocketPool: All hosts in pool are down. ";
    $hosts = array();
    foreach ($this->servers_ as $i => $server) {
      list($host, $port) = $server;
      $h = $host.":".$port;
      if (\hacklib_cast_as_boolean(array_key_exists($i, $fail_reason))) {
        $h .= (string) $fail_reason[$i];
      }
      $hosts[] = $h;
    }
    $hostlist = implode(",", $hosts);
    $error .= "(".$hostlist.")";
    if (\hacklib_cast_as_boolean($this->debug_) &&
        ($this->debugHandler_ !== null)) {
      $dh = $this->debugHandler_;
      $dh($error);
    }
    throw new TTransportException($error);
  }
  public static function getAPCFailtimeKey($host, $port) {
    return "thrift_failtime:".$host.":".$port."~";
  }
  public function recordFailure(
    $host,
    $port,
    $max_failures,
    $down_period,
    $log_handler = null
  ) {
    $marked_down = false;
    $failtimeKey = self::getAPCFailtimeKey($host, $port);
    $consecfailsKey = "thrift_consecfails:".$host.":".$port."~";
    $consecfails = $this->apcFetch($consecfailsKey);
    if ($consecfails === false) {
      $consecfails = 0;
    }
    $consecfails = (int) $consecfails;
    $consecfails++;
    if ($consecfails >= $max_failures) {
      if ($log_handler !== null) {
        $log_handler(
          "TSocketPool: marking ".
          $host.
          ":".
          $port.
          " as down for ".
          $down_period.
          " secs ".
          "after ".
          $consecfails.
          " failed attempts."
        );
      }
      $curr_time = time();
      $this->apcStore($failtimeKey, $curr_time);
      $this->apcLog(
        "TSocketPool: marking ".
        $host.
        ":".
        $port.
        " as down for ".
        $down_period.
        " secs ".
        "after ".
        $consecfails.
        " failed attempts.(".
        "max_failures=".
        $max_failures.
        ")"
      );
      $marked_down = true;
      $this->apcStore($consecfailsKey, 0);
    } else {
      $this->apcLog(
        "TSocketPool: increased ".
        $host.
        ":".
        $port.
        " consec fails to ".
        $consecfails
      );
      $this->apcStore($consecfailsKey, $consecfails);
    }
    return $marked_down;
  }
  public function overrideAPCFunctions(
    $fake_apc_fetch,
    $fake_apc_store,
    $logger,
    $apc_is_useful
  ) {
    if (!\hacklib_cast_as_boolean($apc_is_useful)) {
      $this->fakeAPCFetch_ = $fake_apc_fetch;
      $this->fakeAPCStore_ = $fake_apc_store;
      $this->apcLogger_ = $logger;
    }
  }
  protected function apcFetch($key) {
    if ($this->fakeAPCFetch_ === null) {
      return apc_fetch($key);
    } else {
      $ff = $this->fakeAPCFetch_;
      return $ff($key);
    }
  }
  protected function apcStore($key, $value, $ttl = 0) {
    if ($this->fakeAPCStore_ === null) {
      return apc_store($key, $value, $ttl);
    } else {
      $fs = $this->fakeAPCStore_;
      return $fs($key, $value, $ttl);
    }
  }
  protected function apcLog($message) {
    if ($this->apcLogger_ !== null) {
      $logger = $this->apcLogger_;
      $logger($message);
    }
  }
  private function isTransientConnectFailure($error_code) {
    if ($error_code === null) {
      return false;
    }
    switch ($error_code) {
      case self::ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE:
        return true;
      default:
        return false;
    }
  }
}
