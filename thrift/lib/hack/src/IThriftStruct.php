<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift
*/

/**
 * Base interface for Thrift structs
 */
interface IThriftStruct {
  public function getName(): string;
  public function read(TProtocol $input): int;
  public function write(TProtocol $input): int;
}
