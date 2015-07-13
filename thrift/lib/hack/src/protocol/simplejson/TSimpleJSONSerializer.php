<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplejson
*/

/**
 * Utility class for serializing
 * a thrift object using TSimpleJSONProtocol
 */
class TSimpleJSONSerializer extends TProtocolSerializer {
  public static function serialize(IThriftStruct $object): string {
    $transport = new TMemoryBuffer();
    $protocol = new TSimpleJSONProtocol($transport);
    $object->write($protocol);
    return $transport->getBuffer();
  }

  public static function deserialize<T as IThriftStruct>(
    string $str,
    T $object,
  ): T {
    $transport = new TMemoryBuffer($str);
    $protocol = new TSimpleJSONProtocol($transport);
    $object->read($protocol);
    return $object;
  }
}
