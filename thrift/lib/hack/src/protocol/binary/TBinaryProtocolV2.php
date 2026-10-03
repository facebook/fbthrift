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
 * Pure-Hack binary protocol, byte-for-byte compatible with
 * TBinaryProtocolUnaccelerated.
 *
 * Assumes 64-bit ints: unlike TBinaryProtocolBase, it doesn't support 32-bit
 * PHP (no hi/lo split for i64, and decodeI32 sign-extends through a 64-bit
 * int).
 */
// @oss-disable: <<Oncalls('thrift')>>
final class TBinaryProtocolV2 extends TBinaryProtocolBase {

  <<__Override>>
  public function writeFieldBegin(
    string $_field_name,
    TType $field_type,
    int $field_id,
  )[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('cn', $field_type, $field_id));
    return 3;
  }

  <<__Override>>
  public function writeMapBegin(
    TType $key_type,
    TType $val_type,
    int $size,
  )[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('ccN', $key_type, $val_type, $size));
    return 6;
  }

  <<__Override>>
  public function writeListBegin(
    TType $elem_type,
    int $size,
  )[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('cN', $elem_type, $size));
    return 5;
  }

  <<__Override>>
  public function writeSetBegin(
    TType $elem_type,
    int $size,
  )[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('cN', $elem_type, $size));
    return 5;
  }

  <<__Override>>
  public function writeBool(bool $value)[zoned_shallow]: int {
    $this->trans_->write($value ? "\x01" : "\x00");
    return 1;
  }

  <<__Override>>
  public function writeI16(int $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('n', $value));
    return 2;
  }

  <<__Override>>
  public function writeI32(int $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('N', $value));
    return 4;
  }

  <<__Override>>
  public function writeI64(?int $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('J', $value ?? 0));
    return 8;
  }

  <<__Override>>
  public function writeDouble(float $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('E', $value));
    return 8;
  }

  <<__Override>>
  public function writeFloat(float $value)[zoned_shallow]: int {
    $this->trans_->write(PHP\pack('G', $value));
    return 4;
  }

  <<__Override>>
  public function writeString(string $value)[zoned_shallow]: int {
    $len = Str\length($value);
    $this->trans_->write(PHP\pack('N', $len));
    if ($len !== 0) {
      $this->trans_->write($value);
    }
    return 4 + $len;
  }

  // ord() arithmetic instead of unpack() for 2 and 4 bytes: unpack's format
  // parsing and result dict made i16 decoding 4x slower.
  private static function decodeI16(string $data, int $offset)[]: int {
    $value = (PHP\ord($data[$offset]) << 8) | PHP\ord($data[$offset + 1]);
    return $value > 0x7fff ? $value - 0x10000 : $value;
  }

  private static function decodeI32(string $data, int $offset)[]: int {
    $value = (PHP\ord($data[$offset]) << 24) |
      (PHP\ord($data[$offset + 1]) << 16) |
      (PHP\ord($data[$offset + 2]) << 8) |
      PHP\ord($data[$offset + 3]);
    return $value > 0x7fffffff ? $value - 0x100000000 : $value;
  }

  <<__Override>>
  public function readFieldBegin(
    inout ?string $_name,
    inout ?TType $field_type,
    inout ?int $field_id,
  )[zoned_shallow]: int {
    $field_type = TType::assert(PHP\ord($this->trans_->readAll(1)));
    if ($field_type === TType::STOP) {
      $field_id = 0;
      return 1;
    }
    $field_id = self::decodeI16($this->trans_->readAll(2), 0);
    return 3;
  }

  <<__Override>>
  public function readMapBegin(
    inout ?TType $key_type,
    inout ?TType $val_type,
    inout ?int $size,
  )[zoned_shallow]: int {
    $header = $this->trans_->readAll(6);
    $key_type = TType::assert(PHP\ord($header[0]));
    $val_type = TType::assert(PHP\ord($header[1]));
    $size = self::decodeI32($header, 2);
    return 6;
  }

  <<__Override>>
  public function readListBegin(
    inout ?TType $elem_type,
    inout ?int $size,
  )[zoned_shallow]: int {
    $header = $this->trans_->readAll(5);
    $elem_type = TType::assert(PHP\ord($header[0]));
    $size = self::decodeI32($header, 1);
    return 5;
  }

  <<__Override>>
  public function readSetBegin(
    inout ?TType $elem_type,
    inout ?int $size,
  )[zoned_shallow]: int {
    $header = $this->trans_->readAll(5);
    $elem_type = TType::assert(PHP\ord($header[0]));
    $size = self::decodeI32($header, 1);
    return 5;
  }

  <<__Override>>
  public function readBool(inout bool $value)[zoned_shallow]: int {
    $value = PHP\ord($this->trans_->readAll(1)) === 1;
    return 1;
  }

  <<__Override>>
  public function readI16(inout int $value)[zoned_shallow]: int {
    $value = self::decodeI16($this->trans_->readAll(2), 0);
    return 2;
  }

  <<__Override>>
  public function readI32(inout int $value)[zoned_shallow]: int {
    $value = self::decodeI32($this->trans_->readAll(4), 0);
    return 4;
  }
  // Assumes 64-bit ints: unlike TBinaryProtocolBase, it doesn't support 32-bit
  // PHP - no hi/lo split for i64
  <<__Override>>
  public function readI64(inout int $value)[zoned_shallow]: int {
    $value = PHP\unpack('J', $this->trans_->readAll(8))[1] as int;
    return 8;
  }

  <<__Override>>
  public function readDouble(inout float $value)[zoned_shallow]: int {
    $value = PHP\unpack('E', $this->trans_->readAll(8))[1] as float;
    return 8;
  }

  <<__Override>>
  public function readFloat(inout float $value)[zoned_shallow]: int {
    $value = PHP\unpack('G', $this->trans_->readAll(4))[1] as float;
    return 4;
  }

  <<__Override>>
  public function readString(inout string $value)[zoned_shallow]: int {
    $len = self::decodeI32($this->trans_->readAll(4), 0);
    if ($len !== 0) {
      $value = $this->trans_->readAll($len);
    } else {
      $value = '';
    }
    return 4 + $len;
  }
}
