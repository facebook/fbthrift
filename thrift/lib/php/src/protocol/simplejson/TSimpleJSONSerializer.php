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
 * @package thrift.protocol.simplejson
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolSerializer.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TMemoryBuffer.php';
class TSimpleJSONSerializer extends TProtocolSerializer {
  public static function serialize($object) {
    $transport = new TMemoryBuffer();
    $protocol = new TSimpleJSONProtocol($transport);
    $object->write($protocol);
    return $transport->getBuffer();
  }
  public static function deserialize($str, $object) {
    $transport = new TMemoryBuffer($str);
    $protocol = new TSimpleJSONProtocol($transport);
    $object->read($protocol);
    return $object;
  }
}
