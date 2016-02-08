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
interface IThriftBufferedTransport {
  public function peek($len, $start = 0);
  public function putBack($data);
  public function minBytesAvailable();
}
