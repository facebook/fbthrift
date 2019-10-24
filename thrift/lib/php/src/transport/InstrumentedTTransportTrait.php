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
