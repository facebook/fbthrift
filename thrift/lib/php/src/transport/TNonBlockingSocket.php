<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @package thrift.transport
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftRemoteConn.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
class TNonBlockingSocket extends TTransport implements IThriftRemoteConn {
  private $handle_ = null;
  protected $host_ = "localhost";
  protected $port_ = 9090;
  protected $ipV6_ = false;
  protected $wBuf_ = "";
  protected $rBuf_ = "";
  protected $rBufPos_ = 0;
  protected $debug_ = false;
  protected $debugHandler_;
  private $sockRecvCapacity_ = null;
  public function __construct(
    $host = "localhost",
    $port = 9090,
    $debugHandler = null
  ) {
    $this->host_ = $host;
    $this->port_ = $port;
    $this->ipV6_ = \hacklib_equals(strlen(inet_pton($host)), 16);
    $this->debugHandler_ =
      \hacklib_cast_as_boolean($debugHandler) ?: fun("error_log");
  }
  public function getHost() {
    return $this->host_;
  }
  public function getPort() {
    return $this->port_;
  }
  public function getSocket() {
    return $this->handle_;
  }
  public function isOpen() {
    return is_resource($this->handle_);
  }
  public function getRecvTimeout() {
    return 0;
  }
  public function setRecvTimeout($timeout) {
    throw new TTransportException("setRecvTimeout is insupported");
  }
  public function isReadable() {
    return $this->isOpen();
  }
  public function isWritable() {
    return $this->isOpen();
  }
  public function setDebug($debug) {
    $this->debug_ = $debug;
  }
  public function open() {
    if (\hacklib_cast_as_boolean($this->ipV6_)) {
      $handle = socket_create(AF_INET6, SOCK_STREAM, SOL_TCP);
    } else {
      $handle = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    }
    if ($handle === false) {
      $error = "TNonBlockingSocket: Could not create socket";
      throw new TTransportException($error);
    }
    $this->handle_ = $handle;
    if (!\hacklib_cast_as_boolean(socket_set_nonblock($this->handle_))) {
      $error = "TNonBlockingSocket: Could not set nonblocking.";
      throw new TTransportException($error);
    }
    $res = socket_connect($this->handle_, $this->host_, $this->port_);
    if (!\hacklib_cast_as_boolean($res)) {
      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error =
        "TNonBlockingSocket: socket_connect error (".
        ((string) $errstr).
        "[".
        ((string) $errno).
        "])";
      if (\hacklib_not_equals($errno, 115)) {
        if (\hacklib_cast_as_boolean($this->debug_)) {
          call_user_func($this->debugHandler_, $error);
        }
      }
      throw new TTransportException($error);
    }
    $wBuf_ = "";
    $rBuf_ = "";
    $rBufPos_ = 0;
    $this->sockRecvCapacity_ =
      socket_get_option($this->handle_, SOL_SOCKET, SO_RCVBUF);
    if (\hacklib_equals($this->sockRecvCapacity_, false)) {
      $this->sockRecvCapacity_ = null;
    }
  }
  public function close() {
    if ($this->handle_ !== null) {
      socket_close($this->handle_);
    }
    $this->handle_ = null;
  }
  public function readAll($len) {
    $ret = (string) substr($this->rBuf_, $this->rBufPos_);
    $this->rBuf_ = (string) substr($this->rBuf_, 0, $this->rBufPos_);
    if ($len <= strlen($ret)) {
      $this->rBuf_ .= $ret;
      $this->rBufPos_ += $len;
      return substr($ret, 0, $len);
    }
    $len -= strlen($ret);
    while (true) {
      $buf = $this->read($len);
      if ($buf === "") {
        $this->rBuf_ .= $ret;
        throw new TTransportException(
          "TNonBlockingSocket: readAll could not".
          " read ".
          $len.
          " bytes from ".
          $this->host_.
          ":".
          $this->port_
        );
      } else {
        if (($sz = strlen($buf)) < $len) {
          $ret .= $buf;
          $len -= $sz;
        } else {
          $ret .= $buf;
          $this->rBuf_ .= $ret;
          $this->rBufPos_ += strlen($ret);
          return $ret;
        }
      }
    }
    throw new TTransportException(
      "TNonBlockingSocket: You shouldn't be here"
    );
  }
  public function resetBufferPos() {
    $this->rBufPos_ = 0;
  }
  public function clearBuf() {
    $this->rBuf_ = "";
    $this->rBufPos_ = 0;
  }
  public function read($len) {
    if ($this->sockRecvCapacity_ !== null) {
      $len = min($len, $this->sockRecvCapacity_);
    }
    $data = socket_read($this->handle_, $len);
    if (($data === false) || ($data === "")) {
      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error =
        "read: no data to be read ".
        $this->host_.
        ":".
        $this->port_.
        " (".
        ((string) $errstr).
        " [".
        ((string) $errno).
        "])";
      if (\hacklib_cast_as_boolean($this->debug_)) {
        call_user_func($this->debugHandler_, $error);
      }
      return "";
    }
    return $data;
  }
  public function write($buf) {
    $this->wBuf_ .= $buf;
  }
  public function doWrite() {
    $got = socket_write($this->handle_, $this->wBuf_);
    if (($got === 0) || ($got === false)) {
      $errno = socket_last_error($this->handle_);
      $errstr = socket_strerror($errno);
      $error =
        "doWrite: write failed (".
        ((string) $errno).
        "): ".
        ((string) $errstr).
        " ".
        $this->host_.
        ":".
        $this->port_;
      throw new TTransportException($error);
    }
    $this->wBuf_ = substr($this->wBuf_, $got);
  }
  public function haveData() {
    return strlen($this->wBuf_) > 0;
  }
  public function flush() {}
}
