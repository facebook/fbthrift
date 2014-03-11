<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Copyright (c) 2006- Facebook
 * Distributed under the Thrift Software License
 *
 * See accompanying file LICENSE or visit the Thrift site at:
 * http://developers.facebook.com/thrift/
 *
 * @package thrift.transport
 */

/** Inherits from Socket */
include_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocket.php';

/**
 * Sockets implementation of the TTransport interface that allows connection
 * to a pool of servers.
 *
 * @package thrift.transport
 */
class TSocketPool extends TSocket {

  /**
   * Custom caching functions for environments without AP
   */
  private $fakeAPCFetch_ = null;
  private $fakeAPCStore_ = null;
  private $apcLogger_= null;

  /**
   * Remote servers. List of host:port pairs.
   */
  private $servers_ = array();

  /**
   * How many times to retry each host in connect
   *
   * @var int
   */
  private $numRetries_ = 1;

  /**
   * Retry interval in seconds, how long to not try a host if it has been
   * marked as down.
   *
   * @var int
   */
  private $retryInterval_ = 60;

  /**
   * Max consecutive failures before marking a host down.
   *
   * @var int
   */
  private $maxConsecutiveFailures_ = 1;

  /**
   * Try hosts in order? or Randomized?
   *
   * @var bool
   */
  private $randomize_ = TRUE;

  /**
   * Always try last host, even if marked down?
   *
   * @var bool
   */
  private $alwaysTryLast_ = TRUE;

  /**
   * Always retry the host without wait if there was a transient
   * connection failure (such as Resource temporarily unavailable).
   * If this is set, the wait_and_retry mechanism will ignore the
   * value in $retryInterval_ for transient failures.
   *
   * @var bool
   */
  private $alwaysRetryForTransientFailure_ = false;

  const ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE = 11;

  /**
   * Socket pool constructor
   *
   * @param array  $hosts        List of remote hostnames
   * @param mixed  $ports        Array of remote ports, or a single common port
   * @param bool   $persist      Whether to use a persistent socket
   * @param mixed  $debugHandler Function for error logging
   */
  public function __construct($hosts=array('localhost'),
                              $ports=array(9090),
                              $persist=FALSE,
                              $debugHandler=null) {
    parent::__construct(null, 0, $persist, $debugHandler);

    if (!is_array($ports)) {
      $port = $ports;
      $ports = array();
      foreach ($hosts as $key => $val) {
        $ports[$key] = $port;
      }
    }

    foreach ($hosts as $key => $host) {
      $this->servers_[] = array($host, $ports[$key]);
    }
  }

  /**
   * Add a server to the pool
   *
   * This function does not prevent you from adding a duplicate server entry.
   *
   * @param string $host hostname or IP
   * @param int $port port
   */
  public function addServer($host, $port) {
    $this->servers_[] = array($host, $port);
  }

  /**
   * Sets how many time to keep retrying a host in the connect function.
   *
   * @param int $numRetries
   */
  public function setNumRetries($numRetries) {
    $this->numRetries_ = $numRetries;
  }

  /**
   * Sets how long to wait until retrying a host if it was marked down
   *
   * @param int $numRetries
   */
  public function setRetryInterval($retryInterval) {
    $this->retryInterval_ = $retryInterval;
  }

  /**
   * Sets how many time to keep retrying a host before marking it as down.
   *
   * @param int $numRetries
   */
  public function setMaxConsecutiveFailures($maxConsecutiveFailures) {
    $this->maxConsecutiveFailures_ = $maxConsecutiveFailures;
  }

  /**
   * Turns randomization in connect order on or off.
   *
   * @param bool $randomize
   */
  public function setRandomize($randomize) {
    $this->randomize_ = $randomize;
  }

  /**
   * Whether to always try the last server.
   *
   * @param bool $alwaysTryLast
   */
  public function setAlwaysTryLast($alwaysTryLast) {
    $this->alwaysTryLast_ = $alwaysTryLast;
  }

