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
