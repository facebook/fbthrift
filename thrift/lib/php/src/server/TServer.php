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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
abstract class TServer {
  protected $processor;
  protected $serverTransport;
  protected $transportFactory;
  protected $protocolFactory;
  protected $serverEventHandler;
  public function __construct(
    $processor,
    $serverTransport,
    $transportFactory,
    $protocolFactory
  ) {
    $this->processor = $processor;
    $this->serverTransport = $serverTransport;
    $this->transportFactory = $transportFactory;
    $this->protocolFactory = $protocolFactory;
    $this->serverEventHandler = null;
  }
  protected function _clientBegin($prot) {
    if (\hacklib_cast_as_boolean($this->serverEventHandler)) {
      $this->serverEventHandler->clientBegin($prot);
    }
  }
  protected function handle($client) {
    $trans = $this->transportFactory->getTransport($client);
    $prot = $this->protocolFactory->getProtocol($trans);
    $this->_clientBegin($prot);
    try {
      $this->processor->process($prot, $prot);
    } catch (TTransportException $tx) {
      return false;
    } catch (Exception $x) {
      echo ("Handle caught transport exception: ".$x->getMessage()."\n");
      return false;
    }
    $trans->close();
    return true;
  }
  public abstract function serve();
}
