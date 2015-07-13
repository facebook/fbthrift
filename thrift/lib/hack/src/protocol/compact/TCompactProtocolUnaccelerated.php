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
 * Old slow unaccelerated protocol.
 */
class TCompactProtocolUnaccelerated extends TCompactProtocolBase {
  public function __construct($trans) {
    parent::__construct($trans);
  }
}
