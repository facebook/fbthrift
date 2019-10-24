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
class TProcessorEventHandler {
  public function getHandlerContext($fn_name) {
    return null;
  }
  public function preRead($handler_context, $fn_name, $args) {}
  public function postRead($handler_context, $fn_name, $args) {}
  public function preExec($handler_context, $fn_name, $args) {}
  public function postExec($handler_context, $fn_name, $result) {}
  public function preWrite($handler_context, $fn_name, $result) {}
  public function postWrite($handler_context, $fn_name, $result) {}
  public function handlerException($handler_context, $fn_name, $exception) {}
  public function handlerError($handler_context, $fn_name, $exception) {}
}
