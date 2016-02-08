<?hh

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

interface IThriftBufferedTransport {
  public function peek(int $len, int $start = 0);
  public function putBack(string $data): void;
  public function minBytesAvailable(): int;
}
