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
class TClientEventHandler {
  public function setClient($client) {}
  public function preSend($fn_name, $args, $sequence_id) {}
  public function postSend($fn_name, $args, $sequence_id) {}
  public function sendError($fn_name, $args, $sequence_id, $ex) {}
  public function preRecv($fn_name, $ex_sequence_id) {}
  public function postRecv($fn_name, $ex_sequence_id, $result) {}
  public function recvException($fn_name, $ex_sequence_id, $exception) {}
  public function recvError($fn_name, $ex_sequence_id, $exception) {}
}
