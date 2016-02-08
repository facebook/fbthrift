<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.compact
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/compact/TCompactProtocolBase.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TBufferedTransport.php';
class TCompactProtocolAccelerated extends TCompactProtocolBase {
  public static function checkVersion($v) {
    return \hacklib_equals($v, 1);
  }
  public function __construct($trans) {
    if (!($trans instanceof IThriftBufferedTransport)) {
      $trans = new TBufferedTransport($trans);
    }
    parent::__construct($trans);
  }
}
