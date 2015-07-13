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
 * Message types for RPC
 */
abstract class TMessageType {
  const CALL = 1;
  const REPLY = 2;
  const EXCEPTION = 3;
}
