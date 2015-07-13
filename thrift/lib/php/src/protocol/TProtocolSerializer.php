<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
abstract class TProtocolSerializer {
  abstract public static function serialize($object);
  abstract public static function deserialize($str, $object);
}
