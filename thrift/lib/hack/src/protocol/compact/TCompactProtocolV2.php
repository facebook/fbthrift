<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
 */

use namespace FlibSL\{C, Math, Str, Vec}; // @oss-enable

/**
 * Pure-Hack compact protocol, byte-for-byte compatible with
 * TCompactProtocolUnaccelerated. One decode differs: an i64 varint with bits
 * set beyond the 64th decodes as the native extension decodes it, where the
 * base protocol flips its sign.
 */
// @oss-disable: <<Oncalls('thrift')>>
final class TCompactProtocolV2 extends TCompactProtocolBase {

  // (state, lastFid) pairs for open structs and containers. Slots are reused
  // rather than popped, so struct boundaries stop allocating once the stack
  // has grown. Containers never move lastFid, so one stack covers both. The
  // base class's stacks are unused: every begin/end method must be overridden.
  // Apart from frame handling, those overrides match the base class.
  private vec<int> $frames = vec[];
  private int $sp = 0;

  private function pushFrame()[write_props]: void {
    $sp = $this->sp;
    if ($sp === C\count($this->frames)) {
      $this->frames[] = $this->state;
      $this->frames[] = $this->lastFid;
    } else {
      $this->frames[$sp] = $this->state;
      $this->frames[$sp + 1] = $this->lastFid;
    }
    $this->sp = $sp + 2;
  }

  private function popFrame()[write_props]: void {
    $sp = $this->sp - 2;
    $this->sp = $sp;
    $this->state = $this->frames[$sp];
    $this->lastFid = $this->frames[$sp + 1];
  }

  // Folds the 7-bit groups and their continuation bits into one little-endian
  // integer and packs it once, instead of building the string a byte at a
  // time. Negative values encode as their 64-bit two's complement (10 bytes),
  // which requires the masked (logical) shift below.
  <<__Override>>
  public function getVarint(int $data)[]: string {
    if (($data & ~0x7f) === 0) {
      return PHP\chr($data);
    }
    $word = 0;
    $shift = 0;
    while (($data & ~0x7f) !== 0 && $shift < 64) {
      $word |= (($data & 0x7f) | 0x80) << $shift;
      $data = ($data >> 7) & 0x01ffffffffffffff;
      $shift += 8;
    }
    if ($shift === 64) {
      // 9 or 10 bytes: the first 8 fill the word.
      return PHP\pack('P', $word).$this->getVarint($data);
    }
    $word |= $data << $shift;
    switch ($shift) {
      case 8:
        return PHP\pack('v', $word);
      case 24:
        return PHP\pack('V', $word);
      case 56:
        return PHP\pack('P', $word);
      default:
        return Str\slice(PHP\pack('P', $word), 0, ($shift >> 3) + 1);
    }
  }

  // Most varints are one byte; return before setting up the loop.
  <<__Override>>
  public function readVarint(inout int $result)[zoned_shallow]: int {
    $result = PHP\ord($this->trans_->readAll(1));
    if ($result < 0x80) {
      return 1;
    }
    $result &= 0x7f;
    $idx = 1;
    $shift = 7;
    while (true) {
      $byte = PHP\ord($this->trans_->readAll(1));
      $idx += 1;
      $result |= ($byte & 0x7f) << $shift;
      if (($byte >> 7) === 0) {
        return $idx;
      }
      $shift += 7;
    }
  }

  <<__Override>>
  public function writeStructBegin(string $_name)[write_props]: int {
    $this->pushFrame();
    $this->state = self::STATE_FIELD_WRITE;
    $this->lastFid = 0;
    return 0;
  }

  <<__Override>>
  public function writeStructEnd()[write_props]: int {
    $this->popFrame();
    return 0;
  }

