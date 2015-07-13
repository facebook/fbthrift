<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.binary
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolFactory.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocolAccelerated.php';
class TBinaryProtocolFactory implements TProtocolFactory {
  protected $strictRead = false;
  protected $strictWrite = true;
  public function __construct($strict_read = false, $strict_write = true) {
    $this->strictRead = $strict_read;
    $this->strictWrite = $strict_write;
  }
  public function getProtocol($trans) {
    return new TBinaryProtocolAccelerated(
      $trans,
      $this->strictRead,
      $this->strictWrite
    );
  }
}
