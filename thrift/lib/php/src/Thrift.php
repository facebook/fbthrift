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

if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__;
}

// This file was split into several separate files
// Now we can just require_once all of them

require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TMessageType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TBase.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftClient.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftProcessor.php';
require_once $GLOBALS['THRIFT_ROOT'].'/IThriftStruct.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TProcessorEventHandler.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TClientEventHandler.php';
require_once $GLOBALS['THRIFT_ROOT'].'/TApplicationException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
