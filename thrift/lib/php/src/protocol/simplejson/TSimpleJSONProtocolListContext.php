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
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocolContext.php';
class TSimpleJSONProtocolListContext extends TSimpleJSONProtocolContext {
  private $first = true;
  public function writeStart() {
    $this->trans->write("[");
    return 1;
  }
  public function writeSeparator() {
    if (\hacklib_cast_as_boolean($this->first)) {
      $this->first = false;
      return 0;
    }
    $this->trans->write(",");
    return 1;
  }
  public function writeEnd() {
    $this->trans->write("]");
    return 1;
  }
  public function readStart() {
    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    if ($c !== "[") {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Expected \"[\", encountered 0x".bin2hex($c)
      );
    }
  }
  public function readSeparator() {
    if (\hacklib_cast_as_boolean($this->first)) {
      $this->first = false;
      return;
    }
    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    if ($c !== ",") {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Expected \",\", encountered 0x".bin2hex($c)
      );
    }
  }
  public function readContextOver() {
    $pos = $this->skipWhitespace(false);
    $c = $this->bufTrans->peek(1, $pos);
    if ((!\hacklib_cast_as_boolean($this->first)) &&
        ($c !== ",") &&
        ($c !== "]")) {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Expected \",\" or \"]\", encountered 0x".
        bin2hex($c)
      );
    }
    return $c === "]";
  }
  public function readEnd() {
    $this->skipWhitespace();
    $c = $this->trans->readAll(1);
    if ($c !== "]") {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Expected \"]\", encountered 0x".bin2hex($c)
      );
    }
  }
  public function escapeNum() {
    return false;
  }
}
