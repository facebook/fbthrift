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
 * @package thrift.protocol.compact
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TMessageType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolSerializer.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/compact/TCompactProtocolAccelerated.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TMemoryBuffer.php';
class TCompactSerializer extends TProtocolSerializer {
  public static function serialize(
    $object,
    $override_version = null,
    $disable_hphp_extension = false
  ) {
    $transport = new TMemoryBuffer();
    $protocol = new TCompactProtocolAccelerated($transport);
    $use_hphp_extension =
      \hacklib_cast_as_boolean(
        function_exists("thrift_protocol_write_compact")
      ) &&
      (!\hacklib_cast_as_boolean($disable_hphp_extension));
    $last_version = null;
    if ($override_version !== null) {
      $protocol->setWriteVersion($override_version);
      if (\hacklib_cast_as_boolean(
            function_exists("thrift_protocol_set_compact_version")
          )) {
        $last_version =
          thrift_protocol_set_compact_version($override_version);
      } else {
        $use_hphp_extension = false;
      }
    }
    if (\hacklib_cast_as_boolean($use_hphp_extension)) {
      thrift_protocol_write_compact(
        $protocol,
        $object->getName(),
        TMessageType::REPLY,
        $object,
        0
      );
      if ($last_version !== null) {
        thrift_protocol_set_compact_version($last_version);
      }
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
    $override_version = null,
    $disable_hphp_extension = false
  ) {
    $transport = new TMemoryBuffer();
    $protocol = new TCompactProtocolAccelerated($transport);
    $use_hphp_extension =
      \hacklib_cast_as_boolean(
        function_exists("thrift_protocol_read_compact")
      ) &&
      (!\hacklib_cast_as_boolean($disable_hphp_extension));
    if ($override_version !== null) {
      $protocol->setWriteVersion($override_version);
      if (!\hacklib_cast_as_boolean(
            function_exists("thrift_protocol_set_compact_version")
          )) {
        $use_hphp_extension = false;
      }
    }
    if (\hacklib_cast_as_boolean($use_hphp_extension)) {
      $protocol->writeMessageBegin("", TMessageType::REPLY, 0);
      $transport->write($str);
      $object = thrift_protocol_read_compact($protocol, get_class($object));
    } else {
      $transport->write($str);
      $object->read($protocol);
    }
    return $object;
  }
}
