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
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportStatus.php';
interface IThriftRemoteConn extends TTransportStatus {
  public function open();
  public function close();
  public function flush();
  public function isOpen();
  public function read($len);
  public function readAll($len);
  public function write($buf);
  public function getRecvTimeout();
  public function setRecvTimeout($timeout);
  public function getHost();
  public function getPort();
}
