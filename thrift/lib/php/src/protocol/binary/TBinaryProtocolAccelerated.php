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
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocolBase.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TBufferedTransport.php';
class TBinaryProtocolAccelerated extends TBinaryProtocolBase {
  public function __construct(
    $trans,
    $strict_read = false,
    $strict_write = true
  ) {
    if (!($trans instanceof IThriftBufferedTransport)) {
      $trans = new TBufferedTransport($trans);
    }
    parent::__construct($trans, $strict_read, $strict_write);
  }
  public function isStrictRead() {
    return $this->strictRead_;
  }
  public function isStrictWrite() {
    return $this->strictWrite_;
  }
}
