<?hh // strict

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
 * Compact Protocol Factory
 */
class TCompactProtocolFactory implements TProtocolFactory {

  public function __construct() {}

  public function getProtocol(TTransport $trans): TProtocol {
    return new TCompactProtocolAccelerated($trans);
  }
}
