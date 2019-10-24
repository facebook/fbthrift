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
 * @package thrift.server
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/server/TServer.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportStatus.php';
class TNonBlockingServer extends TServer {
  protected $clientIdx = 0;
  protected $clients = array();
  public function __construct(
    $processor,
    $serverTransport,
    $transportFactory,
    $protocolFactory
  ) {
    parent::__construct(
      $processor,
      $serverTransport,
      $transportFactory,
      $protocolFactory
    );
  }
  protected function handle($client) {
    $trans = $this->transportFactory->getTransport($client);
    $prot = $this->protocolFactory->getProtocol($trans);
    $this->_clientBegin($prot);
    try {
      if ((!($trans instanceof TTransportStatus)) ||
          \hacklib_cast_as_boolean($trans->isReadable())) {
        $this->processor->process($prot, $prot);
      }
    } catch (Exception $x) {
      $md = $client->getMetaData();
      if (\hacklib_cast_as_boolean($md[\hacklib_id("timed_out")])) {
      } else {
        if (\hacklib_cast_as_boolean($md[\hacklib_id("eof")])) {
          \HH\invariant(
            $trans instanceof TTransport,
            "Need to make Hack happy"
          );
          $trans->close();
          return false;
        } else {
          echo ("Handle caught transport exception: ".$x->getMessage()."\n");
        }
      }
    }
    return true;
  }
  protected function processExistingClients() {
    foreach ($this->clients as $i => $client) {
      if (!\hacklib_cast_as_boolean($this->handle($client))) {
        unset($this->clients[$i]);
      }
    }
  }
  public function serve() {
    $this->serverTransport->listen();
    $this->process();
  }
  public function process() {
    $client = $this->serverTransport->accept(0);
    if (\hacklib_cast_as_boolean($client)) {
      $this->clients[$this->clientIdx++] = $client;
    }
    $this->processExistingClients();
  }
}
