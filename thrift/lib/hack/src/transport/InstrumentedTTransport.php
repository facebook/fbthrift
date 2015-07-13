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

interface InstrumentedTTransport {
  public function getBytesWritten(): int;
  public function getBytesRead(): int;
  public function resetBytesWritten(): void;
  public function resetBytesRead(): void;
}
