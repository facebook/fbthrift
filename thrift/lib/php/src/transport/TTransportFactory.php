<?php
// Copyright 2004-present Facebook. All Rights Reserved.

class TTransportFactory {
  /**
   * @static
   * @param TTransport $transport
   * @return TTransport
   */
  public static function getTransport(TTransport $transport) {
    return $transport;
  }
}
