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
abstract class TTransport {
  protected $service_ = null;
  public abstract function isOpen();
  public abstract function open();
  public abstract function close();
  public abstract function read($len);
  public function readAll($len) {
    $data = "";
    $got = 0;
    while (($got = strlen($data)) < $len) {
      $data .= $this->read($len - $got);
    }
    return $data;
  }
  public abstract function write($buf);
  public function flush() {}
  public function onewayFlush() {
    $this->flush();
  }
}
