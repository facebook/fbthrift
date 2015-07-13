<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplephpobject
*/

require_once ($GLOBALS['HACKLIB_ROOT']);
class TSimplePHPObjectProtocolKeyedIteratorWrapper implements \HH\Iterator {
  private $key = true;
  private $itr;
  public function __construct($itr) {
    $this->itr = $itr;
  }
  public function key() {
    \HH\invariant_violation('Cannot Access Key');
  }
  public function current() {
    if (\hacklib_cast_as_boolean($this->key)) { // UNSAFE
      return $this->itr->key();
    } else {
      return $this->itr->current();
    }
  }
  public function next() {
    $this->key = !\hacklib_cast_as_boolean($this->key);
    if (\hacklib_cast_as_boolean($this->key)) {
      $this->itr->next();
    }
  }
  public function rewind() {
    $this->key = !\hacklib_cast_as_boolean($this->key);
    if (!\hacklib_cast_as_boolean($this->key)) {
      $this->itr->rewind();
    }
  }
  public function valid() {
    return $this->itr->valid();
  }
}
