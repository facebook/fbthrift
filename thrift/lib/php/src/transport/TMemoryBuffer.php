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
 * A memory buffer is a tranpsort that simply reads from and writes to an
 * in-memory string buffer. Anytime you call write on it, the data is simply
 * placed into a buffer, and anytime you call read, data is read from that
 * buffer.
 *
 * @package thrift.transport
 */
class TMemoryBuffer extends TTransport {

  private
    $buf_ = '',
    $index_ = 0,
    $length_ = null;

  /**
   * Constructor. Optionally pass an initial value
   * for the buffer.
   */
  public function __construct($buf = '') {
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

  public function write($buf) {
    $this->buf_ .= $buf;
    $this->length_ = null; // reset length
  }

  public function read($len) {
    $available = $this->available();
    if ($available === 0) {
      $buffer_dump = get_site_variable('THRIFT_DUMP_CORRUPTED_BUFFERS') ?
          bin2hex($this->buf_) : 'DISABLED by SITEVAR';
      throw new TTransportException(
          'TMemoryBuffer: Could not read ' .  $len . ' bytes from buffer.' .
          ' Original length is ' . $this->length() .
          ' Current index is ' . $this->index_ .
          ' Buffer content <start>' . $buffer_dump . '<end>',
          TTransportException::UNKNOWN);
    }

    if ($available < $len) {
      $len = $available;
    }
    $ret = substr($this->buf_, $this->index_, $len);
    $this->index_  += $len;
    return $ret;
  }

  public function putBack($buf) {
    if ($this->available() === 0) {
      $this->buf_ = $buf;
    } else {
      $remaining = substr($this->buf_, $this->index_);
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
    $this->buf_ = '';
    $this->index_ = 0;
    $this->length_ = null;
  }
}
