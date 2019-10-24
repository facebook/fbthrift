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
require_once $GLOBALS['THRIFT_ROOT'].'/TClientEventHandler.php';
final class TClientMultiEventHandler extends TClientEventHandler {
  private $handlers;
  public function __construct() {
    $this->handlers = \HH\Map::hacklib_new(array(), array());
  }
  public function addHandler($key, $handler) {
    $this->handlers[$key] = $handler;
    return $this;
  }
  public function getHandler($key) {
    return $this->handlers[$key];
  }
  public function removeHandler($key) {
    $handler = $this->getHandler($key);
    $this->handlers->remove($key);
    return $handler;
  }
  public function preSend($fn_name, $args, $sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->preSend($fn_name, $args, $sequence_id);
    }
  }
  public function postSend($fn_name, $args, $sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->postSend($fn_name, $args, $sequence_id);
    }
  }
  public function sendError($fn_name, $args, $sequence_id, $ex) {
    foreach ($this->handlers as $handler) {
      $handler->sendError($fn_name, $args, $sequence_id, $ex);
    }
  }
  public function preRecv($fn_name, $ex_sequence_id) {
    foreach ($this->handlers as $handler) {
      $handler->preRecv($fn_name, $ex_sequence_id);
    }
  }
  public function postRecv($fn_name, $ex_sequence_id, $result) {
    foreach ($this->handlers as $handler) {
      $handler->postRecv($fn_name, $ex_sequence_id, $result);
    }
  }
  public function recvException($fn_name, $ex_sequence_id, $exception) {
    foreach ($this->handlers as $handler) {
      $handler->recvException($fn_name, $ex_sequence_id, $exception);
    }
  }
  public function recvError($fn_name, $ex_sequence_id, $exception) {
    foreach ($this->handlers as $handler) {
      $handler->recvError($fn_name, $ex_sequence_id, $exception);
    }
  }
}
