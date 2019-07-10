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
 * File stream transport. Reads to and writes from the file streams
 *
 * @package thrift.transport
 */
final class TFileStream extends TTransport {

  const int MODE_R = 1;
  const int MODE_W = 2;

  private ?resource $inStream = null;
  private ?resource $outStream = null;

  private bool $read = false;
  private bool $write = false;

  private int $bufferSize = 1024 * 1024;
  private string $buffer = '';
  private int $bufferIndex = 0;

  public function __construct(int $mode, private string $filePath) {
    $this->read = (bool) ($mode & self::MODE_R);
    $this->write = (bool) ($mode & self::MODE_W);
  }

  public function setBufferSize(int $buffer_size): this {
    $this->bufferSize = $buffer_size;
    return $this;
  }

  <<__Override>>
  public function open(): void {
    if ($this->read) {
      $this->inStream = PHP\fopen($this->filePath, 'r');
      if (!($this->inStream is resource)) {
        throw new TException('TPhpStream: Could not open %s', $this->filePath);
      }
    }
    if ($this->write) {
      $this->outStream = PHP\fopen($this->filePath, 'w');
      if (!($this->outStream is resource)) {
        throw new TException('TPhpStream: Could not open %s', $this->filePath);
      }
    }
  }

  <<__Override>>
  public function close(): void {
    if ($this->read) {
      PHP\fclose(nullthrows($this->inStream));
      $this->inStream = null;
    }
    if ($this->write) {
      $this->flush();
      PHP\fclose(nullthrows($this->outStream));
      $this->outStream = null;
    }
  }

  <<__Override>>
  public function isOpen(): bool {
    return (!$this->read || $this->inStream is resource) &&
      (!$this->write || $this->outStream is resource);
  }

  <<__Override>>
  public function read(int $len): string {
    $available_bytes = Str\length($this->buffer) - $this->bufferIndex;
    if ($available_bytes < $len) {
      $bytes_to_read = $len - $available_bytes;
      $data = @PHP\fread(
        nullthrows($this->inStream),
        $bytes_to_read + $this->bufferSize,
      );
      if ($data === false || $data === '') {
        throw new TException('TFileStream: Could not read %d bytes', $len);
      }
      $result = Str\slice($this->buffer, $this->bufferIndex).
        Str\slice($data, 0, $bytes_to_read);
      $this->buffer = Str\slice($data, $bytes_to_read);
      $this->bufferIndex = 0;
    } else {
      if ($len === 1) {
        $result = $this->buffer[$this->bufferIndex];
      } else {
        $result = Str\slice($this->buffer, $this->bufferIndex, $len);
      }
      $this->bufferIndex += $len;
    }
    return $result;
  }

  <<__Override>>
  public function write(string $buf): void {
    if (Str\length($this->buffer) + Str\length($buf) > $this->bufferSize) {
      $written = @PHP\fwrite(nullthrows($this->outStream), $this->buffer);
      if ($written === 0 || $written === false) {
        throw new TException(
          'TPhpStream: Could not write %d bytes in buffer',
          Str\length($this->buffer),
        );
      }
      $written = @PHP\fwrite(nullthrows($this->outStream), $buf);
      if ($written === 0 || $written === false) {
        throw new TException(
          'TPhpStream: Could not write %d bytes in buffer',
          Str\length($buf),
        );
      }
      $this->buffer = '';
    } else {
      $this->buffer .= $buf;
    }
  }

  <<__Override>>
  public function flush(): void {
    if (Str\length($this->buffer) > 0) {
      $written = @PHP\fwrite(nullthrows($this->outStream), $this->buffer);
      if ($written === 0 || $written === false) {
        throw new TException(
          'TPhpStream: Could not write %d bytes in buffer',
          Str\length($this->buffer),
        );
      }
      $this->buffer = '';
    }
    PHP\fflush(nullthrows($this->outStream));
  }

  /**
   * Name of the transport (e.g.: socket).
   */
  <<__Override>>
  public function getTransportType(): string {
    return 'filestream';
  }
}
