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

if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}

// This file was split into several separate files
// Now we can just require_once all of them

require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TMessageType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TBase.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftClient.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftProcessor.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftStruct.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TProcessorEventHandler.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TClientEventHandler.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TApplicationException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
