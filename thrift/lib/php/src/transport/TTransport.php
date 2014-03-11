<?php
// Copyright 2004-present Facebook. All Rights Reserved.


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
 * Transport exceptions
 *
 * Types deriving from TTransport exception may not be able to
 * translate their custom error into the set of error code
 * supported by TTransportException. For that the $shortMessage
 * facility is provided.
 *
 * @param mixed  $message Message (string) or type-spec (array)
 * @param mixed  $code Code (integer) or values (array)
 * @param string $shortMessage (string)
 */
class TTransportException extends TException {

  const UNKNOWN = 0;
  const NOT_OPEN = 1;
  const ALREADY_OPEN = 2;
  const TIMED_OUT = 3;
  const END_OF_FILE = 4;
  const INVALID_CLIENT = 5;
  const INVALID_FRAME_SIZE = 6;
  const INVALID_TRANSFORM = 7;
  const COULD_NOT_CONNECT = 8;
  const COULD_NOT_READ = 9;
  const COULD_NOT_WRITE = 10;

  protected string $shortMessage;

  public function __construct(
    $message=null,
    $code=0,
    string $short_message='') {
    $this->shortMessage = $short_message;
    parent::__construct($message, $code);
  }

  public function getShortMessage(): string {
    return $this->shortMessage;
  }
}

/**
 * Base interface for a transport agent.
 *
 * @package thrift.transport
 */
abstract class TTransport {

  /**
   * Whether this transport is open.
   *
   * @return boolean true if open
   */
  public abstract function isOpen();

  /**
   * Open the transport for reading/writing
   *
   * @throws TTransportException if cannot open
   */
  public abstract function open();

  /**
   * Close the transport.
   */
  public abstract function close();

  /**
   * Read some data into the array.
   *
   * @param int    $len How much to read
   * @return string The data that has been read
   * @throws TTransportException if cannot read any more data
   */
  public abstract function read($len);

  /**
   * Guarantees that the full amount of data is read.
   *
   * @return string The data, of exact length
   * @throws TTransportException if cannot read data
   */
  public function readAll($len) {
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
  public abstract function write($buf);

  /**
   * Flushes any pending data out of a buffer
   *
   * @throws TTransportException if a writing error occurs
   */
  public function flush() {}

  /**
   * Flushes any pending data out of a buffer for a oneway call
   *
   * @throws TTransportException if a writing error occurs
   */
  public function onewayFlush() {
    // Default to flush()
    $this->flush();
  }
}

/**
 *  Determine (as best as possible) whether the transport can preform
 *  non-blocking read and write operations.
 */
interface TTransportStatus {

  /**
   *  Test whether the transport is ready for a non-blocking read. It is
   *  possible, though, that a transport is ready for a partial read, but a full
   *  read will block.
   *
   *  In the case a transport becomes unavailable for reading due to an error
   *  an exception should be raised. Any timeout logic should also raise an
   *  exception.
   *
   *  @return bool True if a non-blocking read can be preformed on the
   *               transport.
   */
  public function isReadable();

  /**
   *  Test whether the transport is ready for a non-blocking write.
   *
   *  In the case a transport becomes unavailable for writing due to an error
   *  an exception should be raised. Any timeout logic should also raise an
   *  exception.
   *
   *  @return bool True if a non-blocking write can be preformed on the
   *               transport.
   */
  public function isWritable();
}

/**
 * TTransport subclasses that implement InstrumentedTTransport and use
 * InstrumentedTTransportTrait have a simple set of counters available.
 */
trait InstrumentedTTransportTrait {
  private $bytesWritten = 0;
  private $bytesRead = 0;

  public function getBytesWritten() {
    return $this->bytesWritten;
  }

  public function getBytesRead() {
    return $this->bytesRead;
  }

  public function resetBytesWritten() {
    $this->bytesWritten = 0;
  }

  public function resetBytesRead() {
    $this->bytesRead = 0;
  }

  protected function onWrite(int $bytes_written) {
    $this->bytesWritten += $bytes_written;
  }

  protected function onRead(int $bytes_read) {
    $this->bytesRead += $bytes_read;
  }
}

interface InstrumentedTTransport {
  public function getBytesWritten();
  public function getBytesRead();
  public function resetBytesWritten();
  public function resetBytesRead();
}
