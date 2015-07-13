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
 * Old slow unaccelerated protocol.
 */
class TBinaryProtocolUnaccelerated extends TBinaryProtocolBase {
  public function __construct(
    $trans,
    $strict_read = false,
    $strict_write = true,
  ) {
    parent::__construct($trans, $strict_read, $strict_write);
  }
}
