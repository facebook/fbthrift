<?hh // partial

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
 * Binary implementation of the Thrift protocol.
 */
abstract class TBinaryProtocolBase extends TProtocol {

  const VERSION_MASK = 0xffff0000;
  const VERSION_1 = 0x80010000;

  protected bool $strictRead_ = false;
  protected bool $strictWrite_ = true;
  protected bool $littleendian_ = false;
  protected $memory_limit = 128000000; //128M, the default
  protected $sequenceID = null;

  public function __construct(
    $trans,
    bool $strictRead = false,
    bool $strictWrite = true,
  ) {
    parent::__construct($trans);
    $this->strictRead_ = $strictRead;
    $this->strictWrite_ = $strictWrite;
    if (PHP\pack('S', 1) == "\x01\x00") {
      $this->littleendian_ = true;
    }
    $this->memory_limit = MemoryLimit::get();
  }

  <<__Override>>
  public function writeMessageBegin($name, $type, $seqid): num {
    if ($this->strictWrite_) {
      $version = self::VERSION_1 | $type;
      return
        $this->writeI32($version) +
        $this->writeString($name) +
        $this->writeI32($seqid);
    } else {
      return
        $this->writeString($name) +
        $this->writeByte($type) +
        $this->writeI32($seqid);
    }
  }

  <<__Override>>
  public function writeMessageEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeStructBegin($_name): int {
    return 0;
  }

  <<__Override>>
  public function writeStructEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeFieldBegin(
    $_field_name,
    int $field_type,
    int $field_id,
  ): int {
    return $this->writeByte($field_type) + $this->writeI16($field_id);
  }

  <<__Override>>
  public function writeFieldEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeFieldStop(): int {
    return $this->writeByte(TType::STOP);
  }

  <<__Override>>
  public function writeMapBegin(
    int $key_type,
    int $val_type,
    int $size,
  ): int {
    return
      $this->writeByte($key_type) +
      $this->writeByte($val_type) +
      $this->writeI32($size);
  }

  <<__Override>>
  public function writeMapEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeListBegin(int $elem_type, int $size): int {
    return $this->writeByte($elem_type) + $this->writeI32($size);
  }

  <<__Override>>
  public function writeListEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeSetBegin(int $elem_type, int $size): int {
    return $this->writeByte($elem_type) + $this->writeI32($size);
  }

  <<__Override>>
  public function writeSetEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeBool($value): int {
    $data = PHP\pack('c', $value ? 1 : 0);
    $this->trans_->write($data);
    return 1;
  }

  <<__Override>>
  public function writeByte(int $value): int {
    $this->trans_->write(PHP\chr($value));
    return 1;
  }

  <<__Override>>
  public function writeI16(int $value): int {
    $data = PHP\chr($value).PHP\chr($value >> 8);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $this->trans_->write($data);
    return 2;
  }

  <<__Override>>
  public function writeI32(int $value): int {
    $data = PHP\chr($value).PHP\chr($value >> 8).PHP\chr($value >> 16).PHP\chr($value >> 24);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $this->trans_->write($data);
    return 4;
  }

  <<__Override>>
  public function writeI64($value): int {
    // If we are on a 32bit architecture we have to explicitly deal with
    // 64-bit twos-complement arithmetic since PHP wants to treat all ints
    // as signed and any int over 2^31 - 1 as a float
    if (PHP_INT_SIZE === 4) {
      $neg = $value < 0;

      if ($neg) {
        $value *= -1;
      }

      $hi = (int) ($value / 4294967296);
      $lo = (int) $value;

      if ($neg) {
        $hi = ~$hi;
        $lo = ~$lo;
        if (($lo & (int) 0xffffffff) == (int) 0xffffffff) {
          $lo = 0;
          $hi++;
        } else {
          $lo++;
        }
      }
      $data = PHP\pack('N2', $hi, $lo);
    } else {
      $data =
        PHP\chr($value).
        PHP\chr($value >> 8).
        PHP\chr($value >> 16).
        PHP\chr($value >> 24).
        PHP\chr($value >> 32).
        PHP\chr($value >> 40).
        PHP\chr($value >> 48).
        PHP\chr($value >> 56);
      if ($this->littleendian_) {
        $data = PHP\strrev($data);
      }
    }

    $this->trans_->write($data);
    return 8;
  }

  <<__Override>>
  public function writeDouble($value): int {
    $data = PHP\pack('d', $value);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $this->trans_->write($data);
    return 8;
  }

  <<__Override>>
  public function writeFloat($value): int {
    $data = PHP\pack('f', $value);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $this->trans_->write($data);
    return 4;
  }

