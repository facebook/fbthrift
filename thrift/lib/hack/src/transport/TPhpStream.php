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
 * Php stream transport. Reads to and writes from the php standard streams
 * php://input and php://output
 *
 * @package thrift.transport
 */
class TPhpStream extends TTransport {

  const int MODE_R = 1;
  const int MODE_W = 2;

  private ?resource $inStream_ = null;

  private ?resource $outStream_ = null;

  private bool $read_ = false;

  private bool $write_ = false;

  /**
   * Specifies the maximum number of bytes to read
   * at once from internal stream.
   */
  private ?int $maxReadChunkSize_ = null;

  public function __construct(int $mode) {
    $this->read_ = (bool) ($mode & self::MODE_R);
    $this->write_ = (bool) ($mode & self::MODE_W);
  }

  /**
   * Sets the internal max read chunk size.
   * null for no limit (default).
   */
  public function setMaxReadChunkSize(int $maxReadChunkSize): void {
    $this->maxReadChunkSize_ = $maxReadChunkSize;
  }

  <<__Override>>
  public function open(): void {
    if ($this->read_) {
      $this->inStream_ = @PHP\fopen(self::inStreamName(), 'r');
        if (!($this->inStream_ is resource)) {
          throw new TException('TPhpStream: Could not open php://input');
        }
      }
    if ($this->write_) {
      $this->outStream_ = @PHP\fopen('php://output', 'w');
        if (!($this->outStream_ is resource)) {
          throw new TException('TPhpStream: Could not open php://output');
        }
      }
  }

  <<__Override>>
  public function close(): void {
    if ($this->read_) {
      @PHP\fclose(nullthrows($this->inStream_));
      $this->inStream_ = null;
    }
    if ($this->write_) {
      @PHP\fclose(nullthrows($this->outStream_));
      $this->outStream_ = null;
    }
  }

  <<__Override>>
  public function isOpen(): bool {
    return (!$this->read_ || $this->inStream_ is resource) &&
      (!$this->write_ || $this->outStream_ is resource);
  }

  <<__Override>>
  public function read(int $len): string {
    if ($this->maxReadChunkSize_ !== null) {
      $len = Math\minva($len, $this->maxReadChunkSize_);
    }

    $data = @PHP\fread(nullthrows($this->inStream_), $len);

    if ($data === false || $data === '') {
      throw new TException('TPhpStream: Could not read '.$len.' bytes');
    }
    return $data;
  }

  <<__Override>>
  public function write(string $buf): void {
    while (Str\length($buf) > 0) {
      $got = @PHP\fwrite(nullthrows($this->outStream_), $buf);

      if ($got === 0 || $got === false) {
        throw new TException(
          'TPhpStream: Could not write '.(string) Str\length($buf).' bytes',
        );
      }
      $buf = PHP\substr($buf, $got);
    }
  }

  <<__Override>>
  public function flush(): void {
    @PHP\fflush(nullthrows($this->outStream_));
  }

  private static function inStreamName(): string {
    if (PHP\php_sapi_name() == 'cli') {
      return 'php://stdin';
    }
    return 'php://input';
  }
}
