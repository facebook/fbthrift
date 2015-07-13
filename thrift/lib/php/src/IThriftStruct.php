<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
interface IThriftStruct {
  public function getName();
  public function read($input);
  public function write($input);
}
