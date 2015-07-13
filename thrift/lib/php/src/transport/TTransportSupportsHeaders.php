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

require_once ($GLOBALS['HACKLIB_ROOT']);
interface TTransportSupportsHeaders {
  public function setHeader($str_key, $str_value);
  public function setPersistentHeader($str_key, $str_value);
  public function getWriteHeaders();
  public function getPersistentWriteHeaders();
  public function getHeaders();
  public function clearHeaders();
  public function clearPersistentHeaders();
}
