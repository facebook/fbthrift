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

interface IThriftRemoteConn extends TTransportStatus {
  public function open(): void;
  public function close(): void;
  public function flush(): void;
  public function isOpen(): bool;

  public function read(int $len): string;
  public function readAll(int $len): string;
  public function write(string $buf): void;

  public function getRecvTimeout(): int;
  public function setRecvTimeout(int $timeout): void;

  public function getHost(): string;
  public function getPort(): int;
}
