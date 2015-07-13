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
  public function isReadable(): bool;

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
  public function isWritable(): bool;
}
