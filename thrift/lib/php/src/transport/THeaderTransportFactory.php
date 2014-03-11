<?php
// Copyright 2004-present Facebook. All Rights Reserved.

class THeaderTransportFactory {
  /**
   * @static
   * @param TTransport $transport
   * @return TTransport
   */
  public static function getTransport(TTransport $transport) {
    $p = array(THeaderTransport::HEADER_CLIENT_TYPE,
      THeaderTransport::FRAMED_DEPRECATED,
      THeaderTransport::UNFRAMED_DEPRECATED,
    );
    return new THeaderTransport($transport, $p);
  }
}
