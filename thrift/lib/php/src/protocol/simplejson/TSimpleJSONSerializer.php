<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplejson
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolSerializer.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TMemoryBuffer.php';
class TSimpleJSONSerializer extends TProtocolSerializer {
  public static function serialize($object) {
    $transport = new TMemoryBuffer();
    $protocol = new TSimpleJSONProtocol($transport);
    $object->write($protocol);
    return $transport->getBuffer();
  }
  public static function deserialize($str, $object) {
    $transport = new TMemoryBuffer($str);
    $protocol = new TSimpleJSONProtocol($transport);
    $object->read($protocol);
    return $object;
  }
}
