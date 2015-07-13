<?hh

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
 * Server socket class
 */
class TServerSocket {
  protected ?string $host;
  protected int $port;
  protected ?resource $handle;

  private int $send_buffer_size;
  private int $recv_buffer_size;
  public function __construct(
    int $port,
    int $send_buffer_size = 512,
    int $recv_buffer_size = 512,
  ) {
    $this->host = null;
    $this->port = $port;
    $this->handle = null;
    $this->send_buffer_size = $send_buffer_size;
    $this->recv_buffer_size = $recv_buffer_size;
  }

  public function listen(): void {
    foreach (array('[::]', '0.0.0.0') as $addr) {
      $errno = 0;
      $errstr = '';
      $this->handle = stream_socket_server(
        'tcp://'.$addr.':'.$this->port,
        $errno,
        $errstr,
        STREAM_SERVER_BIND | STREAM_SERVER_LISTEN,
      );
      if ($this->handle !== false) {
        break;
      }
    }
  }

  public function accept(int $timeout = -1): ?TBufferedTransport {
    if ($timeout !== 0) {
      $client = stream_socket_accept($this->handle, $timeout);
    } else {
      $client = @stream_socket_accept($this->handle, $timeout);
    }

    if (!$client) {
      return null;
    }

    $socket = new TSocket();
    $socket->setHandle($client);

    // We need to create a buffered transport because TSocket's isReadable
    // is not reliable in PHP (hphp is fine) and buffered transport is more
    // efficient (60% faster in my tests)
    $transport = new TBufferedTransport(
      $socket,
      $this->send_buffer_size,
      $this->recv_buffer_size,
    );

    return $transport;
  }

  public function close(): void {
    if (isset($this->handle)) {
      fclose($this->handle);
      $this->handle = null;
    }
  }
}
