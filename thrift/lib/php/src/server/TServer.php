<?php
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
      while (true) {
        $this->processor->process($prot, $prot);
      }
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
 * Simple single-threaded server that just pumps around one transport.
 */
class TSimpleServer extends TServer {

  public function __construct($processor, $serverTransport,
                              $transportFactory, $protocolFactory) {
    parent::__construct($processor, $serverTransport,
                        $transportFactory, $protocolFactory);
  }

  public function serve() {
    $this->serverTransport->listen();
    while (true) {
      $client = $this->serverTransport->accept();
      $this->handle($client);
    }
  }
}

/**
 * A Thrift server that forks a new process for each request
 *
 * Note that this has different semantics from the threading server.
 * Specifically, updates to shared variables are not shared.
 *
 * This code is heavily inspired by SocketServer.ForkingMixIn in the
 * Python stdlib.
 */
class TForkingServer extends TServer {

  public function __construct($processor, $serverTransport,
                              $transportFactory, $protocolFactory) {
    parent::__construct($processor, $serverTransport,
                        $transportFactory, $protocolFactory);
    $this->children = array();
  }

  private function tryClose($file) {
    try {
      $file->close();
    } catch (IOError $e) {
      echo 'Close caught IOError: ' . $e->getMessage() . "\n";
    }
  }

  public function serve() {
    $this->serverTransport->listen();
    while (true) {
      $client = $this->serverTransport->accept();
      try {
        $trans = $this->transportFactory->getTransport($client);
        $prot  = $this->protocolFactory->getProtocol($trans);

        $this->_clientBegin($prot);

        $pid = pcntl_fork();

        if ($pid) { // parent
          // add before collect, otherwise you race w/ waitpid
          $this->children[$pid] = 1;
          $this->_collectChildren();

          // Parent must close socket or the connection may not get
          // closed promptly
          $this->tryClose($trans);
        } else {
          $ecode = 0;
          try {
            try {
              while (true) {
                $this->processor->process($prot, $prot);
              }
            } catch (TTransportException $tx) {
              // ignore
            } catch (Exception $e) {
              echo 'Serve caught Exception: ' . $e->getMessage() . "\n";
              $ecode = 1;
            }
          } catch (Exception $e) {
            $this->tryClose($trans);
          }

          exit($ecode);
        }
      } catch (TTransportException $tx) {
        // ignore
      } catch (Exception $e) {
        echo 'Serve caught outer Exception: ' . $e->getMessage() . "\n";
      }
    }
  }

  private function _collectChildren() {
    while (count($this->children)) {
      try {
        $pid = pcntl_waitpid(0, $status, WNOHANG);
      } catch (Exception $e) {
        echo 'Waitpid caught Exception: ' . $e->getMessage() . "\n";
        $pid = null;
      }

      if ($pid) {
        unset($this->children[$pid]);
      } else {
        break;
      }
    }
  }
}
