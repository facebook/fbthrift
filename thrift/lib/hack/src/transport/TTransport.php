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
 * Base interface for a transport agent.
 *
 * @package thrift.transport
 */
abstract class TTransport {

  /**
   * Name of the service targeted (for logging).
   */
  protected ?string $service_ = null;

  /**
   * Whether this transport is open.
   *
   * @return boolean true if open
   */
  public abstract function isOpen(): bool;

  /**
   * Open the transport for reading/writing
   *
   * @throws TTransportException if cannot open
   */
  public abstract function open(): void;

  /**
   * Close the transport.
   */
  public abstract function close(): void;

  /**
   * Read some data into the array.
   *
   * @param int    $len How much to read
   * @return string The data that has been read
   * @throws TTransportException if cannot read any more data
   */
  public abstract function read(int $len): string;

  /**
   * Guarantees that the full amount of data is read.
   *
   * @return string The data, of exact length
   * @throws TTransportException if cannot read data
   */
  public function readAll(int $len): string {
    // return $this->read($len);

    $data = '';
    $got = 0;
    while (($got = strlen($data)) < $len) {
      $data .= $this->read($len - $got);
    }
    return $data;
  }

  /**
   * Writes the given data out.
   *
   * @param string $buf  The data to write
   * @throws TTransportException if writing fails
   */
  public abstract function write(string $buf): void;

  /**
   * Flushes any pending data out of a buffer
   *
   * @throws TTransportException if a writing error occurs
   */
  public function flush(): void {}

  /**
   * Flushes any pending data out of a buffer for a oneway call
   *
   * @throws TTransportException if a writing error occurs
   */
  public function onewayFlush(): void {
    // Default to flush()
    $this->flush();
  }
}
