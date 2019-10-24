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
 * @package thrift.transport
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TException.php';
class TTransportException extends TException {
  const UNKNOWN = 0;
  const NOT_OPEN = 1;
  const ALREADY_OPEN = 2;
  const TIMED_OUT = 3;
  const END_OF_FILE = 4;
  const INVALID_CLIENT = 5;
  const INVALID_FRAME_SIZE = 6;
  const INVALID_TRANSFORM = 7;
  const COULD_NOT_CONNECT = 8;
  const COULD_NOT_READ = 9;
  const COULD_NOT_WRITE = 10;
  protected $shortMessage;
  public function __construct(
    $message = null,
    $code = 0,
    $short_message = ""
  ) {
    $this->shortMessage = $short_message;
    parent::__construct($message, $code);
  }
  public function getShortMessage() {
    return $this->shortMessage;
  }
}
