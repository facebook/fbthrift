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
 * @package thrift
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftClient.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TClientEventHandler.php';
abstract class ThriftClientBase implements IThriftClient {
  protected $input_;
  protected $output_;
  protected $asyncHandler_;
  protected $eventHandler_;
  protected $seqid_ = 0;
  final public static function factory() {
    return array(
      get_called_class(),
      function($input, $output) {
        return new static($input, $output);
      }
    );
  }
  public function __construct($input, $output = null) {
    $this->input_ = $input;
    $this->output_ = \hacklib_cast_as_boolean($output) ?: $input;
    $this->asyncHandler_ = new TClientAsyncHandler();
    $this->eventHandler_ = new TClientEventHandler();
  }
  public function setAsyncHandler($async_handler) {
    $this->asyncHandler_ = $async_handler;
    return $this;
  }
  public function getAsyncHandler() {
    return $this->asyncHandler_;
  }
  public function setEventHandler($event_handler) {
    $this->eventHandler_ = $event_handler;
    return $this;
  }
  public function getEventHandler() {
    return $this->eventHandler_;
  }
  protected function getNextSequenceID() {
    $currentseqid = $this->seqid_;
    if ($this->seqid_ >= 0x7fffffff) {
      $this->seqid_ = 0;
    } else {
      $this->seqid_++;
    }
    return $currentseqid;
  }
}
