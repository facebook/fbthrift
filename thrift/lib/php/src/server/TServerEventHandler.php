<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.server
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
abstract class TServerEventHandler {
  public abstract function clientBegin($prot);
}
