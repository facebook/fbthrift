<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.binary
*/

/**
 * Utility class for serializing and deserializing
 * a thrift object using TBinaryProtocolAccelerated.
 */
class TBinarySerializer extends TProtocolSerializer {

  // NOTE(rmarin): Because thrift_protocol_write_binary
  // adds a begin message prefix, you cannot specify
  // a transport in which to serialize an object. It has to
  // be a string. Otherwise we will break the compatibility with
  // normal deserialization.
  public static function serialize(
    IThriftStruct $object,
    bool $disable_hphp_extension = false,
  ): string {
    $transport = new TMemoryBuffer();
    $protocol = new TBinaryProtocolAccelerated($transport);
    if (function_exists('thrift_protocol_write_binary') &&
        !$disable_hphp_extension) {
      thrift_protocol_write_binary(
        $protocol,
        $object->getName(),
        TMessageType::REPLY,
        $object,
        0,
        $protocol->isStrictWrite(),
      );

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
    bool $disable_hphp_extension = false,
  ): T {
    $transport = new TMemoryBuffer();
    $protocol = new TBinaryProtocolAccelerated($transport);
    if (function_exists('thrift_protocol_read_binary') &&
        !$disable_hphp_extension) {
      $protocol->writeMessageBegin('', TMessageType::REPLY, 0);
      $transport->write($str);
      $object = thrift_protocol_read_binary(
        $protocol,
        get_class($object),
        $protocol->isStrictRead(),
      );
    } else {
      $transport->write($str);
      $object->read($protocol);
    }
    return $object;
  }
}
