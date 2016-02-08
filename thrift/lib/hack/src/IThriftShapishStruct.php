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
 * Interface for Thrift structs that support conversions to shapes
 */
interface IThriftShapishStruct extends IThriftStruct {
  abstract const type TShape;

  public function __toShape(): this::TShape;
  public static function __fromShape(this::TShape $shape): this;

  public static function __jsonArrayToShape(
    array<arraykey, mixed> $json_data,
  ): ?this::TShape;
}
