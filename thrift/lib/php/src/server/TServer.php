<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * @package thrift.server
 */

include_once $GLOBALS['THRIFT_ROOT'].'/Thrift.php';
include_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
include_once $GLOBALS['THRIFT_ROOT'].'/protocol/TBinaryProtocol.php';

/**
 * Event handler base class.  Override selected methods on this class
 * to implement custom event handling
 */
abstract class TServerEventHandler {

  /**
   * Called before a new client request is handled
   */
  public abstract function clientBegin($prot);
}

/**
 * Base interface for a server.
 *
 *     The attribute serverEventHandler (default: null) receives
 *     callbacks for various events in the server lifecycle.  It should
 *     be set to an instance of TServerEventHandler.
 */
abstract class TServer {
  protected $processor;
  protected $serverTransport;
  protected $transportFactory;
  protected $protocolFactory;
  protected $serverEventHandler;

  public function __construct($processor, $serverTransport,
                              $transportFactory, $protocolFactory) {
    $this->processor              = $processor;
    $this->serverTransport        = $serverTransport;
    $this->transportFactory       = $transportFactory;
    $this->protocolFactory        = $protocolFactory;

    $this->serverEventHandler = null;
  }

  protected function _clientBegin($prot) {
    if ($this->serverEventHandler) {
      $this->serverEventHandler->clientBegin($prot);
    }
  }

  protected function handle($client) {
    $trans = $this->transportFactory->getTransport($client);
    $prot  = $this->protocolFactory->getProtocol($trans);

    $this->_clientBegin($prot);
    try {
      $this->processor->process($prot, $prot);
    } catch (TTransportException $tx) {
      // ignore
    } catch (Exception $x) {
      echo 'Handle caught transport exception: ' . $x->getMessage() . "\n";
    }

    $trans->close();
  }

  public abstract function serve();
}

/**
 * Server that can run in non-blocking mode
 */
class TNonBlockingServer extends TServer {
  protected $clients = array();

  public function __construct($processor, $serverTransport,
                              $transportFactory, $protocolFactory) {
    parent::__construct($processor, $serverTransport,
                        $transportFactory, $protocolFactory);
  }

  /**
   * Because our server is non-blocking, don't close this socket
   * until we need to.
   *
   * @return bool true if we should keep the client alive
   */
  protected function handle($client) {
    $trans = $this->transportFactory->getTransport($client);
    $prot  = $this->protocolFactory->getProtocol($trans);

    $this->_clientBegin($prot);
    try {
      // First check the transport is readable to avoid
      // blocking on read
      if ($trans->isReadable()) {
        $this->processor->process($prot, $prot);
      }
    } catch (Exception $x) {
      $md = $client->getMetaData();
      if ($md['timed_out']) {
        // keep waiting for the client to send more requests
      } else if ($md['eof']) {
        $trans->close();
        return false;
      } else {
        echo 'Handle caught transport exception: ' . $x->getMessage() . "\n";
      }
    }
    return true;
  }

  protected function processExistingClients() {
    foreach ($this->clients as $i => $client) {
      if (!$this->handle($client)) {
        // remove the client from our list of open clients if
        // our handler reports that the client is no longer alive
        unset($this->clients[$i]);
      }
    }
  }

  /*
   * This method should be called repeately on idle to listen and
   * process an request. If there is no pending request, it will
   * return;
   */
  public function serve() {
    $this->serverTransport->listen();
    $this->process();
  }

  public function process() {
    // 0 timeout is non-blocking
    $client = $this->serverTransport->accept(0);
    if ($client) {
      array_unshift($this->clients, $client);
    }

    $this->processExistingClients();
  }
}
