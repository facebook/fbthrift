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
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TSocket.php';
class TServerSocket {
  protected $host;
  protected $port;
  protected $handle;
  private $send_buffer_size;
  private $recv_buffer_size;
  public function __construct(
    $port,
    $send_buffer_size = 512,
    $recv_buffer_size = 512
  ) {
    $this->host = null;
    $this->port = $port;
    $this->handle = null;
    $this->send_buffer_size = $send_buffer_size;
    $this->recv_buffer_size = $recv_buffer_size;
  }
  public function listen() {
    foreach (array("[::]", "0.0.0.0") as $addr) {
      $errno = 0;
      $errstr = "";
      $this->handle = stream_socket_server(
        "tcp://".$addr.":".$this->port,
        $errno,
        $errstr,
        STREAM_SERVER_BIND | STREAM_SERVER_LISTEN
      );
      if ($this->handle !== false) {
        break;
      }
    }
  }
  public function accept($timeout = -1) {
    if ($timeout !== 0) {
      $client = stream_socket_accept($this->handle, $timeout);
    } else {
      $client = stream_socket_accept($this->handle, $timeout);
    }
    if (!\hacklib_cast_as_boolean($client)) {
      return null;
    }
    $socket = new TSocket();
    $socket->setHandle($client);
    $transport = new TBufferedTransport(
      $socket,
      $this->send_buffer_size,
      $this->recv_buffer_size
    );
    return $transport;
  }
  public function close() {
    if (\hacklib_cast_as_boolean(isset($this->handle))) {
      fclose($this->handle);
      $this->handle = null;
    }
  }
}
