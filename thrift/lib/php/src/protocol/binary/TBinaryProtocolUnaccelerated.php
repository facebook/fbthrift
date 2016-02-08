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

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocolBase.php';
class TBinaryProtocolUnaccelerated extends TBinaryProtocolBase {
  public function __construct(
    $trans,
    $strict_read = false,
    $strict_write = true
  ) {
    parent::__construct($trans, $strict_read, $strict_write);
  }
}
