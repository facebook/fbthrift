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
 * Do not use this class.
 *
 * This class exists for backwards compatibility, use
 * TCompactProtocolAccelerated or TCompactProtocolUnaccelerated
 * @deprecated
 */
class TCompactProtocol extends TCompactProtocolAccelerated {
  public function __construct($trans) {
    parent::__construct($trans);
  }
}
