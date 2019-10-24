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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
class TPhpStream extends TTransport {
  const MODE_R = 1;
  const MODE_W = 2;
  private $inStream_ = null;
  private $outStream_ = null;
  private $read_ = false;
  private $write_ = false;
  private $maxReadChunkSize_ = null;
  public function __construct($mode) {
    $this->read_ = (bool) ($mode & self::MODE_R);
    $this->write_ = (bool) ($mode & self::MODE_W);
  }
  public function setMaxReadChunkSize($maxReadChunkSize) {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }
  public function open() {
    if (\hacklib_cast_as_boolean($this->read_)) {
      $this->inStream_ = fopen(self::inStreamName(), "r");
      if (!\hacklib_cast_as_boolean(is_resource($this->inStream_))) {
        throw new TException("TPhpStream: Could not open php://input");
      }
    }
    if (\hacklib_cast_as_boolean($this->write_)) {
      $this->outStream_ = fopen("php://output", "w");
      if (!\hacklib_cast_as_boolean(is_resource($this->outStream_))) {
        throw new TException("TPhpStream: Could not open php://output");
      }
    }
  }
  public function close() {
    if (\hacklib_cast_as_boolean($this->read_)) {
      fclose($this->inStream_);
      $this->inStream_ = null;
    }
    if (\hacklib_cast_as_boolean($this->write_)) {
      fclose($this->outStream_);
      $this->outStream_ = null;
    }
  }
  public function isOpen() {
    return
      ((!\hacklib_cast_as_boolean($this->read_)) ||
       \hacklib_cast_as_boolean(is_resource($this->inStream_))) &&
      ((!\hacklib_cast_as_boolean($this->write_)) ||
       \hacklib_cast_as_boolean(is_resource($this->outStream_)));
  }
  public function read($len) {
    if ($this->maxReadChunkSize_ !== null) {
      $len = min($len, $this->maxReadChunkSize_);
    }
    $data = fread($this->inStream_, $len);
    if (($data === false) || ($data === "")) {
      throw new TException("TPhpStream: Could not read ".$len." bytes");
    }
    return $data;
  }
  public function write($buf) {
    while (strlen($buf) > 0) {
      $got = fwrite($this->outStream_, $buf);
      if (($got === 0) || ($got === false)) {
        throw new TException(
          "TPhpStream: Could not write ".((string) strlen($buf))." bytes"
        );
      }
      $buf = substr($buf, $got);
    }
  }
  public function flush() {
    fflush($this->outStream_);
  }
  private static function inStreamName() {
    if (\hacklib_equals(php_sapi_name(), "cli")) {
      return "php://stdin";
    }
    return "php://input";
  }
}
