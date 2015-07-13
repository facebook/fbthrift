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

class THeaderTransportFactory extends TTransportFactory {
  /**
   * @static
   * @param TTransport $transport
   * @return TTransport
   */
  public function getTransport(TTransport $transport): THeaderTransport {
    $p = Vector {
      THeaderTransport::HEADER_CLIENT_TYPE,
      THeaderTransport::FRAMED_DEPRECATED,
      THeaderTransport::UNFRAMED_DEPRECATED,
    };
    return new THeaderTransport($transport, $p);
  }
}