  <<__Override>>
  public function writeCollectionBegin(
    TType $etype,
    int $size,
  )[zoned_shallow]: int {
    if ($size <= 14) {
      $header = PHP\chr($size << 4 | self::CTYPES[$etype]);
    } else {
      $header = PHP\chr(0xf0 | self::CTYPES[$etype]).$this->getVarint($size);
    }
    $this->trans_->write($header);
    $this->pushFrame();
    $this->state = self::STATE_CONTAINER_WRITE;
    return Str\length($header);
  }

  <<__Override>>
  public function writeMapBegin(
    TType $key_type,
    TType $val_type,
    int $size,
  )[zoned_shallow]: int {
    if ($size === 0) {
      $header = "\x00";
    } else {
      $header = $this->getVarint($size).
        PHP\chr(self::CTYPES[$key_type] << 4 | self::CTYPES[$val_type]);
    }
    $this->trans_->write($header);
    $this->pushFrame();
    $this->state = self::STATE_CONTAINER_WRITE;
    return Str\length($header);
  }

  <<__Override>>
  public function writeCollectionEnd()[write_props]: int {
    $this->popFrame();
    return 0;
  }

  <<__Override>>
  public function writeI64(int $value)[zoned_shallow]: int {
    return $this->writeVarint($this->toZigZag($value, 64));
  }

  <<__Override>>
  public function writeDouble(float $value)[zoned_shallow]: int {
    $format = $this->version >= self::VERSION_DOUBLE_BE ? 'E' : 'e';
    $this->trans_->write(PHP\pack($format, $value));
    return 8;
  }

  <<__Override>>
  public function writeFloat(float $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('G', $value));
    return 4;
  }

  <<__Override>>
  public function readStructBegin(inout ?string $name)[write_props]: int {
    $name = '';
    $this->pushFrame();
    $this->state = self::STATE_FIELD_READ;
    $this->lastFid = 0;
    return 0;
  }

  <<__Override>>
  public function readStructEnd()[write_props]: int {
    $this->popFrame();
    return 0;
  }

  <<__Override>>
  public function readCollectionBegin(
    inout ?TType $type,
    inout ?int $size,
  )[zoned_shallow]: int {
    $size_type = PHP\ord($this->trans_->readAll(1));
    $result = 1;
    $size = $size_type >> 4;
    $type = $this->getTType($size_type);
    if ($size === 15) {
      $result += $this->readVarint(inout $size);
    }
    $this->pushFrame();
    $this->state = self::STATE_CONTAINER_READ;
    return $result;
  }

  <<__Override>>
  public function readMapBegin(
    inout ?TType $key_type,
    inout ?TType $val_type,
    inout ?int $size,
  )[zoned_shallow]: int {
    $size_nonnull = -1;
    $result = $this->readVarint(inout $size_nonnull);
    $size = $size_nonnull;
    $types = 0;
    if ($size > 0) {
      $types = PHP\ord($this->trans_->readAll(1));
      $result += 1;
    }
    $val_type = $this->getTType($types);
    $key_type = $this->getTType($types >> 4);
    $this->pushFrame();
    $this->state = self::STATE_CONTAINER_READ;
    return $result;
  }

  <<__Override>>
  public function readCollectionEnd()[write_props]: int {
    $this->popFrame();
    return 0;
  }

  <<__Override>>
  public function readI64(inout int $value)[zoned_shallow]: int {
    $result = $this->readVarint(inout $value);
    // Logical shift: an i64's zig-zag form can have its top bit set, which an
    // arithmetic shift would copy down.
    $value = (($value >> 1) & 0x7fffffffffffffff) ^ -($value & 1);
    return $result;
  }

  <<__Override>>
  public function readDouble(inout float $value)[zoned_shallow]: int {
    $format = $this->version >= self::VERSION_DOUBLE_BE ? 'E' : 'e';
    $value = PHP\unpack($format, $this->trans_->readAll(8))[1] as float;
    return 8;
  }

  <<__Override>>
  public function readFloat(inout float $value)[zoned_shallow]: int {
    $value = PHP\unpack('G', $this->trans_->readAll(4))[1] as float;
    return 4;
  }
}
