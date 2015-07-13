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
 * Protocol factory creates protocol objects from transports
 */
interface TProtocolFactory {
  /**
   * Build a protocol from the base transport
   *
   * @return TProtocol protocol
   */
  public function getProtocol(TTransport $trans): TProtocol;
}
