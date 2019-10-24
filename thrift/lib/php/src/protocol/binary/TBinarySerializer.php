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
 * @package thrift.protocol.binary
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TMessageType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolSerializer.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/binary/TBinaryProtocolAccelerated.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TMemoryBuffer.php';
class TBinarySerializer extends TProtocolSerializer {
  public static function serialize($object, $disable_hphp_extension = false) {
    $transport = new TMemoryBuffer();
    $protocol = new TBinaryProtocolAccelerated($transport);
    if (\hacklib_cast_as_boolean(
          function_exists("thrift_protocol_write_binary")
        ) &&
        (!\hacklib_cast_as_boolean($disable_hphp_extension))) {
      thrift_protocol_write_binary(
        $protocol,
        $object->getName(),
        TMessageType::REPLY,
        $object,
        0,
        $protocol->isStrictWrite()
      );
      $unused_name = $unused_type = $unused_seqid = null;
      $protocol->readMessageBegin($unused_name, $unused_type, $unused_seqid);
    } else {
      $object->write($protocol);
    }
    return $transport->getBuffer();
  }
  public static function deserialize(
    $str,
    $object,
    $disable_hphp_extension = false
  ) {
    $transport = new TMemoryBuffer();
    $protocol = new TBinaryProtocolAccelerated($transport);
    if (\hacklib_cast_as_boolean(
          function_exists("thrift_protocol_read_binary")
        ) &&
        (!\hacklib_cast_as_boolean($disable_hphp_extension))) {
      $protocol->writeMessageBegin("", TMessageType::REPLY, 0);
      $transport->write($str);
      $object = thrift_protocol_read_binary(
        $protocol,
        get_class($object),
        $protocol->isStrictRead()
      );
    } else {
      $transport->write($str);
      $object->read($protocol);
    }
    return $object;
  }
}
