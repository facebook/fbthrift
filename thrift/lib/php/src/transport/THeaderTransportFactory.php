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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/THeaderTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportFactory.php';
class THeaderTransportFactory extends TTransportFactory {
  public function getTransport($transport) {
    $p = new \HH\Vector(
      array(
        THeaderTransport::HEADER_CLIENT_TYPE,
        THeaderTransport::FRAMED_DEPRECATED,
        THeaderTransport::UNFRAMED_DEPRECATED
      )
    );
    return new THeaderTransport($transport, $p);
  }
}
