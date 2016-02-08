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
trait InstrumentedTTransportTrait {
  private $bytesWritten = 0;
  private $bytesRead = 0;
  public function getBytesWritten() {
    return $this->bytesWritten;
  }
  public function getBytesRead() {
    return $this->bytesRead;
  }
  public function resetBytesWritten() {
    $this->bytesWritten = 0;
  }
  public function resetBytesRead() {
    $this->bytesRead = 0;
  }
  protected function onWrite($bytes_written) {
    $this->bytesWritten += $bytes_written;
  }
  protected function onRead($bytes_read) {
    $this->bytesRead += $bytes_read;
  }
  protected static final function hacklib_initialize_statics_InstrumentedTTransportTrait(
  ) {}
}
