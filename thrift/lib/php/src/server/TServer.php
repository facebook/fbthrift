<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.server
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
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
      echo ('Handle caught transport exception: '.$x->getMessage()."\n");
      return false;
    }
    $trans->close();
    return true;
  }
  public abstract function serve();
}
