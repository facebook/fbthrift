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

/**
 * Transport that only accepts writes and ignores them.
 * This is useful for measuring the serialized size of structures.
 *
 * @package thrift.transport
 */
class TNullTransport extends TTransport {

  public function isOpen(): bool {
    return true;
  }

  public function open(): void {}

  public function close(): void {}

  public function read(int $len): string {
    throw new TTransportException("Can't read from TNullTransport.");
  }

  public function write(string $buf): void {}

}
