<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @package thrift.protocol.simplephpobject
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
class TSimplePHPObjectProtocolKeyedIteratorWrapper implements \HH\Iterator {
  private $key = true;
  private $itr;
  public function __construct($itr) {
    $this->itr = $itr;
  }
  public function key() {
    \HH\invariant_violation("Cannot Access Key");
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
