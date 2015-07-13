<?hh // strict

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

class TTransportFactory {
  /**
   * @static
   * @param TTransport $transport
   * @return TTransport
   */
  public function getTransport(TTransport $transport): TTransport {
    return $transport;
  }
}
