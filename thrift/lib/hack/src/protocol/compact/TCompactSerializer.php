<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.compact
*/

/**
 * Utility class for serializing and deserializing
 * a thrift object using TCompactProtocol.
 */
class TCompactSerializer extends TProtocolSerializer {

  public static function serialize(
    IThriftStruct $object,
    ?int $override_version = null,
    bool $disable_hphp_extension = false,
  ): string {
    $transport = new TMemoryBuffer();
    $protocol = new TCompactProtocolAccelerated($transport);

    $use_hphp_extension =
      function_exists('thrift_protocol_write_compact') &&
      !$disable_hphp_extension;

    $last_version = null;
    if ($override_version !== null) {
      $protocol->setWriteVersion($override_version);
      if (function_exists('thrift_protocol_set_compact_version')) {
        $last_version =
          thrift_protocol_set_compact_version($override_version);
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
        0,
      );
      if ($last_version !== null) {
        thrift_protocol_set_compact_version($last_version);
      }
      $unused_name = $unused_type = $unused_seqid = null;
      $protocol->readMessageBegin(
        $unused_name,
        $unused_type,
        $unused_seqid,
      );
    } else {
      $object->write($protocol);
    }
    return $transport->getBuffer();
  }

  public static function deserialize<T as IThriftStruct>(
    string $str,
    T $object,
    ?int $override_version = null,
    bool $disable_hphp_extension = false,
  ) {
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
      $transport->write($str);
      $object = thrift_protocol_read_compact($protocol, get_class($object));
    } else {
      $transport->write($str);
      /* HH_FIXME[2060] Trust me, I know what I'm doing */
      $object->read($protocol);
    }
    return $object;
  }
}
