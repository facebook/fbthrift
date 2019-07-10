<?hh

/*
 * Copyright 2006-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * Sockets implementation of the TTransport interface.
 *
 * @package thrift.transport
 */
class TSocket extends TTransport
  implements TTransportStatus, InstrumentedTTransport, IThriftRemoteConn {

  use InstrumentedTTransportTrait;

  /**
   * Handle to PHP socket
   *
   * @var resource
   */
  private ?resource $handle_ = null;

  /**
   * Remote hostname
   *
   * @var string
   */
  protected string $host_ = 'localhost';

  /**
   * Remote port
   *
   * @var int
   */
  protected int $port_ = 9090;

  /**
   * Local port
   *
   * @var int
   */
  protected DisableRuntimeTypecheck<int> $lport_ = 0;

  /**
   * Send timeout in milliseconds
   *
   * @var int
   */
  private int $sendTimeout_ = 100;

  /**
   * Recv timeout in milliseconds
   *
   * @var int
   */
  private int $recvTimeout_ = 750;

  /**
   * Is send timeout set?
   *
   * @var bool
   */
  private bool $sendTimeoutSet_ = false;

  /**
   * Persistent socket or plain?
   *
   * @var bool
   */
  private bool $persist_ = false;

  /**
   * When the current read is started
   *
   * @var int, null means no read is started
   */
  private ?int $readAttemptStart_ = null;

  /**
   * When the current write is started
   *
   * @var int, null means no write is started
   */
  private ?int $writeAttemptStart_ = null;

  /**
   * Debugging on?
   *
   * @var bool
   */
  protected bool $debug_ = false;

  /**
   * Debug handler
   *
   * @var mixed
   */
  protected Predicate<string> $debugHandler_;

  /**
   * error string (in case of open failure)
   *
   * @var string or null
   */
  protected ?DisableRuntimeTypecheck<string> $errstr_ = null;

  /**
   * error number (in case of open failure)
   *
   * @var int or null
   */
  protected ?DisableRuntimeTypecheck<int> $errno_ = null;

  /**
   * Specifies the maximum number of bytes to read
   * at once from internal stream.
   */
  protected ?int $maxReadChunkSize_ = null;

  /**
   * Socket constructor
   *
   * @param string $host         Remote hostname
   * @param int    $port         Remote port
   * @param bool   $persist      Whether to use a persistent socket
   * @param string $debugHandler Function to call for error logging
   */
  public function __construct(
    string $host = 'localhost',
    int $port = 9090,
    bool $persist = false,
    ?Predicate<string> $debugHandler = null,
  ) {
      $this->host_ = $host;
    $this->port_ = $port;
    $this->persist_ = $persist;
    $this->debugHandler_ = $debugHandler ?: fun('HH\\Lib\\PHP\\error_log');
  }

  /**
   * Sets the internal max read chunk size.
   * null for no limit (default).
   */
  public function setMaxReadChunkSize(int $maxReadChunkSize): void {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }

  /**
   * Sets the socket handle
   * @param resource $handle
   * @return $this
   */
  public function setHandle(resource $handle): this {
    $this->handle_ = $handle;
    return $this;
  }

  /**
   * Gets the meta_data for the current handle
   */
  public function getMetaData(): darray<string, mixed> {
    return PHP\stream_get_meta_data(nullthrows($this->handle_));
  }

  /**
   * Gets the send timeout.
   *
   * @return int timeout
   */
  public function getSendTimeout(): int {
    return $this->sendTimeout_;
  }

  /**
   * Sets the send timeout.
   *
   * @param int $timeout  Timeout in milliseconds.
   */
  public function setSendTimeout(int $timeout): void {
    $this->sendTimeout_ = $timeout;
  }

  /**
   * Gets the receive timeout.
   *
   * @return int timeout
   */
  public function getRecvTimeout(): int {
    return $this->recvTimeout_;
  }

  /**
   * Sets the receive timeout.
   *
   * @param int $timeout  Timeout in milliseconds.
   */
  public function setRecvTimeout(int $timeout): void {
    $this->recvTimeout_ = $timeout;
  }

  /**
   * Sets debugging output on or off
   *
   * @param bool $debug
   */
  public function setDebug(bool $debug): void {
    $this->debug_ = $debug;
  }

  /**
   * Get the host that this socket is connected to
   *
   * @return string host
   */
  public function getHost(): string {
    return $this->host_;
  }

  /**
   * Get the remote port that this socket is connected to
   *
   * @return int port
   */
  public function getPort(): int {
    return $this->port_;
  }

  /**
   * Get the error string in case of open failure
   *
   * @return errstr_ or null
   */
  public function getErrStr(): ?string {
    return $this->errstr_;
  }

  /**
   * Get the error number in case of open failure
   *
   * @return errno_ or null
   */
  public function getErrNo(): ?int {
    return $this->errno_;
  }

  /**
   * Tests whether this is open
   *
   * @return bool true if the socket is open
   */
  <<__Override>>
  public function isOpen(): bool {
    return $this->handle_ is resource;
  }

  /**
   * Connects the socket.
   */
  <<__Override>>
  public function open(): void {
    if ($this->isOpen()) {
      throw new TTransportException(
        'TSocket: socket already connected',
        TTransportException::ALREADY_OPEN,
      );
    }

    if ($this->host_ === null) {
      throw new TTransportException(
        'TSocket: cannot open null host',
        TTransportException::NOT_OPEN,
      );
    }

    if ($this->port_ <= 0) {
      throw new TTransportException(
        'TSocket: cannot open without port',
        TTransportException::NOT_OPEN,
      );
    }

    $handle = null;
    if ($this->persist_) {
      $__errno = $this->errno_;
      $__errstr = $this->errstr_;
      $handle = @PHP\pfsockopen(
        $this->host_,
        $this->port_,
        inout $__errno,
        inout $__errstr,
        $this->sendTimeout_ / 1000.0,
      );
      $this->errstr_ = $__errstr;
      $this->errno_ = $__errno;
    } else {
      $__errno = $this->errno_;
      $__errstr = $this->errstr_;
      $handle = @PHP\fsockopen(
        $this->host_,
        $this->port_,
        inout $__errno,
        inout $__errstr,
        $this->sendTimeout_ / 1000.0,
      );
      $this->errstr_ = $__errstr;
      $this->errno_ = $__errno;
    }

    // Connect failed?
    if (!$handle) {
      $error =
        'TSocket: could not connect to '.$this->host_.':'.$this->port_;
      $error .= ' ('.$this->errstr_.' ['.$this->errno_.'])';
      if ($this->debug_) {
        ($this->debugHandler_)($error);
      }
      throw new TTransportException(
        $error,
        TTransportException::COULD_NOT_CONNECT,
      );
    }

    $this->handle_ = $handle;

    $sock_name = PHP\stream_socket_get_name($this->handle_, false);
    // IPv6 is returned [2401:db00:20:702c:face:0:7:0]:port
    // or when stream_socket_get_name is buggy it is
    // 2401:db00:20:702c:face:0:7:0:port
    $this->lport_ = PHP\fb\end(PHP\explode(":", $sock_name));

    PHP\stream_set_timeout(nullthrows($this->handle_), 0, $this->sendTimeout_ * 1000);
    $this->sendTimeoutSet_ = true;
  }

  /**
   * Closes the socket.
   */
  <<__Override>>
  public function close(): void {
    if (!$this->persist_ && $this->handle_ !== null) {
      @PHP\fclose($this->handle_);
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
  public function isReadable(): bool {
    return $this->isSocketActionable($this->handle_, /* $check_read */ true);
  }

  /**
   *  Test to see if the socket is ready for writing. This method returns
   *  immediately. If calling this method in a loop one should sleep or do
   *  other work else CPU cycles will be wasted.
   *
   *  @return bool True if a non-blocking write can be preformed on the socket.
   */
  public function isWritable(): bool {
    $writable =
      $this->isSocketActionable($this->handle_, /* $check_read */ false);
    if (!$writable && $this->sendTimeout_ > 0) {
      if ($this->writeAttemptStart_ === null) {
        $this->writeAttemptStart_ = PHP\microtime(true);
      }
      if (PHP\microtime(true) -
          (int) $this->writeAttemptStart_ > ($this->sendTimeout_ / 1000.0)) {
        throw new TTransportException(
          'TSocket: socket not writable after '.$this->sendTimeout_.'ms',
          TTransportException::TIMED_OUT,
        );
      }
    }
    return $writable;
  }

  private function isSocketActionable(
    ?resource $socket,
    bool $check_read,
  ): bool {
    // the socket is technically actionable, although any read or write will
    // fail since close() was already called.
    if ($socket === null) {
      return true;
    }

    $read = varray[];
    $write = $read;
    if ($check_read) {
      $read = varray[$socket];
    } else {
      $write = varray[$socket];
    }

    $excpt = varray[];
    $ret = PHP\stream_select(inout $read, inout $write, inout $excpt, 0, 0);
    if ($ret === false) {
      $error = 'TSocket: stream_select failed on socket.';
      if ($this->debug_) {
        ($this->debugHandler_)($error);
      }
      throw new TTransportException($error);
    }

    return $ret !== 0;
  }

  /**
   * Reads maximum min($len, $maxReadChunkSize_) bytes
   * from the stream.
   */
  private function readChunk(int $len): ?string {
    if ($this->maxReadChunkSize_ !== null) {
      $len = Math\minva($len, $this->maxReadChunkSize_);
    }

    $res = @PHP\fread(nullthrows($this->handle_), $len);
    $size = Str\length($res);

    $this->onRead($size);
    return $res;
  }

  /**
   * Uses stream get contents to do the reading
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  <<__Override>>
  public function readAll(int $len): string {
    if ($this->sendTimeoutSet_) {
      $sec = 0;
      if ($this->recvTimeout_ > 1000) {
        $msec = $this->recvTimeout_ % 1000;
        $sec = (int)(($this->recvTimeout_ - $msec) / 1000);
      } else {
        $msec = $this->recvTimeout_;
      }
      PHP\stream_set_timeout(nullthrows($this->handle_), $sec, $msec * 1000);
      $this->sendTimeoutSet_ = false;
    }
    // This call does not obey stream_set_timeout values!
    // $buf = @stream_get_contents($this->handle_, $len);
    $pre = '';
    while (true) {
      $t_start = PHP\microtime(true);
      $buf = $this->readChunk($len);
      $t_stop = PHP\microtime(true);
      if ($buf === null || $buf === '') {
        $read_err_detail = Str\format(
          '%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.',
          $len,
          $this->host_,
          $this->port_,
          $this->lport_,
          ($t_stop - $t_start) * 1000,
        );
        $md = PHP\stream_get_meta_data(nullthrows($this->handle_));
        if ($md['timed_out']) {
          throw new TTransportException(
            'TSocket: timeout while reading '.$read_err_detail,
            TTransportException::TIMED_OUT,
          );
        } else {
          throw new TTransportException(
            'TSocket: could not read '.$read_err_detail,
            TTransportException::COULD_NOT_READ,
          );
        }
      } else {
        $sz = Str\length($buf);
        if ($sz < $len) {
          $md = PHP\stream_get_meta_data(nullthrows($this->handle_));
          if ($md['timed_out']) {
            $read_err_detail = Str\format(
              '%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.',
              $len,
              $this->host_,
              $this->port_,
              $this->lport_,
              ($t_stop - $t_start) * 1000,
            );
            throw new TTransportException(
              'TSocket: timeout while reading '.$read_err_detail,
              TTransportException::TIMED_OUT,
            );
          } else {
            $pre .= $buf;
            $len -= $sz;
          }
        } else {
          $this->readAttemptStart_ = null;
          $res = $pre.$buf;
          $this->onRead(Str\length($res));
          return $res;
        }
      }
    }

    throw new TTransportException("TSocket: You shouldn't be here");
  }

  /**
   * Read from the socket
   *
   * @param int $len How many bytes
   * @return string Binary data
   */
  <<__Override>>
  public function read(int $len): string {
    if ($this->sendTimeoutSet_) {
      PHP\stream_set_timeout(nullthrows($this->handle_), 0, $this->recvTimeout_ * 1000);
      $this->sendTimeoutSet_ = false;
    }
    $t_start = PHP\microtime(true);
    $data = $this->readChunk($len);
    $t_stop = PHP\microtime(true);
    if ($data === null || $data === '') {
      $read_err_detail = Str\format(
        '%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.',
        $len,
        $this->host_,
        $this->port_,
        $this->lport_,
        ($t_stop - $t_start) * 1000,
      );
      $md = PHP\stream_get_meta_data(nullthrows($this->handle_));
      if ($md['timed_out']) {
        throw new TTransportException(
          'TSocket: timeout while reading '.$read_err_detail,
          TTransportException::TIMED_OUT,
        );
      } else {
        throw new TTransportException(
          'TSocket: could not read '.$read_err_detail,
          TTransportException::COULD_NOT_READ,
        );
      }
    } else {
      $this->readAttemptStart_ = null;
    }

    $this->onRead(Str\length($data));
    return $data;
  }

  /**
   * Perform a nonblocking read.
   * @param int $len Number of bytes to read
   * @return string Binary data or '' is no data is read
   */
  public function nonBlockingRead(int $len): string {
    $md = PHP\stream_get_meta_data(nullthrows($this->handle_));
    $is_blocking = $md['blocked'];

    // If the stream is currently blocking, we will set to nonblocking
    // first
    if ($is_blocking && !PHP\stream_set_blocking(nullthrows($this->handle_), false)) {
      throw new TTransportException(
        'TSocket: '.'cannot set stream to non-blocking',
      );
    }

    $data = $this->readChunk($len);

    if ($data === null) {
      throw new TTransportException('TSocket: failed in non-blocking read');
    }

    // Switch back to blocking mode is necessary
    if ($is_blocking && !PHP\stream_set_blocking(nullthrows($this->handle_), true)) {
      throw new TTransportException(
        'TSocket: '.'cannot swtich stream back to blocking',
      );
    }
    $this->onRead(Str\length($data));
    return $data;
  }

  /**
   * Write to the socket.
   *
   * @param string $buf The data to write
   */
  <<__Override>>
  public function write(string $buf): void {
    if ($this->handle_ === null) {
      throw new TException('TSocket: handle_ is null');
    }

    $this->onWrite(Str\length($buf));

    if (!$this->sendTimeoutSet_) {
      PHP\stream_set_timeout(nullthrows($this->handle_), 0, $this->sendTimeout_ * 1000);
      $this->sendTimeoutSet_ = true;
    }

    while (Str\length(PHPism_FIXME::coerceToString($buf)) > 0) {
      $buflen = Str\length($buf);
      $t_start = PHP\microtime(true);
      $got = @PHP\fwrite(nullthrows($this->handle_), $buf);
      $write_time = PHP\microtime(true) - $t_start;

      if ($got === 0 || !($got is int)) {
        $read_err_detail = Str\format(
          '%d bytes from %s:%d to localhost:%d. Spent %2.2f ms.',
          $buflen,
          $this->host_,
          $this->port_,
          $this->lport_,
          $write_time * 1000,
        );
        $md = PHP\stream_get_meta_data(nullthrows($this->handle_));
        if ($md['timed_out']) {
          throw new TTransportException(
            'TSocket: timeout while writing '.$read_err_detail,
            TTransportException::TIMED_OUT,
          );
        } else {
          throw new TTransportException(
            'TSocket: could not write '.$read_err_detail,
            TTransportException::COULD_NOT_WRITE,
          );
        }
      }
      $buf = PHP\substr($buf, $got);
    }

    $this->writeAttemptStart_ = null;
  }

  /**
   * Flush output to the socket.
   */
  <<__Override>>
  public function flush(): void {
    $ret = PHP\fflush(nullthrows($this->handle_));

    if ($ret === false) {
      throw new TTransportException(
        'TSocket: could not flush '.$this->host_.':'.$this->port_,
      );
    }
  }
}
