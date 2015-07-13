<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol
*/

/**
 * Base class for serializing thrift structs using a TProtocol
 */
<<__ConsistentConstruct>>
abstract class TProtocolSerializer {
  abstract public static function serialize(IThriftStruct $object): string;

  abstract public static function deserialize<T as IThriftStruct>(
    string $str,
    T $object,
  ): T;
}
