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
 * TTransport subclasses that implement InstrumentedTTransport and use
 * InstrumentedTTransportTrait have a simple set of counters available.
 */
trait InstrumentedTTransportTrait {
  private int $bytesWritten = 0;
  private int $bytesRead = 0;

  public function getBytesWritten(): int {
    return $this->bytesWritten;
  }

  public function getBytesRead(): int {
    return $this->bytesRead;
  }

  public function resetBytesWritten(): void {
    $this->bytesWritten = 0;
  }

  public function resetBytesRead(): void {
    $this->bytesRead = 0;
  }

  protected function onWrite(int $bytes_written): void {
    $this->bytesWritten += $bytes_written;
  }

  protected function onRead(int $bytes_read): void {
    $this->bytesRead += $bytes_read;
  }
}
