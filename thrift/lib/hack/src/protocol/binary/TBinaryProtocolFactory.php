<?hh // strict

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
 * Binary Protocol Factory
 */
class TBinaryProtocolFactory implements TProtocolFactory {
  protected bool $strictRead = false;
  protected bool $strictWrite = true;

  public function __construct(
    bool $strict_read = false,
    bool $strict_write = true,
  ) {
    $this->strictRead = $strict_read;
    $this->strictWrite = $strict_write;
  }

  public function getProtocol(TTransport $trans): TProtocol {
    return new TBinaryProtocolAccelerated(
      $trans,
      $this->strictRead,
      $this->strictWrite,
    );
  }
}
