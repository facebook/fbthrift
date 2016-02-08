<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TNullTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportStatus.php';
class TFramedTransport extends TTransport
  implements TTransportStatus, IThriftBufferedTransport {
  protected $transport_;
  protected $rBuf_ = "";
  protected $rIndex_ = 0;
  protected $wBuf_ = "";
  private $read_;
  private $write_;
  public function __construct($transport = null, $read = true, $write = true) {
    $this->transport_ =
      \hacklib_cast_as_boolean($transport) ?: (new TNullTransport());
    $this->read_ = $read;
    $this->write_ = $write;
  }
  public function isOpen() {
    return $this->transport_->isOpen();
  }
  public function open() {
    $this->transport_->open();
  }
  public function close() {
    $this->transport_->close();
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
    return strlen($this->rBuf_) - $this->rIndex_;
  }
  public function read($len) {
    if (!\hacklib_cast_as_boolean($this->read_)) {
      return $this->transport_->read($len);
    }
    if (strlen($this->rBuf_) === 0) {
      $this->readFrame();
    }
    $out = substr($this->rBuf_, $this->rIndex_, $len);
    $this->rIndex_ += $len;
    if (strlen($this->rBuf_) <= $this->rIndex_) {
      $this->rBuf_ = "";
      $this->rIndex_ = 0;
    }
    return $out;
  }
  public function peek($len, $start = 0) {
    if (!\hacklib_cast_as_boolean($this->read_)) {
      return "";
    }
    if (strlen($this->rBuf_) === 0) {
      $this->readFrame();
    }
    $out = substr($this->rBuf_, $this->rIndex_ + $start, $len);
    return $out;
  }
  public function putBack($data) {
    if (strlen($this->rBuf_) === 0) {
      $this->rBuf_ = $data;
    } else {
      $this->rBuf_ = $data.((string) substr($this->rBuf_, $this->rIndex_));
    }
    $this->rIndex_ = 0;
  }
  private function readFrame() {
    $buf = $this->transport_->readAll(4);
    $val = unpack("N", $buf);
    $sz = $val[1];
    $this->rBuf_ = $this->transport_->readAll($sz);
    $this->rIndex_ = 0;
  }
  public function write($buf, $len = null) {
    if (!\hacklib_cast_as_boolean($this->write_)) {
      $this->transport_->write($buf);
      return;
    }
    if (($len !== null) && ($len < strlen($buf))) {
      $buf = substr($buf, 0, $len);
    }
    $this->wBuf_ .= $buf;
  }
  public function flush() {
    if ((!\hacklib_cast_as_boolean($this->write_)) ||
        \hacklib_equals(strlen($this->wBuf_), 0)) {
      $this->transport_->flush();
      return;
    }
    $out = (string) pack("N", strlen($this->wBuf_));
    $out .= $this->wBuf_;
    $this->wBuf_ = "";
    $this->transport_->write($out);
    $this->transport_->flush();
  }
}
