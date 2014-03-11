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
 * NonBlocking implementation of TSocket. Does internal
 * buffering on sends and recvs. An external socket
 * select loop can drive io. (See ClientSet.php).
 *
 * @package thrift.transport
 */
class TNonBlockingSocket extends TTransport {

  /**
   * Handle to PHP socket
   *
   * @var resource
   */
  private $handle_ = null;

  /**
   * Remote hostname
   *
   * @var string
   */
  protected $host_ = 'localhost';

  /**
   * Remote port
   *
   * @var int
   */
  protected $port_ = '9090';

  protected $ipV6_ = false;

  /**
   * The write buffer.
   *
   *  @var string
   */
  protected $wBuf_ = '';

  /**
   * The read buffer and pos.
   *
   *  @var string
   *  @var int
   */
  protected $rBuf_ = '';
  protected $rBufPos_ = 0;

  /**
   * Debug on?
   *
   * @var bool
   */
  protected $debug_ = false;

  /**
   * Debug handler
   *
   * @var mixed
   */
  protected $debugHandler_ = null;

  /**
   * Socket recv buffer capacity.
   * @var int or null.
   */
  private $sockRecvCapacity_ = null;

  /**
   * Socket constructor
   *
   * @param string $host         Remote hostname
   * @param int    $port         Remote port
   * @param string $debugHandler Function to call for error logging
   */

  public function __construct($host='localhost',
                              $port=9090,
                              $debugHandler=null) {
    $this->host_ = $host;
    $this->port_ = $port;
    $this->ipV6_ = strlen(@inet_pton($host)) == 16;

    $this->debugHandler_ = $debugHandler ?: 'error_log';
  }

  /**
   * Get the host that this socket is connected to
   *
   * @return string host
   */
  public function getHost() {
    return $this->host_;
  }

  /**
   * Get the remote port that this socket is connected to
   *
   * @return int port
   */
  public function getPort() {
    return $this->port_;
  }

  /**
   * Get the socket for this connection. So we can select on it.
   *
   * @return socket resource
   */
  public function getSocket() {
    return $this->handle_;
  }

  /**
   * Tests whether this is open
   *
   * @return bool true if the socket is open
   */
  public function isOpen() {
    return is_resource($this->handle_);
  }

  /**
   * Set debugging
   *
   * @param bool on/off
   */
  public function setDebug($debug) {
    $this->debug_ = $debug;
  }

  /**
   * Connects the socket.
   */
  public function open() {
    if ($this->ipV6_) {
      $this->handle_ = socket_create(AF_INET6, SOCK_STREAM, SOL_TCP);
    } else {
      $this->handle_ = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    }

    if ($this->handle_ === FALSE) {
      $error = 'TNonBlockingSocket: Could not create socket';
      throw new TTransportException($error);
    }

    if (!socket_set_nonblock($this->handle_)) {
      $error = 'TNonBlockingSocket: Could not set nonblocking.';
      throw new TTransportException($error);
    }

    if (!@socket_connect($this->handle_,
                         $this->host_,
                         $this->port_)) {

      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error = 'TNonBlockingSocket: socket_connect error ('.$errstr.'['.$errno.
               '])';
      if ($errno != 115) {
        if ($this->debug_) {
          call_user_func($this->debugHandler_, $error);
        }
      }
      throw new TTransportException($error);
    }

    $wBuf_ = '';
    $rBuf_ = '';
    $rBufPos_ = 0;

    $this->sockRecvCapacity_ = socket_get_option($this->handle_,
                                                 SOL_SOCKET,
                                                 SO_RCVBUF);
    if ($this->sockRecvCapacity_ == false) {
      $this->sockRecvCapacity_ = null;
    }
  }

  /**
   * Closes the socket.
   */
  public function close() {
    if ($this->handle_ !== FALSE) {
      @socket_close($this->handle_);
    }
    $this->handle_ = null;
  }

