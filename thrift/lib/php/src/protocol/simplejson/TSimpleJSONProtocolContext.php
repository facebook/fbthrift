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

require_once ($GLOBALS["HACKLIB_ROOT"]);
class TSimpleJSONProtocolContext {
  protected $trans;
  protected $bufTrans;
  final public function __construct($trans, $bufTrans) {
    $this->trans = $trans;
    $this->bufTrans = $bufTrans;
  }
  public function writeStart() {
    return 0;
  }
  public function writeSeparator() {
    return 0;
  }
  public function writeEnd() {
    return 0;
  }
  public function readStart() {}
  public function readSeparator() {}
  public function readContextOver() {
    return true;
  }
  public function readEnd() {}
  public function escapeNum() {
    return false;
  }
  protected function skipWhitespace($skip = true) {
    $count = 0;
    $reading = true;
    while (\hacklib_cast_as_boolean($reading)) {
      $byte = $this->bufTrans->peek(1, $count);
      switch ($byte) {
        case " ":
        case "\t":
        case "\n":
        case "\r":
          $count++;
          break;
        default:
          $reading = false;
          break;
      }
    }
    if (\hacklib_cast_as_boolean($skip)) {
      $this->trans->readAll($count);
    }
    return $count;
  }
}
