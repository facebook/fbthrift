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
 * A memory buffer is a tranpsort that simply reads from and writes to an
 * in-memory string buffer. Anytime you call write on it, the data is simply
 * placed into a buffer, and anytime you call read, data is read from that
 * buffer.
 *
 * @package thrift.transport
 */
final class TMemoryBuffer
  extends TTransport
  implements IThriftBufferedTransport {

  private string $buf_ = '';
  private int $index_ = 0;
  private ?int $length_ = null;

  /**
   * Constructor. Optionally pass an initial value
   * for the buffer.
   */
  <<__Rx>>
  public function __construct(string $buf = '') {
    $this->buf_ = $buf;
  }

  <<__Override>>
  public function isOpen(): bool {
    return true;
  }

  <<__Override>>
  public function open(): void {}

  <<__Override>>
  public function close(): void {}

  private function length(): int {
    if ($this->length_ === null) {
      $this->length_ = Str\length($this->buf_);
    }
    return $this->length_;
  }

  public function available(): int {
    return $this->length() - $this->index_;
  }

  <<__Override>>
  public function write(string $buf): void {
    $this->buf_ .= $buf;
    $this->length_ = null; // reset length
  }

  <<__Override>>
  public function read(int $len): string {
    $available = $this->available();
    if ($available === 0) {
      $buffer_dump = PHP\bin2hex($this->buf_);
      throw new TTransportException(
        'TMemoryBuffer: Could not read '.
        $len.
        ' bytes from buffer.'.
        ' Original length is '.
        $this->length().
        ' Current index is '.
        $this->index_.
        ' Buffer content <start>'.
        $buffer_dump.
        '<end>',
        TTransportException::UNKNOWN,
      );
    }

    if ($available < $len) {
      $len = $available;
    }
    $ret = $this->peek($len);
    $this->index_ += $len;
    return $ret;
  }

  public function peek(int $len, int $start = 0): string {
    if ($len !== 1) {
      return Str\slice($this->buf_, $this->index_ + $start, $len);
    }
    if (Str\length($this->buf_)) {
      return $this->buf_[$this->index_ + $start];
    }
    return '';
  }

  public function putBack(string $buf): void {
    if ($this->available() === 0) {
      $this->buf_ = $buf;
    } else {
      $remaining = (string) PHP\substr($this->buf_, $this->index_);
      $this->buf_ = $buf.$remaining;
    }
    $this->length_ = null;
    $this->index_ = 0;
  }

  <<__RxShallow>>
  public function getBuffer(): @string {
    if ($this->index_ === 0) {
      return $this->buf_;
    }
    return PHP\substr($this->buf_, $this->index_);
  }

  public function resetBuffer(): void {
    $this->buf_ = '';
    $this->index_ = 0;
    $this->length_ = null;
  }
}