  /**
   * Do a nonblocking read. If all of $len is not there, throw an exception.
   * Save unread data to the recv buffer so subsequent reads can retrieve it.
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  public function readAll($len) {
    // We may already have data for this read in the buffer
    $ret = substr($this->rBuf_, $this->rBufPos_);
    // set the buffer to all but this read
    $this->rBuf_ = substr($this->rBuf_, 0, $this->rBufPos_);

    if ($len <= strlen($ret)) {
      $this->rBuf_ .= $ret;
      $this->rBufPos_ += $len;

      return substr($ret, 0, $len);
    }

    // we already have this much for this read
    $len -= strlen($ret);

    while (true) {
      $buf = $this->read($len);
      if ($buf === '') {
        // Put back what we may have already read for this read.
        // Don't advance pos
        $this->rBuf_ .= $ret;

        throw new TTransportException('TNonBlockingSocket: readAll could not'.
                                      ' read '.$len.' bytes from '.
                                      $this->host_.':'.$this->port_);
      } else if (($sz = strlen($buf)) < $len) {
        $ret .= $buf;
        $len -= $sz;
      } else {
        $ret .= $buf;
        $this->rBuf_ .= $ret;
        $this->rBufPos_ += strlen($ret); // advance pos to next read

        return $ret;
      }
    }
  }

  /**
   * Occasionally we will restart several readAlls due to a failure,
   * EAGAIN for instance, and we want to read all data we already buffered.
   * In this case call resetBufferPos. This occurs for example when a gen
   * client does several recevies, and fails on one other than the first.
   * It then retries all of them
   */
  public function resetBufferPos() {
    $this->rBufPos_ = 0;
  }

  /*
   * Clear the buffer and reset it
   */
  public function clearBuf() {
    $this->rBuf_ = '';
    $this->rBufPos_ = 0;
  }

  /**
   * Read from the socket
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  public function read($len) {
    if ($this->sockRecvCapacity_ !== null) {
      $len = min($len, $this->sockRecvCapacity_);
    }

    $t_start = microtime(true);
    $data = @socket_read($this->handle_, $len);
    $t_delta = microtime(true) - $t_start;
    ProfilingCounters::incrCount(IProfilingCounters::THRIFT_READ_COUNT);
    ProfilingCounters::incrCount(
      IProfilingCounters::THRIFT_READ_BYTES, strlen($data));
    ProfilingCounters::incrDuration(
      IProfilingCounters::THRIFT_READ_DURATION, $t_delta);

    if ($data === FALSE || $data === '') {
      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error =  "read: no data to be read ".$this->host_.':'
        .$this->port_.' ('.$errstr.' ['.$errno.'])';
      if ($this->debug_) {
        call_user_func($this->debugHandler_, $error);
      }

      return '';
    }

    return $data;
  }

  /**
   * Do a buffered write. Use doWrite to notify when the socket can be written.
   *
   * @param string $buf The data to write
   */
  public function write($buf) {
    $this->wBuf_ .= $buf;
  }

  public function doWrite() {

    $t_start = microtime(true);
    $got = @socket_write($this->handle_, $this->wBuf_);
    $t_delta = microtime(true) - $t_start;
    ProfilingCounters::incrCount(IProfilingCounters::THRIFT_WRITE_COUNT);
    ProfilingCounters::incrCount(
      IProfilingCounters::THRIFT_WRITE_BYTES, (int)$got);
    ProfilingCounters::incrDuration(
      IProfilingCounters::THRIFT_WRITE_DURATION, $t_delta);

    if ($got === 0 || $got === FALSE) {
      // Could not write
      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error = "doWrite: write failed ($errno): $errstr ".
                                                 $this->host_.':'.$this->port_;
      throw new TTransportException($error);
    }

    $this->wBuf_ = substr($this->wBuf_, $got);
  }

  /**
   * Do we have buffered data to send?
   *
   * @return bool
   */
  public function haveData() {
    return strlen($this->wBuf_) > 0;
  }

  /**
   * No flush implemented.
   * Generated code will flush on send, we'd like to send as data becomes
   * available without blocking.
   */
  public function flush() {
  }
}

/**
 * Wrapper class over TNonBlockingSocket that suppresses
 * the exception "operation in progress 115" when the open()
 * is called. This is helpful for classes that don't
 * handle properly this event. Ie. TFramedTransport.
 */
class TNonBlockingSocketNoThrow extends TNonBlockingSocket {
  public function open() {
    try {
      parent::open();
    } catch (Exception $e) {
      $op_in_progress = (strpos($e->getMessage(),
                                "socket_connect error") !== false) &&
                        (strpos($e->getMessage(), "[115]") !== false);
      if (!$op_in_progress) {
        throw $e;
      }
    }
  }
}