  <<__Override>>
  public function writeString(string $value): num {
    $len = Str\length($value);
    $result = $this->writeI32($len);
    if ($len) {
      $this->trans_->write($value);
    }
    return $result + $len;
  }

  private function unpackI32($data): int {
    $value =
      PHP\ord($data[3]) |
      (PHP\ord($data[2]) << 8) |
      (PHP\ord($data[1]) << 16) |
      (PHP\ord($data[0]) << 24);
    if ($value > 0x7fffffff) {
      $value = 0 - (($value - 1) ^ 0xffffffff);
    }
    return $value;
  }

  /**
   * Returns the sequence ID of the next message; only valid when called
   * before readMessageBegin()
   */
  public function peekSequenceID(): int {
    $trans = $this->trans_;
    if (!($trans is IThriftBufferedTransport)) {
      throw new TProtocolException(
        get_class($this->trans_).' does not support peek',
        TProtocolException::BAD_VERSION,
      );
    }
    if ($this->sequenceID !== null) {
      throw new TProtocolException(
        'peekSequenceID can only be called '.'before readMessageBegin',
        TProtocolException::INVALID_DATA,
      );
    }

    $data = $trans->peek(4);
    $sz = $this->unpackI32($data);
    $start = 4;

    if ($sz < 0) {
      $version = $sz & self::VERSION_MASK;
      if ($version != self::VERSION_1) {
        throw new TProtocolException(
          'Bad version identifier: '.$sz,
          TProtocolException::BAD_VERSION,
        );
      }
      // skip name string
      $data = $trans->peek(4, $start);
      $name_len = $this->unpackI32($data);
      $start += 4 + $name_len;
      // peek seqId
      $data = $trans->peek(4, $start);
      $seqid = $this->unpackI32($data);
    } else {
      if ($this->strictRead_) {
        throw new TProtocolException(
          'No version identifier, old protocol client?',
          TProtocolException::BAD_VERSION,
        );
      } else {
        // need to guard the length from mis-configured other type of TCP server
        // for example, if mis-configure sshd, will read 'SSH-' as the length
        // if memory limit is -1, means no limit.
        if ($this->memory_limit > 0 && $sz > $this->memory_limit) {
          throw new TProtocolException(
            'Length overflow: '.$sz,
            TProtocolException::SIZE_LIMIT,
          );
        }
        // Handle pre-versioned input
        $start += $sz;
        // skip type byte
        $start += 1;
        // peek seqId
        $data = $trans->peek(4, $start);
        $seqid = $this->unpackI32($data);
      }
    }
    return $seqid;
  }

  <<__Override>>
  public function readMessageBegin(
    inout $name,
    inout $type,
    inout $seqid,
  ): num {
    $sz = 0;
    $result = $this->readI32(inout $sz);
    if ($sz < 0) {
      $version = $sz & self::VERSION_MASK;
      if ($version != self::VERSION_1) {
        throw new TProtocolException(
          'Bad version identifier: '.$sz,
          TProtocolException::BAD_VERSION,
        );
      }
      $type = $sz & 0x000000ff;
      $name_res = $this->readString(inout $name);
      $seqid_res = $this->readI32(inout $seqid);
      $result += $name_res + $seqid_res;
    } else {
      if ($this->strictRead_) {
        throw new TProtocolException(
          'No version identifier, old protocol client?',
          TProtocolException::BAD_VERSION,
        );
      } else {
        // need to guard the length from mis-configured other type of TCP server
        // for example, if mis-configure sshd, will read 'SSH-' as the length
        // if memory limit is -1, means no limit.
        if ($this->memory_limit > 0 && $sz > $this->memory_limit) {
          throw new TProtocolException(
            'Length overflow: '.$sz,
            TProtocolException::SIZE_LIMIT,
          );
        }
        // Handle pre-versioned input
        $name = $this->trans_->readAll($sz);
        $type_res = $this->readByte(inout $type);
        $seqid_res = $this->readI32(inout $seqid);
        $result += $sz + $type_res + $seqid_res;
      }
    }
    $this->sequenceID = $seqid;
    return $result;
  }

  <<__Override>>
  public function readMessageEnd(): int {
    $this->sequenceID = null;
    return 0;
  }

  <<__Override>>
  public function readStructBegin(inout $name): int {
    $name = '';
    return 0;
  }

  <<__Override>>
  public function readStructEnd(): int {
    return 0;
  }

  <<__Override>>
  public function readFieldBegin(
    inout $_name,
    inout $field_type,
    inout $field_id,
  ): int {
    $result = $this->readByte(inout $field_type);
    if ($field_type === TType::STOP) {
      $field_id = 0;
      return $result;
    }
    $fid_res = $this->readI16(inout $field_id);
    $result += $fid_res;
    return $result;
  }

  <<__Override>>
  public function readFieldEnd(): int {
    return 0;
  }

