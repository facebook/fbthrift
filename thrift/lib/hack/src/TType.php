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
 * Data types that can be sent via Thrift
 */
enum TType: int as int {
  STOP = 0;
  VOID = 1;
  BOOL = 2;
  BYTE = 3;
  DOUBLE = 4;
  I16 = 6;
  I32 = 8;
  I64 = 10;
  STRING = 11;
  STRUCT = 12;
  MAP = 13;
  SET = 14;
  LST = 15; // N.B. cannot use LIST keyword in PHP!
  UTF8 = 16;
  UTF16 = 17;
  FLOAT = 19;
}
