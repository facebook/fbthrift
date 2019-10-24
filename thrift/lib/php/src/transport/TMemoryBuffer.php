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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class TMemoryBuffer extends TTransport implements IThriftBufferedTransport {
  private $buf_ = "";
  private $index_ = 0;
  private $length_ = null;
  public function __construct($buf = "") {
    $this->buf_ = (string) $buf;
  }
  public function isOpen() {
    return true;
  }
  public function open() {}
  public function close() {}
  private function length() {
    if ($this->length_ === null) {
      $this->length_ = strlen($this->buf_);
    }
    return $this->length_;
  }
  public function available() {
    return $this->length() - $this->index_;
  }
  public function minBytesAvailable() {
    return $this->available();
  }
  public function write($buf) {
    $this->buf_ .= $buf;
    $this->length_ = null;
  }
  public function read($len) {
    $available = $this->available();
    if ($available === 0) {
      $buffer_dump = bin2hex($this->buf_);
      throw new TTransportException(
        "TMemoryBuffer: Could not read ".
        $len.
        " bytes from buffer.".
        " Original length is ".
        $this->length().
        " Current index is ".
        $this->index_.
        " Buffer content <start>".
        $buffer_dump.
        "<end>",
        TTransportException::UNKNOWN
      );
    }
    if ($available < $len) {
      $len = $available;
    }
    $ret = $this->peek($len);
    $this->index_ += $len;
    return $ret;
  }
  public function peek($len, $start = 0) {
    return
      ($len === 1)
        ? $this->buf_[$this->index_ + $start]
        : substr($this->buf_, $this->index_ + $start, $len);
  }
  public function putBack($buf) {
    if ($this->available() === 0) {
      $this->buf_ = $buf;
    } else {
      $remaining = (string) substr($this->buf_, $this->index_);
      $this->buf_ = $buf.$remaining;
    }
    $this->length_ = null;
    $this->index_ = 0;
  }
  public function getBuffer() {
    if ($this->index_ === 0) {
      return $this->buf_;
    }
    return substr($this->buf_, $this->index_);
  }
  public function resetBuffer() {
    $this->buf_ = "";
    $this->index_ = 0;
    $this->length_ = null;
  }
}
