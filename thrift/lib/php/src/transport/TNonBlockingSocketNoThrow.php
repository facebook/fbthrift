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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TNonBlockingSocket.php';
class TNonBlockingSocketNoThrow extends TNonBlockingSocket {
  public function open() {
    try {
      parent::open();
    } catch (Exception $e) {
      $op_in_progress =
        (strpos($e->getMessage(), "socket_connect error") !== false) &&
        (strpos($e->getMessage(), "[115]") !== false);
      if (!\hacklib_cast_as_boolean($op_in_progress)) {
        throw $e;
      }
    }
  }
}
