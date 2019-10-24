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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TNullTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocket.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportStatus.php';
class TBufferedTransport extends TTransport
  implements TTransportStatus, IThriftBufferedTransport {
  public function __construct(
    $transport = null,
    $rBufSize = 512,
    $wBufSize = 512
  ) {
    $this->transport_ =
      \hacklib_cast_as_boolean($transport) ?: (new TNullTransport());
    $this->rBufSize_ = $rBufSize;
    $this->wBufSize_ = $wBufSize;
  }
  protected $transport_;
  protected $rBufSize_ = 512;
  protected $wBufSize_ = 512;
  protected $wBuf_ = "";
  protected $rBuf_ = "";
  public function isOpen() {
    return $this->transport_->isOpen();
  }
  public function open() {
    $this->transport_->open();
  }
  public function close() {
    $this->transport_->close();
  }
  public function putBack($data) {
    if (strlen($this->rBuf_) === 0) {
      $this->rBuf_ = $data;
    } else {
      $this->rBuf_ = $data.$this->rBuf_;
    }
  }
  public function getMetaData() {
    if ($this->transport_ instanceof TSocket) {
      return $this->transport_->getMetaData();
    }
    return array();
  }
  public function isReadable() {
    if (strlen($this->rBuf_) > 0) {
      return true;
    }
    if ($this->transport_ instanceof TTransportStatus) {
      return $this->transport_->isReadable();
    }
    return true;
  }
  public function isWritable() {
    if ($this->transport_ instanceof TTransportStatus) {
      return $this->transport_->isWritable();
    }
    return true;
  }
  public function minBytesAvailable() {
    return strlen($this->rBuf_);
  }
  public function readAll($len) {
    $have = strlen($this->rBuf_);
    $data = "";
    if (\hacklib_equals($have, 0)) {
      $data = $this->transport_->readAll($len);
    } else {
      if ($have < $len) {
        $data = $this->rBuf_;
        $this->rBuf_ = "";
        $data .= $this->transport_->readAll($len - $have);
      } else {
        if (\hacklib_equals($have, $len)) {
          $data = $this->rBuf_;
          $this->rBuf_ = "";
        } else {
          if ($have > $len) {
            $data = substr($this->rBuf_, 0, $len);
            $this->rBuf_ = substr($this->rBuf_, $len);
          }
        }
      }
    }
    return $data;
  }
  public function read($len) {
    if (strlen($this->rBuf_) === 0) {
      $this->rBuf_ = $this->transport_->read($this->rBufSize_);
    }
    if (strlen($this->rBuf_) <= $len) {
      $ret = $this->rBuf_;
      $this->rBuf_ = "";
      return $ret;
    }
    $ret = substr($this->rBuf_, 0, $len);
    $this->rBuf_ = substr($this->rBuf_, $len);
    return $ret;
  }
  public function peek($len, $start = 0) {
    $bytes_needed = $len + $start;
    if (\hacklib_equals(strlen($this->rBuf_), 0)) {
      $this->rBuf_ = $this->transport_->readAll($bytes_needed);
    } else {
      if ($bytes_needed > strlen($this->rBuf_)) {
        $this->rBuf_ .=
          $this->transport_->readAll($bytes_needed - strlen($this->rBuf_));
      }
    }
    $ret = substr($this->rBuf_, $start, $len);
    return $ret;
  }
  public function write($buf) {
    $this->wBuf_ .= $buf;
    if (strlen($this->wBuf_) >= $this->wBufSize_) {
      $out = $this->wBuf_;
      $this->wBuf_ = "";
      $this->transport_->write($out);
    }
  }
  public function flush() {
    if (strlen($this->wBuf_) > 0) {
      $this->transport_->write($this->wBuf_);
      $this->wBuf_ = "";
    }
    $this->transport_->flush();
  }
}
