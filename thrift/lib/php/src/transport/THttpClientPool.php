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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/THttpClient.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class THttpClientPool extends THttpClient {
  protected $servers_ = array();
  private $randomize_ = true;
  private $numTries_ = 1;
  public function __construct(
    $hosts,
    $ports,
    $uri = "",
    $scheme = "http",
    $debugHandler = null
  ) {
    parent::__construct("", 0, $uri, $scheme, $debugHandler);
    foreach ($hosts as $key => $host) {
      $this->servers_[] = array($host, $ports[$key]);
    }
  }
  public function addServer($host, $port) {
    $this->servers_[] = array($host, $port);
  }
  public function setRandomize($randomize) {
    $this->randomize_ = $randomize;
  }
  public function setNumTries($numTries) {
    $this->numTries_ = $numTries;
  }
  public function flush() {
    if (\hacklib_cast_as_boolean($this->randomize_)) {
      shuffle($this->servers_);
    }
    foreach ($this->servers_ as $server) {
      $this->host_ = $server[0];
      $this->port_ = $server[1];
      $j = $this->numTries_;
      while ($j > 0) {
        try {
          parent::flush();
          return;
        } catch (TTransportException $e) {
          if (\hacklib_cast_as_boolean($this->debug_)) {
            call_user_func($this->debugHandler_, $e->getMessage());
          }
          --$j;
        }
      }
    }
    $this->host_ = "";
    $this->port_ = 0;
    $error =
      "THttpClientPool: Could not connect to any of the servers ".
      "in the pool";
    throw new TTransportException($error, TTransportException::NOT_OPEN);
  }
}
