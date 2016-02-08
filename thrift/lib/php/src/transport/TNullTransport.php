<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class TNullTransport extends TTransport {
  public function isOpen() {
    return true;
  }
  public function open() {}
  public function close() {}
  public function read($len) {
    throw new TTransportException("Can't read from TNullTransport.");
  }
  public function write($buf) {}
}