  <<__Override>>
  public function readMapBegin(
    inout $key_type,
    inout $val_type,
    inout $size,
  ): int {
    $key_res = $this->readByte(inout $key_type);
    $val_res = $this->readByte(inout $val_type);
    $size_res = $this->readI32(inout $size);
    return $key_res + $val_res + $size_res;
  }

  <<__Override>>
  public function readMapEnd(): int {
    return 0;
  }

  <<__Override>>
  public function readListBegin(inout $elem_type, inout $size): int {
    $etype_res = $this->readByte(inout $elem_type);
    $size_res = $this->readI32(inout $size);
    return $etype_res + $size_res;
  }

  <<__Override>>
  public function readListEnd(): int {
    return 0;
  }

  <<__Override>>
  public function readSetBegin(inout $elem_type, inout $size): int {
    $etype_res = $this->readByte(inout $elem_type);
    $size_res = $this->readI32(inout $size);
    return $etype_res + $size_res;
  }

  <<__Override>>
  public function readSetEnd(): int {
    return 0;
  }

  <<__Override>>
  public function readBool(inout $value): int {
    $data = $this->trans_->readAll(1);
    $arr = PHP\unpack('c', $data);
    $value = $arr[1] == 1;
    return 1;
  }

  <<__Override>>
  public function readByte(inout $value): int {
    $data = $this->trans_->readAll(1);
    $value = PHP\ord($data);
    if ($value > 0x7f) {
      $value = 0 - (($value - 1) ^ 0xff);
    }
    return 1;
  }

  <<__Override>>
  public function readI16(inout $value): int {
    $data = $this->trans_->readAll(2);
    $value = PHP\ord($data[1]) | (PHP\ord($data[0]) << 8);
    if ($value > 0x7fff) {
      $value = 0 - (($value - 1) ^ 0xffff);
    }
    return 2;
  }

  <<__Override>>
  public function readI32(inout $value): int {
    $data = $this->trans_->readAll(4);
    $value = $this->unpackI32($data);
    return 4;
  }

  <<__Override>>
  public function readI64(inout $value): int {
    $data = $this->trans_->readAll(8);

    $arr = PHP\unpack('N2', $data);

    // If we are on a 32bit architecture we have to explicitly deal with
    // 64-bit twos-complement arithmetic since PHP wants to treat all ints
    // as signed and any int over 2^31 - 1 as a float
    if (PHP_INT_SIZE === 4) {
      $hi = $arr[1];
      $lo = $arr[2];
      $isNeg = $hi < 0;

      // Check for a negative
      if ($isNeg) {
        $hi = ~$hi & (int) 0xffffffff;
        $lo = ~$lo & (int) 0xffffffff;

        if ($lo == (int) 0xffffffff) {
          $hi++;
          $lo = 0;
        } else {
          $lo++;
        }
      }

      // Force 32bit words in excess of 2G to pe positive - we deal wigh sign
      // explicitly below

      if ($hi & (int) 0x80000000) {
        $hi &= (int) 0x7fffffff;
        $hi += 0x80000000;
      }

      if ($lo & (int) 0x80000000) {
        $lo &= (int) 0x7fffffff;
        $lo += 0x80000000;
      }

      $value = $hi * 4294967296 + $lo;

      if ($isNeg) {
        $value = 0 - $value;
      }
    } else {
      // Upcast negatives in LSB bit
      if ($arr[2] & 0x80000000) {
        $arr[2] = $arr[2] & 0xffffffff;
      }

      // Check for a negative
      if ($arr[1] & 0x80000000) {
        $arr[1] = $arr[1] & 0xffffffff;
        $arr[1] = $arr[1] ^ 0xffffffff;
        $arr[2] = $arr[2] ^ 0xffffffff;
        $value = 0 - $arr[1] * 4294967296 - $arr[2] - 1;
      } else {
        $value = $arr[1] * 4294967296 + $arr[2];
      }
    }

    return 8;
  }

  <<__Override>>
  public function readDouble(inout $value): int {
    $data = $this->trans_->readAll(8);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $arr = PHP\unpack('d', $data);
    $value = $arr[1];
    return 8;
  }

  <<__Override>>
  public function readFloat(inout $value): int {
    $data = $this->trans_->readAll(4);
    if ($this->littleendian_) {
      $data = PHP\strrev($data);
    }
    $arr = PHP\unpack('f', $data);
    $value = $arr[1];
    return 4;
  }

  <<__Override>>
  public function readString(inout $value): num {
    $len = 0;
    $result = $this->readI32(inout $len);
    if ($len) {
      $value = $this->trans_->readAll($len);
    } else {
      $value = '';
    }
    return $result + $len;
  }
}
