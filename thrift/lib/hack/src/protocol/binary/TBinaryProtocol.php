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
 * Do not use this class.
 *
 * This class exists for backwards compatibility, use TBinaryProtocolAccelerated
 * or TBinaryProtocolUnaccelerated
 * @deprecated
 */
class TBinaryProtocol extends TBinaryProtocolAccelerated {
  public function __construct(
    $trans,
    $strict_read = false,
    $strict_write = true,
  ) {
    parent::__construct($trans, $strict_read, $strict_write);
  }
}
