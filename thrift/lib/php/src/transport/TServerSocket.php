<?php
// Copyright 2004-present Facebook. All Rights Reserved.

/**
 * Server socket class
 */
class TServerSocket {
  protected $host;
  protected $port;
  protected $handle;

  private $send_buffer_size;
  private $recv_buffer_size;
  public function __construct($port,
                              $send_buffer_size=512,
                              $recv_buffer_size=512) {
    $this->host         = null;
    $this->port         = $port;
    $this->handle       = null;
    $this->send_buffer_size = $send_buffer_size;
    $this->recv_buffer_size = $recv_buffer_size;
  }

  public function listen() {
    $this->handle = stream_socket_server(
      'tcp://0.0.0.0:'.$this->port,
      $errno = 0,
      $errstr = '',
      STREAM_SERVER_BIND | STREAM_SERVER_LISTEN
    );
  }

  public function accept($timeout=-1) {
    if ($timeout !== 0) {
      $client = stream_socket_accept($this->handle, $timeout);
    } else {
      $client = @stream_socket_accept($this->handle, $timeout);
    }

    if (!$client) {
      return false;
    }

    $socket = new TSocket();
    $socket->setHandle($client);

    // We need to create a buffered transport because TSocket's isReadable
    // is not reliable in PHP (hphp is fine) and buffered transport is more
    // efficient (60% faster in my tests)
    $transport = new TBufferedTransport($socket, $this->send_buffer_size,
                                        $this->recv_buffer_size);

    return $transport;
  }

  public function close() {
    if (isset($this->handle)) {
      fclose($this->handle);
      $this->handle = null;
    }
  }
}
