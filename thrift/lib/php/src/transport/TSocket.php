<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.transport
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftRemoteConn.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/InstrumentedTTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/InstrumentedTTransportTrait.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TTransportStatus.php';
class TSocket extends TTransport
  implements TTransportStatus, InstrumentedTTransport, IThriftRemoteConn {
  use InstrumentedTTransportTrait;
  private $handle_ = null;
  protected $host_ = "localhost";
  protected $port_ = 9090;
  protected $lport_ = 0;
  private $sendTimeout_ = 100;
  private $recvTimeout_ = 750;
  private $sendTimeoutSet_ = false;
  private $persist_ = false;
  private $readAttemptStart_ = null;
  private $writeAttemptStart_ = null;
  protected $debug_ = false;
  protected $debugHandler_;
  protected $errstr_ = null;
  protected $errno_ = null;
  protected $maxReadChunkSize_ = null;
  public function __construct(
    $host = "localhost",
    $port = 9090,
    $persist = false,
    $debugHandler = null
  ) {
    $this->host_ = $host;
    $this->port_ = $port;
    $this->persist_ = $persist;
    $this->debugHandler_ =
      \hacklib_cast_as_boolean($debugHandler) ?: fun("error_log");
  }
  public function setMaxReadChunkSize($maxReadChunkSize) {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }
  public function setHandle($handle) {
    $this->handle_ = $handle;
    return $this;
  }
  public function getMetaData() {
    return stream_get_meta_data($this->handle_);
  }
  public function getSendTimeout() {
    return $this->sendTimeout_;
  }
  public function setSendTimeout($timeout) {
    $this->sendTimeout_ = $timeout;
  }
  public function getRecvTimeout() {
    return $this->recvTimeout_;
  }
  public function setRecvTimeout($timeout) {
    $this->recvTimeout_ = $timeout;
  }
  public function setDebug($debug) {
    $this->debug_ = $debug;
  }
  public function getHost() {
    return $this->host_;
  }
  public function getPort() {
    return $this->port_;
  }
  public function getErrStr() {
    return $this->errstr_;
  }
  public function getErrNo() {
    return $this->errno_;
  }
  public function isOpen() {
    return is_resource($this->handle_);
  }
  public function open() {
    if (\hacklib_cast_as_boolean($this->isOpen())) {
      throw new TTransportException(
        "TSocket: socket already connected",
        TTransportException::ALREADY_OPEN
      );
    }
    if ($this->host_ === null) {
      throw new TTransportException(
        "TSocket: cannot open null host",
        TTransportException::NOT_OPEN
      );
    }
    if ($this->port_ <= 0) {
      throw new TTransportException(
        "TSocket: cannot open without port",
        TTransportException::NOT_OPEN
      );
    }
    $handle = null;
    if (\hacklib_cast_as_boolean($this->persist_)) {
      $handle = pfsockopen(
        $this->host_,
        $this->port_,
        $this->errno_,
        $this->errstr_,
        $this->sendTimeout_ / 1000.0
      );
    } else {
      $handle = fsockopen(
        $this->host_,
        $this->port_,
        $this->errno_,
        $this->errstr_,
        $this->sendTimeout_ / 1000.0
      );
    }
    if (!\hacklib_cast_as_boolean($handle)) {
      $error = "TSocket: could not connect to ".$this->host_.":".$this->port_;
      $error .= " (".$this->errstr_." [".$this->errno_."])";
      if (\hacklib_cast_as_boolean($this->debug_)) {
        call_user_func($this->debugHandler_, $error);
      }
      throw new TTransportException(
        $error,
        TTransportException::COULD_NOT_CONNECT
      );
    }
    $this->handle_ = $handle;
    $sock_name = stream_socket_get_name($this->handle_, false);
    $this->lport_ = end(explode(":", $sock_name));
    stream_set_timeout($this->handle_, 0, $this->sendTimeout_ * 1000);
    $this->sendTimeoutSet_ = true;
  }
  public function close() {
    if (!\hacklib_cast_as_boolean($this->persist_)) {
      fclose($this->handle_);
      $this->handle_ = null;
    }
  }
  public function isReadable() {
    return $this->isSocketActionable($this->handle_, $check_read = true);
  }
  public function isWritable() {
    $writable =
      $this->isSocketActionable($this->handle_, $check_read = false);
    if ((!\hacklib_cast_as_boolean($writable)) && ($this->sendTimeout_ > 0)) {
      if ($this->writeAttemptStart_ === null) {
        $this->writeAttemptStart_ = microtime(true);
      }
      if ((microtime(true) - ((int) $this->writeAttemptStart_)) >
          ($this->sendTimeout_ / 1000.0)) {
        throw new TTransportException(
          "TSocket: socket not writable after ".$this->sendTimeout_."ms",
          TTransportException::TIMED_OUT
        );
      }
    }
    return $writable;
  }
  private function isSocketActionable($socket, $check_read) {
    if ($socket === null) {
      return true;
    }
    $read = $write = array();
    if (\hacklib_cast_as_boolean($check_read)) {
      $read = array($socket);
    } else {
      $write = array($socket);
    }
    $excpt = array();
    $ret = stream_select($read, $write, $excpt, 0, 0);
    if ($ret === false) {
      $error = "TSocket: stream_select failed on socket.";
      if (\hacklib_cast_as_boolean($this->debug_)) {
        call_user_func($this->debugHandler_, $error);
      }
      throw new TTransportException($error);
    }
    return $ret !== 0;
  }
  private function readChunk($len) {
    if ($this->maxReadChunkSize_ !== null) {
      $len = min($len, $this->maxReadChunkSize_);
    }
    $res = fread($this->handle_, $len);
    $size = strlen($res);
    $this->onRead($size);
    return $res;
  }
  public function readAll($len) {
    if (\hacklib_cast_as_boolean($this->sendTimeoutSet_)) {
      $sec = 0;
      if ($this->recvTimeout_ > 1000) {
        $msec = $this->recvTimeout_ % 1000;
        $sec = ($this->recvTimeout_ - $msec) / 1000;
      } else {
        $msec = $this->recvTimeout_;
      }
      stream_set_timeout($this->handle_, $sec, $msec * 1000);
      $this->sendTimeoutSet_ = false;
    }
    $pre = "";
    while (true) {
      $t_start = microtime(true);
      $buf = $this->readChunk($len);
      $t_stop = microtime(true);
      if (($buf === null) || ($buf === "")) {
        $read_err_detail = sprintf(
          "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.",
          $len,
          $this->host_,
          $this->port_,
          $this->lport_,
          ($t_stop - $t_start) * 1000
        );
        $md = stream_get_meta_data($this->handle_);
        if (\hacklib_cast_as_boolean($md[\hacklib_id("timed_out")])) {
          throw new TTransportException(
            "TSocket: timeout while reading ".$read_err_detail,
            TTransportException::TIMED_OUT
          );
        } else {
          $md_str = str_replace("\n", " ", print_r($md, true));
          throw new TTransportException(
            "TSocket: could not read ".$read_err_detail,
            TTransportException::COULD_NOT_READ
          );
        }
      } else {
        if (($sz = strlen($buf)) < $len) {
          $md = stream_get_meta_data($this->handle_);
          if (\hacklib_cast_as_boolean($md[\hacklib_id("timed_out")])) {
            $read_err_detail = sprintf(
              "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.",
              $len,
              $this->host_,
              $this->port_,
              $this->lport_,
              ($t_stop - $t_start) * 1000
            );
            throw new TTransportException(
              "TSocket: timeout while reading ".$read_err_detail,
              TTransportException::TIMED_OUT
            );
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
    throw new TTransportException("TSocket: You shouldn't be here");
  }
  public function read($len) {
    if (\hacklib_cast_as_boolean($this->sendTimeoutSet_)) {
      stream_set_timeout($this->handle_, 0, $this->recvTimeout_ * 1000);
      $this->sendTimeoutSet_ = false;
    }
    $t_start = microtime(true);
    $data = $this->readChunk($len);
    $t_stop = microtime(true);
    if (($data === null) || ($data === "")) {
      $read_err_detail = sprintf(
        "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.",
        $len,
        $this->host_,
        $this->port_,
        $this->lport_,
        ($t_stop - $t_start) * 1000
      );
      $md = stream_get_meta_data($this->handle_);
      if (\hacklib_cast_as_boolean($md[\hacklib_id("timed_out")])) {
        throw new TTransportException(
          "TSocket: timeout while reading ".$read_err_detail,
          TTransportException::TIMED_OUT
        );
      } else {
        $md_str = str_replace("\n", " ", print_r($md, true));
        throw new TTransportException(
          "TSocket: could not read ".$read_err_detail,
          TTransportException::COULD_NOT_READ
        );
      }
    } else {
      $this->readAttemptStart_ = null;
    }
    $this->onRead(strlen($data));
    return $data;
  }
  public function nonBlockingRead($len) {
    $md = stream_get_meta_data($this->handle_);
    $is_blocking = $md[\hacklib_id("blocked")];
    if (\hacklib_cast_as_boolean($is_blocking) &&
        (!\hacklib_cast_as_boolean(stream_set_blocking($this->handle_, 0)))) {
      throw new TTransportException(
        "TSocket: "."cannot set stream to non-blocking"
      );
    }
    $data = $this->readChunk($len);
    if ($data === null) {
      throw new TTransportException("TSocket: failed in non-blocking read");
    }
    if (\hacklib_cast_as_boolean($is_blocking) &&
        (!\hacklib_cast_as_boolean(stream_set_blocking($this->handle_, 1)))) {
      throw new TTransportException(
        "TSocket: "."cannot swtich stream back to blocking"
      );
    }
    $this->onRead(strlen($data));
    return $data;
  }
  public function write($buf) {
    if ($this->handle_ === null) {
      throw new TException("TSocket: handle_ is null");
    }
    $this->onWrite(strlen($buf));
    if (!\hacklib_cast_as_boolean($this->sendTimeoutSet_)) {
      stream_set_timeout($this->handle_, 0, $this->sendTimeout_ * 1000);
      $this->sendTimeoutSet_ = true;
    }
    while (strlen($buf) > 0) {
      $buflen = strlen($buf);
      $t_start = microtime(true);
      $got = fwrite($this->handle_, $buf);
      $write_time = microtime(true) - $t_start;
      if (($got === 0) || (!\hacklib_cast_as_boolean(is_int($got)))) {
        $read_err_detail = sprintf(
          "%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.",
          $buflen,
          $this->host_,
          $this->port_,
          $this->lport_,
          $write_time * 1000
        );
        $md = stream_get_meta_data($this->handle_);
        if (\hacklib_cast_as_boolean($md[\hacklib_id("timed_out")])) {
          throw new TTransportException(
            "TSocket: timeout while writing ".$read_err_detail,
            TTransportException::TIMED_OUT
          );
        } else {
          $md_str = str_replace("\n", " ", print_r($md, true));
          throw new TTransportException(
            "TSocket: could not write ".$read_err_detail,
            TTransportException::COULD_NOT_WRITE
          );
        }
      }
      $buf = substr($buf, $got);
    }
    $this->writeAttemptStart_ = null;
  }
  public function flush() {
    $ret = fflush($this->handle_);
    if ($ret === false) {
      throw new TTransportException(
        "TSocket: could not flush ".$this->host_.":".$this->port_
      );
    }
  }
  public static function hacklib_initialize_statics() {
    self::hacklib_initialize_statics_InstrumentedTTransportTrait();
  }
}
TSocket::hacklib_initialize_statics();
