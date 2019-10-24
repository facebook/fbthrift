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
 * @package thrift.protocol.binary
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolFactory.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocolAccelerated.php';
class TBinaryProtocolFactory implements TProtocolFactory {
  protected $strictRead = false;
  protected $strictWrite = true;
  public function __construct($strict_read = false, $strict_write = true) {
    $this->strictRead = $strict_read;
    $this->strictWrite = $strict_write;
  }
  public function getProtocol($trans) {
    return new TBinaryProtocolAccelerated(
      $trans,
      $this->strictRead,
      $this->strictWrite
    );
  }
}
