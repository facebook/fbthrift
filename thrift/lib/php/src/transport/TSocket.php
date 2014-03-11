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
 * Sockets implementation of the TTransport interface.
 *
 * @package thrift.transport
 */
class TSocket
  extends TTransport
  implements TTransportStatus, InstrumentedTTransport {

  use InstrumentedTTransportTrait;

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
  protected $port_ = 9090;

  /**
   * Local port
   *
   * @var int
   */
  protected $lport_ = 0;

  /**
   * Send timeout in milliseconds
   *
   * @var int
   */
  private $sendTimeout_ = 100;

  /**
   * Recv timeout in milliseconds
   *
   * @var int
   */
  private $recvTimeout_ = 750;

  /**
   * Is send timeout set?
   *
   * @var bool
   */
  private $sendTimeoutSet_ = FALSE;

  /**
   * Persistent socket or plain?
   *
   * @var bool
   */
  private $persist_ = FALSE;

  /**
   * When the current read is started
   *
   * @var int, null means no read is started
   */
  private $readAttemptStart_ = null;

  /**
   * When the current write is started
   *
   * @var int, null means no write is started
   */
  private $writeAttemptStart_ = null;

  /**
   * Debugging on?
   *
   * @var bool
   */
  protected $debug_ = FALSE;

  /**
   * Debug handler
   *
   * @var mixed
   */
  protected $debugHandler_ = null;

  /**
   * error string (in case of open failure)
   *
   * @var string or null
   */
  protected $errstr_ = null;

  /**
   * error number (in case of open failure)
   *
   * @var int or null
   */
  protected $errno_ = null;

  /**
   * Specifies the maximum number of bytes to read
   * at once from internal stream.
   */
  protected $maxReadChunkSize_ = null;

  /**
   * Socket constructor
   *
   * @param string $host         Remote hostname
   * @param int    $port         Remote port
   * @param bool   $persist      Whether to use a persistent socket
   * @param string $debugHandler Function to call for error logging
   */
  public function __construct($host='localhost',
                              $port=9090,
                              $persist=FALSE,
                              $debugHandler=null) {
    $this->host_ = $host;
    $this->port_ = $port;
    $this->persist_ = $persist;
    $this->debugHandler_ = $debugHandler ?: 'error_log';
  }

  /**
   * Sets the internal max read chunk size.
   * null for no limit (default).
   */
  public function setMaxReadChunkSize($maxReadChunkSize) {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }

  /**
   * Sets the socket handle
   * @param resource $handle
   * @return $this
   */
  public function setHandle($handle) {
    $this->handle_ = $handle;
    return $this;
  }

  /**
   * Gets the meta_data for the current handle
   */
  public function getMetaData() {
    return stream_get_meta_data($this->handle_);
  }

  /**
   * Gets the send timeout.
   *
   * @return int timeout
   */
  public function getSendTimeout() {
    return $this->sendTimeout_;
  }

  /**
   * Sets the send timeout.
   *
   * @param int $timeout  Timeout in milliseconds.
   */
  public function setSendTimeout($timeout) {
    $this->sendTimeout_ = $timeout;
  }

  /**
   * Gets the receive timeout.
   *
   * @return int timeout
   */
  public function getRecvTimeout() {
    return $this->recvTimeout_;
  }

  /**
   * Sets the receive timeout.
   *
   * @param int $timeout  Timeout in milliseconds.
   */
  public function setRecvTimeout($timeout) {
    $this->recvTimeout_ = $timeout;
  }

  /**
   * Sets debugging output on or off
   *
   * @param bool $debug
   */
  public function setDebug($debug) {
    $this->debug_ = $debug;
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
   * Get the error string in case of open failure
   *
   * @return errstr_ or null
   */
  public function getErrStr() {
    return $this->errstr_;
  }

  /**
   * Get the error number in case of open failure
   *
   * @return errno_ or null
   */
  public function getErrNo() {
    return $this->errno_;
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
   * Connects the socket.
   */
  public function open() {
    if ($this->isOpen()) {
      throw new TTransportException('TSocket: socket already connected',
                                    TTransportException::ALREADY_OPEN);
    }

    if (empty($this->host_)) {
      throw new TTransportException('TSocket: cannot open null host',
                                    TTransportException::NOT_OPEN);
    }

    if ($this->port_ <= 0) {
      throw new TTransportException('TSocket: cannot open without port',
                                    TTransportException::NOT_OPEN);
    }

    $t_start = microtime(true);
    if ($this->persist_) {
      $this->handle_ = @pfsockopen($this->host_,
                                   $this->port_,
                                   $this->errno_,
                                   $this->errstr_,
                                   $this->sendTimeout_ / 1000.0);
    } else {
      $this->handle_ = @fsockopen($this->host_,
                                  $this->port_,
                                  $this->errno_,
                                  $this->errstr_,
                                  $this->sendTimeout_ / 1000.0);
    }
    $t_delta = microtime(true) - $t_start;

    // Connect failed?
    if ($this->handle_ === FALSE) {
      $error = 'TSocket: could not connect to '.$this->host_ .':'.$this->port_;
      $error .= ' (' . $this->errstr_ . ' [' . $this->errno_ . '])';
      if ($this->debug_) {
        call_user_func($this->debugHandler_, $error);
      }
      throw new TTransportException($error,
                                    TTransportException::COULD_NOT_CONNECT);
    }

    $sock_name = stream_socket_get_name($this->handle_, false);
    // IPv6 is returned [2401:db00:20:702c:face:0:7:0]:port
    // or when stream_socket_get_name is buggy it is
    // 2401:db00:20:702c:face:0:7:0:port
    $this->lport_ = end(explode(":", $sock_name));

    stream_set_timeout($this->handle_, 0, $this->sendTimeout_ * 1000);
    $this->sendTimeoutSet_ = TRUE;
  }

  /**
   * Closes the socket.
   */
  public function close() {
    if (!$this->persist_) {
      @fclose($this->handle_);
      $this->handle_ = null;
    }
  }

  /**
   *  Test to see if the socket is ready for reading. This method returns
   *  immediately. If calling this method in a loop one should sleep or do
   *  other work else CPU cycles will be wasted.
   *
   *  @return bool  True if a non-blocking read of at least one character can
   *                be preformed on the socket.
   */
  public function isReadable() {
    return $this->isSocketActionable($this->handle_, $check_read = true);
  }

  /**
   *  Test to see if the socket is ready for writing. This method returns
   *  immediately. If calling this method in a loop one should sleep or do
   *  other work else CPU cycles will be wasted.
   *
   *  @return bool True if a non-blocking write can be preformed on the socket.
   */
  public function isWritable() {
    $writable = $this->isSocketActionable($this->handle_, $check_read = false);
    if (!$writable && $this->sendTimeout_ > 0) {
      if (!isset($this->writeAttemptStart_)) {
        $this->writeAttemptStart_ = microtime(true);
      }
      if (microtime(true) - $this->writeAttemptStart_ >
          ($this->sendTimeout_ / 1000.0)) {
        throw new TTransportException('TSocket: socket not writable after '.
                                      $this->sendTimeout_.'ms',
                                      TTransportException::TIMED_OUT);
      }
    }
    return $writable;
  }

  private function isSocketActionable($socket, $check_read) {
    // the socket is technically actionable, although any read or write will
    // fail since close() was already called.
    if ($socket === null) {
      return true;
    }

    $read = $write = array();
    if ($check_read) {
      $read = array($socket);
    } else {
      $write = array($socket);
    }

    $excpt = array();
    $ret = stream_select($read, $write, $excpt, 0, 0);
    if ($ret === false) {
      $error = 'TSocket: stream_select failed on socket.';
      if ($this->debug_) {
        call_user_func($this->debugHandler_, $error);
      }
      throw new TTransportException($error);
    }

    return $ret !== 0;
  }

  /**
   * Reads maximum min($len, $maxReadChunkSize_) bytes
   * from the stream.
   */
  private function readChunk($len) {
    if ($this->maxReadChunkSize_ !== null) {
      $len = min($len, $this->maxReadChunkSize_);
    }
    $t_start = microtime(true);
    $res = @fread($this->handle_, $len);
    $size = strlen($res);
    $t_delta = microtime(true) - $t_start;

    $this->onRead($size);
    return $res;
  }

  /**
   * Uses stream get contents to do the reading
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  public function readAll($len) {
    if ($this->sendTimeoutSet_) {
      $sec = 0;
      if ($this->recvTimeout_ > 1000) {
        $msec = $this->recvTimeout_ % 1000;
        $sec = ($this->recvTimeout_ - $msec) / 1000;
      } else {
        $msec = $this->recvTimeout_;
      }
      stream_set_timeout($this->handle_, $sec, $msec * 1000);
      $this->sendTimeoutSet_ = FALSE;
    }
    // This call does not obey stream_set_timeout values!
    // $buf = @stream_get_contents($this->handle_, $len);
    $pre = null;
    while (TRUE) {
      $t_start = microtime(true);
      $buf = $this->readChunk($len);
      $t_stop = microtime(true);
      $err_fmt = "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.";
      $read_err_detail = $err_fmt; //TODO
      if ($buf === FALSE || $buf === '') {
        $md = stream_get_meta_data($this->handle_);
        if ($md['timed_out']) {
          throw new TTransportException('TSocket: timeout while reading '
                                        .$read_err_detail,
                                        TTransportException::TIMED_OUT);
        } else {
          $md_str = str_replace("\n", " ", print_r($md, true));
          throw new TTransportException('TSocket: could not read '
                                        .$read_err_detail,
                                        TTransportException::COULD_NOT_READ);
        }
      } else if (($sz = strlen($buf)) < $len) {
        $md = stream_get_meta_data($this->handle_);
        if ($md['timed_out']) {
          throw new TTransportException('TSocket: timeout while reading '
                                        .$read_err_detail,
                                        TTransportException::TIMED_OUT);
        } else {
          $pre .= $buf;
          $len -= $sz;
        }
      } else {
        $this->readAttemptStart_ = null;
        $res = $pre.$buf;
        $this->onRead(strlen($res));
        return $res;
      }
    }
  }

  /**
   * Read from the socket
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  public function read($len) {
    if ($this->sendTimeoutSet_) {
      stream_set_timeout($this->handle_, 0, $this->recvTimeout_ * 1000);
      $this->sendTimeoutSet_ = FALSE;
    }
    $t_start = microtime(true);
    $data = $this->readChunk($len);
    $t_stop = microtime(true);
    $err_fmt = "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.";
    $read_err_detail = $err_fmt; // TODO
    if ($data === FALSE || $data === '') {
      $md = stream_get_meta_data($this->handle_);
      if ($md['timed_out']) {
        throw new TTransportException('TSocket: timeout while reading '
                                      .$read_err_detail,
                                      TTransportException::TIMED_OUT);
      } else {
        $md_str = str_replace("\n", " ", print_r($md, true));
        throw new TTransportException('TSocket: could not read '
                                      .$read_err_detail,
                                      TTransportException::COULD_NOT_READ);
      }
    } else {
      $this->readAttemptStart_ = null;
    }

    $this->onRead(strlen($data));
    return $data;
  }

  /**
   * Perform a nonblocking read.
   * @param int $len Number of bytes to read
   * @return string Binary data or '' is no data is read
   */
  public function nonBlockingRead($len) {
    $md = stream_get_meta_data($this->handle_);
    $is_blocking = $md['blocked'];

    // If the stream is currently blocking, we will set to nonblocking
    // first
    if ($is_blocking && !stream_set_blocking($this->handle_, 0)) {
      throw new TTransportException('TSocket: '
                                    .'cannot set stream to non-blocking');
    }

    $data = $this->readChunk($len);

    if ($data === false) {
      throw new TTransportException('TSocket: failed in non-blocking read');
    }

    // Switch back to blocking mode is necessary
    if ($is_blocking && !stream_set_blocking($this->handle_, 1)) {
      throw new TTransportException('TSocket: '
                                    .'cannot swtich stream back to blocking');
    }
    $this->onRead(strlen($data));
    return $data;
  }

  /**
   * Write to the socket.
   *
   * @param string $buf The data to write
   */
  public function write($buf) {
    if ($this->handle_ === null) {
      throw new TException('TSocket: handle_ is null');
    }

    $this->onWrite(strlen($buf));

    if (!$this->sendTimeoutSet_) {
      stream_set_timeout($this->handle_, 0, $this->sendTimeout_ * 1000);
      $this->sendTimeoutSet_ = TRUE;
    }

    while (strlen($buf) > 0) {
      $buflen = strlen($buf);
      $t_start = microtime(true);
      $got = @fwrite($this->handle_, $buf);
      $write_time = microtime(true) - $t_start;

      if ($got === 0 || $got === FALSE) {
        $err_fmt = "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.";
        $read_err_detail = $err_fmt; //TODO
        $md = stream_get_meta_data($this->handle_);
        if ($md['timed_out']) {
          throw new TTransportException('TSocket: timeout while writing '.
                                        $read_err_detail,
                                        TTransportException::TIMED_OUT);
        } else {
          $md_str = str_replace("\n", " ", print_r($md, true));
          throw new TTransportException('TSocket: could not write '.
                                        $read_err_detail,
                                        TTransportException::COULD_NOT_WRITE);
        }
      }
      $buf = substr($buf, $got);
    }

    $this->writeAttemptStart_ = null;
  }

  /**
   * Flush output to the socket.
   */
  public function flush() {
    $t_start = microtime(true);
    $ret = fflush($this->handle_);
    $t_delta = microtime(true) - $t_start;
    if ($ret === FALSE) {
      throw new TTransportException('TSocket: could not flush '.
                                    $this->host_.':'.$this->port_);
    }
  }
}
