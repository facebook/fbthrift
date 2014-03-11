<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * @package thrift.protocol
 */


/**
 * Utility class for serializing and deserializing
 * a thrift object using TCompactProtocol.
 */
class TCompactSerializer {

  public static function serialize(
    $object,
    $override_version = null,
    bool $disable_hphp_extension = false,
  ) {
    $transport = new TMemoryBuffer();
    $protocol = new TCompactProtocolAccelerated($transport);

    $use_hphp_extension =
      function_exists('thrift_protocol_write_compact') &&
      !$disable_hphp_extension;

    $last_version = null;
    if ($override_version !== null) {
      $protocol->setWriteVersion($override_version);
      if (function_exists('thrift_protocol_set_compact_version')) {
        $last_version = thrift_protocol_set_compact_version($override_version);
      } else {
        $use_hphp_extension = false;
      }
    }

    if ($use_hphp_extension) {
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

  public static function deserialize($string_object,
                                     $class_name,
                                     $override_version = null,
                                     bool $disable_hphp_extension = false) {
    $transport = new TMemoryBuffer();
    $protocol = new TCompactProtocolAccelerated($transport);

    $use_hphp_extension =
      function_exists('thrift_protocol_read_compact') &&
      !$disable_hphp_extension;

    if ($override_version !== null) {
      $protocol->setWriteVersion($override_version);
      if (!function_exists('thrift_protocol_set_compact_version')) {
        $use_hphp_extension = false;
      }
    }

    if ($use_hphp_extension) {
      $protocol->writeMessageBegin('', TMessageType::REPLY, 0);
      $transport->write($string_object);
      $object = thrift_protocol_read_compact($protocol, $class_name);
    } else {
      $transport->write($string_object);
      $object = new $class_name();
      $object->read($protocol);
    }
    $remaining = $transport->available();
    if ($remaining) {
      FBLogger('TCompactSerializer')->warn(
        "Deserialization didn't consume the whole input string (%d bytes left)".
        "Are you sure this was serialized as a '%s'?",
        $remaining,
        $class_name);
    }
    return $object;
  }
}