  /**
   * Whether to always retry the host without wait for
   * transient connection failures
   *
   * @param bool $alwaysRetry
   */
  public function setAlwaysRetryForTransientFailure($alwaysRetry) {
    $this->alwaysRetryForTransientFailure_ = $alwaysRetry;
  }

  /**
   * Connects the socket by iterating through all the servers in the pool
   * and trying to find one that:
   * 1. is not marked down after consecutive failures
   * 2. can really be connected to
   *
   * @return bool  false: any IP in the pool failed to connect before returning
   *               true: no failures
   */
  public function open() {
    // Check if we want order randomization
    if ($this->randomize_) {
      // warning: don't use shuffle here because it leads to uneven
      // load distribution
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

    // Count servers to identify the "last" one
    $numServers = count($this->servers_);
    $has_conn_errors = false;

    $fail_reason = array();  // reasons of conn failures
    for ($i = 0; $i < $numServers; ++$i) {

      // host port is stored as an array
      list($host, $port) = $this->servers_[$i];

      $failtimeKey = TSocketPool::getAPCFailtimeKey($host, $port);
      // Cache miss? Assume it's OK
      $lastFailtime = $this->apcFetch($failtimeKey);
      $this->apcLog("TSocketPool: host $host:$port last fail time: ".
                    $lastFailtime);
      if ($lastFailtime === FALSE) {
        $lastFailtime = 0;
      }

      $retryIntervalPassed = FALSE;

      // Cache hit...make sure enough the retry interval has elapsed
      if ($lastFailtime > 0) {
        $elapsed = time() - $lastFailtime;
        if ($elapsed > $this->retryInterval_) {
          $retryIntervalPassed = TRUE;
          if ($this->debug_) {
            call_user_func($this->debugHandler_,
                           'TSocketPool: retryInterval '.
                           '('.$this->retryInterval_.') '.
                           'has passed for host '.$host.':'.$port);
          }
        }
      }

      // Only connect if not in the middle of a fail interval, OR if this
      // is the LAST server we are trying, just hammer away on it
      $isLastServer = FALSE;
      if ($this->alwaysTryLast_) {
        $isLastServer = ($i == ($numServers - 1));
      }

      if (($lastFailtime === 0) ||
          ($isLastServer) ||
          ($lastFailtime > 0 && $retryIntervalPassed)) {

        // Set underlying TSocket params to this one
        // fsockopen requires IPv6 addresses be bracet enclosed
        $this->host_ = $host;
        $this->port_ = $port;

        // Try up to numRetries_ connections per server
        for ($attempt = 0; $attempt < $this->numRetries_; $attempt++) {
          try {
            // Use the underlying TSocket open function
            parent::open();

            // Only clear the failure counts if required to do so
            if ($lastFailtime > 0) {
              $this->apcStore($failtimeKey, 0);
            }

            // Successful connection, return now
            return !$has_conn_errors;

          } catch (TException $tx) {
            // Connection failed
            // keep the reason for the last try
            $errstr = $this->getErrStr();
            $errno = $this->getErrNo();
            if ($errstr !== null || $errno !== null) {
              $fail_reason[$i] = '(' .$errstr. '[' .$errno. '])';
            } else {
              $fail_reason[$i] = '(?)';
            }
          }
        }

        // For transient errors (like Resource temporarily unavailable),
        // we might want not to cache the failure.
        if ($this->alwaysRetryForTransientFailure_ &&
            $this->isTransientConnectFailure($this->getErrNo())) {
          continue;
        }


        $has_conn_errors = $this->recordFailure($host, $port,
                                 $this->maxConsecutiveFailures_,
                                 $this->retryInterval_,
                                 $this->debug_ ? $this->debugHandler_ : null);
      } else {
        $fail_reason[$i] = '(cached-down)';
      }
    }

    // Holy shit we failed them all. The system is totally ill!
    $error = 'TSocketPool: All hosts in pool are down. ';
    $hosts = array();
    foreach ($this->servers_ as $i => $server) {
      // array(host, port) (reasons, if exist)
      list($host, $port) = $server;
      if (ip_is_valid($host)) {
        $host = IPAddress($host)->forURL();
      }
      $h = $host.':'.$port;
      if (isset($fail_reason[$i])) {
        $h .= $fail_reason[$i];
      }
      $hosts[] = $h;
    }
    $hostlist = implode(',', $hosts);
    $error .= '('.$hostlist.')';
    if ($this->debug_) {
      call_user_func($this->debugHandler_, $error);
    }
    throw new TTransportException($error);
  }

  public static function getAPCFailtimeKey($host, $port) {
    // Check APC cache for a record of this server being down
    return 'thrift_failtime:'.$host.':'.$port.'~';
  }

  /**
   * Record the failure in APC
   * @param string  host  dest IP
   * @param int     port  dest port
   * @param int     max_failures   max consec errors before mark host/port down
   * @param int     down_period    how long to mark the host/port down
   *
   * @return bool  if mark a host/port down
   */
  public function recordFailure($host, $port,
                                $max_failures, $down_period,
                                $log_handler = null) {
    $marked_down = false;
    // Mark failure of this host in the cache
    $failtimeKey = self::getAPCFailtimeKey($host, $port);
    $consecfailsKey = 'thrift_consecfails:'.$host.':'.$port.'~';

    // Ignore cache misses
    $consecfails = $this->apcFetch($consecfailsKey);
    if ($consecfails === false) {
      $consecfails = 0;
    }

    // Increment by one
    $consecfails++;

    // Log and cache this failure
    if ($consecfails >= $max_failures) {
      if ($log_handler) {
        call_user_func($log_handler,
                       'TSocketPool: marking '.$host.':'.$port.
                       ' as down for '.$down_period.' secs '.
                       'after '.$consecfails.' failed attempts.');
      }
      // Store the failure time
      $curr_time = time();
      $this->apcStore($failtimeKey, $curr_time);
      $this->apcLog('TSocketPool: marking '.$host.':'.$port.
                    ' as down for '.$down_period.' secs '.
                    'after '.$consecfails.' failed attempts.('.
                    "max_failures=$max_failures)");
      $marked_down = true;

      // Clear the count of consecutive failures
      $this->apcStore($consecfailsKey, 0);
    } else {
      $this->apcLog("TSocketPool: increased $host:$port consec fails to ".
                    $consecfails);
      $this->apcStore($consecfailsKey, $consecfails);
    }

    return $marked_down;
  }

  /**
   * !! To call this API, you must give apc_is_useful() as the last param
   */
  public function overrideAPCFunctions($fake_apc_fetch, $fake_apc_store,
                                       $logger, $apc_is_useful) {
    if (!$apc_is_useful) {
      $this->fakeAPCFetch_ = $fake_apc_fetch;
      $this->fakeAPCStore_ = $fake_apc_store;
      $this->apcLogger_ = $logger;
    }
  }

  /**
   * Wrapper function around apc_fetch to be able to fetch from fake APC
   * when there is no real APC
   */
  protected function apcFetch($key) {
    if (!$this->fakeAPCFetch_) {
      return apc_fetch($key);
    } else {
      // try fake APC here
      return call_user_func($this->fakeAPCFetch_, $key);
    }
  }

  /**
   * wrapper function around apc_store to be able to store variable to fake APC
   * when there is no real APC
   */
  protected function apcStore($key, $value, $ttl = 0) {
    if (!$this->fakeAPCStore_) {
      return apc_store($key, $value, $ttl);
    } else {
      // try fake APC here
      return call_user_func($this->fakeAPCStore_, $key, $value, $ttl);
    }
  }

  protected function apcLog($message) {
    if ($this->apcLogger_) {
      call_user_func($this->apcLogger_, $message);
    }
  }

  /**
   * Whether a connection failure is a transient failure
   * based on the error-code
   *
   * @param int $error_code
   * @return bool true or false
   */
  private function isTransientConnectFailure($error_code) {

    // todo: add more to this list
    if ($error_code == self::ERROR_RESOURCE_TEMPORARILY_UNAVAILABLE) {
      return true;
    }

    return false;
  }

}
