<?php
// Copyright 2004-present Facebook. All Rights Reserved.


/**
 * Copyright (c) 2006- Facebook
 * Distributed under the Thrift Software License
 *
 * See accompanying file LICENSE or visit the Thrift site at:
 * http://developers.facebook.com/thrift/
 *
 * @package thrift.transport
 */

/**
 * Framed transport. Writes and reads data in chunks that are stamped with
 * their length.
 *
 * @package thrift.transport
 */
class TFramedTransport extends TTransport {

  /**
   * Underlying transport object.
   *
   * @var TTransport
   */
  protected $transport_;

  /**
   * Buffer for read data.
   *
   * @var string
   */
  protected $rBuf_;

  /**
   * Position in rBuf_ to read the next char
   *
   * @var int
   */
  protected $rIndex_;

  /**
   * Buffer for queued output data
   *
   * @var string
   */
  protected $wBuf_;

  /**
   * Whether to frame reads
   *
   * @var bool
   */
  private $read_;

  /**
   * Whether to frame writes
   *
   * @var bool
   */
  private $write_;

  /**
   * Constructor.
   *
   * @param TTransport $transport Underlying transport
   */
  public function __construct($transport=null, $read=true, $write=true) {
    $this->transport_ = $transport;
    $this->read_ = $read;
    $this->write_ = $write;
  }

  public function isOpen() {
    return $this->transport_->isOpen();
  }

  public function open() {
    return $this->transport_->open();
  }

  public function close() {
    $this->transport_->close();
  }

  public function isReadable() {
    if (strlen($this->rBuf_) > 0) {
      return true;
    }

    return $this->transport_->isReadable();
  }

  /**
   * Reads from the buffer. When more data is required reads another entire
   * chunk and serves future reads out of that.
   *
   * @param int $len How much data
   */
  public function read($len) {
    if (!$this->read_) {
      return $this->transport_->read($len);
    }

    if (strlen($this->rBuf_) === 0) {
      $this->readFrame();
    }


    // Return substr
    $out = substr($this->rBuf_, $this->rIndex_, $len);
    $this->rIndex_ += $len;

    if (strlen($this->rBuf_) <= $this->rIndex_) {
      $this->rBuf_ = null;
      $this->rIndex_ = 0;
    }
    return $out;
  }

  /**
   * Peek some bytes in the frame without removing the bytes from the buffer
   *
   * @param int $len   length to peek
   * @param int $start the start position of the returned string
   */
  public function peek($len, $start = 0) {
    if (!$this->read_) {
      return false;
    }

    if (strlen($this->rBuf_) === 0) {
      $this->readFrame();
    }

    // Return substr
    $out = substr($this->rBuf_, $this->rIndex_ + $start, $len);

    return $out;
  }

  /**
   * Put previously read data back into the buffer
   *
   * @param string $data data to return
   */
  public function putBack($data) {
    if (strlen($this->rBuf_) === 0) {
      $this->rBuf_ = $data;
    } else {
      $this->rBuf_ = ($data . substr($this->rBuf_, $this->rIndex_));
    }
    $this->rIndex_ = 0;
  }

  /**
   * Reads a chunk of data into the internal read buffer.
   */
  private function readFrame() {
    $buf = $this->transport_->readAll(4);
    $val = unpack('N', $buf);
    $sz = $val[1];

    $this->rBuf_ = $this->transport_->readAll($sz);
    $this->rIndex_ = 0;
  }

  /**
   * Writes some data to the pending output buffer.
   *
   * @param string $buf The data
   * @param int    $len Limit of bytes to write
   */
  public function write($buf, $len=null) {
    if (!$this->write_) {
      return $this->transport_->write($buf, $len);
    }

    if ($len !== null && $len < strlen($buf)) {
      $buf = substr($buf, 0, $len);
    }
    $this->wBuf_ .= $buf;
  }

  /**
   * Writes the output buffer to the stream in the format of a 4-byte length
   * followed by the actual data.
   */
  public function flush() {
    if (!$this->write_ || strlen($this->wBuf_) == 0) {
      return $this->transport_->flush();
    }

    $out = pack('N', strlen($this->wBuf_));
    $out .= $this->wBuf_;

    // Note that we clear the internal wBuf_ prior to the underlying write
    // to ensure we're in a sane state (i.e. internal buffer cleaned)
    // if the underlying write throws up an exception
    $this->wBuf_ = '';
    $this->transport_->write($out);
    $this->transport_->flush();
  }

}
