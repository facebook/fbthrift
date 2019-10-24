<?php
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
 * @package thrift.protocol.compact
 */

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolException.php';
abstract class TCompactProtocolBase extends TProtocol {
  const COMPACT_STOP = 0x00;
  const COMPACT_TRUE = 0x01;
  const COMPACT_FALSE = 0x02;
  const COMPACT_BYTE = 0x03;
  const COMPACT_I16 = 0x04;
  const COMPACT_I32 = 0x05;
  const COMPACT_I64 = 0x06;
  const COMPACT_DOUBLE = 0x07;
  const COMPACT_BINARY = 0x08;
  const COMPACT_LIST = 0x09;
  const COMPACT_SET = 0x0A;
  const COMPACT_MAP = 0x0B;
  const COMPACT_STRUCT = 0x0C;
  const COMPACT_FLOAT = 0x0D;
  const STATE_CLEAR = 0;
  const STATE_FIELD_WRITE = 1;
  const STATE_VALUE_WRITE = 2;
  const STATE_CONTAINER_WRITE = 3;
  const STATE_BOOL_WRITE = 4;
  const STATE_FIELD_READ = 5;
  const STATE_CONTAINER_READ = 6;
  const STATE_VALUE_READ = 7;
  const STATE_BOOL_READ = 8;
  const VERSION_MASK = 0x1f;
  const VERSION = 2;
  const VERSION_LOW = 1;
  const VERSION_DOUBLE_BE = 2;
  const PROTOCOL_ID = 0x82;
  const TYPE_MASK = 0xe0;
  const TYPE_SHIFT_AMOUNT = 5;
  const I16_MIN = -2 ** 15;
  const I16_MAX = 2 ** 15 - 1;
  const I32_MIN = -2 ** 31;
  const I32_MAX = 2 ** 31 - 1;
  protected static
    $ctypes = array(
      TType::STOP => self::COMPACT_STOP,
      TType::BOOL => self::COMPACT_TRUE,
      TType::BYTE => self::COMPACT_BYTE,
      TType::I16 => self::COMPACT_I16,
      TType::I32 => self::COMPACT_I32,
      TType::I64 => self::COMPACT_I64,
      TType::DOUBLE => self::COMPACT_DOUBLE,
      TType::FLOAT => self::COMPACT_FLOAT,
      TType::STRING => self::COMPACT_BINARY,
      TType::STRUCT => self::COMPACT_STRUCT,
      TType::LST => self::COMPACT_LIST,
      TType::SET => self::COMPACT_SET,
      TType::MAP => self::COMPACT_MAP
    );
  protected static
    $ttypes = array(
      self::COMPACT_STOP => TType::STOP,
      self::COMPACT_TRUE => TType::BOOL,
      self::COMPACT_FALSE => TType::BOOL,
      self::COMPACT_BYTE => TType::BYTE,
      self::COMPACT_I16 => TType::I16,
      self::COMPACT_I32 => TType::I32,
      self::COMPACT_I64 => TType::I64,
      self::COMPACT_DOUBLE => TType::DOUBLE,
      self::COMPACT_FLOAT => TType::FLOAT,
      self::COMPACT_BINARY => TType::STRING,
      self::COMPACT_STRUCT => TType::STRUCT,
      self::COMPACT_LIST => TType::LST,
      self::COMPACT_SET => TType::SET,
      self::COMPACT_MAP => TType::MAP
    );
  protected $state = self::STATE_CLEAR;
  protected $lastFid = 0;
  protected $boolFid = null;
  protected $boolValue = null;
  protected $structs = array();
  protected $containers = array();
  protected $version = self::VERSION;
  public function setWriteVersion($ver) {
    $this->version = $ver;
  }
  public function toZigZag($n, $bits) {
    return ($n << 1) ^ ($n >> ($bits - 1));
  }
  public function fromZigZag($n) {
    return ($n >> 1) ^ (-($n & 1));
  }
  public function checkRange($value, $min, $max) {
    if ($value < $min || $value > $max) {
      throw new TProtocolException("Value is out of range");
    }
  }
  public function getVarint($data) {
    $out = "";
    while (true) {
      if (($data & (~0x7f)) === 0) {
        $out .= chr($data);
        break;
      } else {
        $out .= chr(($data & 0xff) | 0x80);
        $data = $data >> 7;
      }
    }
    return $out;
  }
  public function writeVarint($data) {
    $out = $this->getVarint($data);
    $this->trans_->write($out);
    return strlen($out);
  }
  public function readVarint(&$result) {
    $idx = 0;
    $shift = 0;
    $result = 0;
    while (true) {
      $x = $this->trans_->readAll(1);
      $byte = ord($x);
      $idx += 1;
      $result |= ($byte & 0x7f) << $shift;
      if (($byte >> 7) === 0) {
        return $idx;
      }
      $shift += 7;
    }
    return $idx;
  }
  public function __construct($trans) {
    parent::__construct($trans);
  }
  public function writeMessageBegin($name, $type, $seqid) {
    $written =
      $this->writeUByte(self::PROTOCOL_ID) +
      $this->writeUByte($this->version | ($type << self::TYPE_SHIFT_AMOUNT)) +
      $this->writeVarint($seqid) +
      $this->writeString($name);
    $this->state = self::STATE_VALUE_WRITE;
    return $written;
  }
  public function writeMessageEnd() {
    $this->state = self::STATE_CLEAR;
    return 0;
  }
  public function writeStructBegin($name) {
    $this->structs[] = array($this->state, $this->lastFid);
    $this->state = self::STATE_FIELD_WRITE;
    $this->lastFid = 0;
    return 0;
  }
  public function writeStructEnd() {
    $old_values = array_pop($this->structs);
    $this->state = $old_values[0];
    $this->lastFid = $old_values[1];
    return 0;
  }
  public function writeFieldStop() {
    return $this->writeByte(0);
  }
  public function writeFieldHeader($type, $fid) {
    $written = 0;
    $delta = $fid - $this->lastFid;
    if ((0 < $delta) && ($delta <= 15)) {
      $written = $this->writeUByte(($delta << 4) | $type);
    } else {
      $written = $this->writeByte($type) + $this->writeI16($fid);
    }
    $this->lastFid = $fid;
    return $written;
  }
  public function writeFieldBegin($field_name, $field_type, $field_id) {
    if (\hacklib_equals($field_type, TType::BOOL)) {
      $this->state = self::STATE_BOOL_WRITE;
      $this->boolFid = $field_id;
      return 0;
    } else {
      $this->state = self::STATE_VALUE_WRITE;
      return $this->writeFieldHeader(self::$ctypes[$field_type], $field_id);
    }
  }
  public function writeFieldEnd() {
    $this->state = self::STATE_FIELD_WRITE;
    return 0;
  }
  public function writeCollectionBegin($etype, $size) {
    $written = 0;
    if ($size <= 14) {
      $written = $this->writeUByte(($size << 4) | self::$ctypes[$etype]);
    } else {
      $written =
        $this->writeUByte(0xf0 | self::$ctypes[$etype]) +
        $this->writeVarint($size);
    }
    $this->containers[] = $this->state;
    $this->state = self::STATE_CONTAINER_WRITE;
    return $written;
  }
  public function writeMapBegin($key_type, $val_type, $size) {
    $written = 0;
    if (\hacklib_equals($size, 0)) {
      $written = $this->writeByte(0);
    } else {
      $written =
        $this->writeVarint($size) +
        $this->writeUByte(
          (self::$ctypes[$key_type] << 4) | self::$ctypes[$val_type]
        );
    }
    $this->containers[] = $this->state;
    $this->state = self::STATE_CONTAINER_WRITE;
    return $written;
  }
  public function writeCollectionEnd() {
    $this->state = array_pop($this->containers);
    return 0;
  }
  public function writeMapEnd() {
    return $this->writeCollectionEnd();
  }
  public function writeListBegin($elem_type, $size) {
    return $this->writeCollectionBegin($elem_type, $size);
  }
  public function writeListEnd() {
    return $this->writeCollectionEnd();
  }
  public function writeSetBegin($elem_type, $size) {
    return $this->writeCollectionBegin($elem_type, $size);
  }
  public function writeSetEnd() {
    return $this->writeCollectionEnd();
  }
  public function writeBool($value) {
    if (\hacklib_equals($this->state, self::STATE_BOOL_WRITE)) {
      $ctype = self::COMPACT_FALSE;
      if (\hacklib_cast_as_boolean($value)) {
        $ctype = self::COMPACT_TRUE;
      }
      return $this->writeFieldHeader($ctype, $this->boolFid);
    } else {
      if (\hacklib_equals($this->state, self::STATE_CONTAINER_WRITE)) {
        return $this->writeByte(
          \hacklib_cast_as_boolean($value)
            ? self::COMPACT_TRUE
            : self::COMPACT_FALSE
        );
      } else {
        throw new TProtocolException("Invalid state in compact protocol");
      }
    }
  }
  public function writeByte($value) {
    $this->trans_->write(chr($value));
    return 1;
  }
  public function writeUByte($byte) {
    $this->trans_->write(chr($byte));
    return 1;
  }
  public function writeI16($value) {
    $this->checkRange($value, self::I16_MIN, self::I16_MAX);
    $thing = $this->toZigZag($value, 16);
    return $this->writeVarint($thing);
  }
  public function writeI32($value) {
    $this->checkRange($value, self::I32_MIN, self::I32_MAX);
    $thing = $this->toZigZag($value, 32);
    return $this->writeVarint($thing);
  }
  public function writeDouble($value) {
    $data = pack("d", $value);
    if ($this->version >= self::VERSION_DOUBLE_BE) {
      $data = strrev($data);
    }
    $this->trans_->write($data);
    return 8;
  }
  public function writeFloat($value) {
    $data = pack("f", $value);
    $data = strrev($data);
    $this->trans_->write($data);
    return 4;
  }
  public function writeString($value) {
    $value = (string) $value;
    $len = strlen($value);
    $result = $this->writeVarint($len);
    if (\hacklib_cast_as_boolean($len)) {
      $this->trans_->write($value);
    }
    return $result + $len;
  }
  public function readFieldBegin(&$name, &$field_type, &$field_id) {
    $result = $this->readUByte($field_type);
    $delta = $field_type >> 4;
    $field_type = $field_type & 0x0f;
    if (\hacklib_equals($field_type, self::COMPACT_STOP)) {
      $field_id = 0;
      $field_type = $this->getTType($field_type);
      return $result;
    }
    if (\hacklib_equals($delta, 0)) {
      $result += $this->readI16($field_id);
    } else {
      $field_id = $this->lastFid + $delta;
    }
    $this->lastFid = $field_id;
    if (\hacklib_equals($field_type, self::COMPACT_TRUE)) {
      $this->state = self::STATE_BOOL_READ;
      $this->boolValue = true;
    } else {
      if (\hacklib_equals($field_type, self::COMPACT_FALSE)) {
        $this->state = self::STATE_BOOL_READ;
        $this->boolValue = false;
      } else {
        $this->state = self::STATE_VALUE_READ;
      }
    }
    $field_type = $this->getTType($field_type);
    return $result;
  }
  public function readFieldEnd() {
    $this->state = self::STATE_FIELD_READ;
    return 0;
  }
  public function readUByte(&$value) {
    $data = $this->trans_->readAll(1);
    $value = ord($data);
    return 1;
  }
  public function readByte(&$value) {
    $data = $this->trans_->readAll(1);
    $value = ord($data);
    if ($value > 0x7f) {
      $value = 0 - (($value - 1) ^ 0xff);
    }
    return 1;
  }
  public function readZigZag(&$value) {
    $result = $this->readVarint($value);
    $value = $this->fromZigZag($value);
    return $result;
  }
  public function readMessageBegin(&$name, &$type, &$seqid) {
    $protoId = 0;
    $result = $this->readUByte($protoId);
    if (\hacklib_not_equals($protoId, self::PROTOCOL_ID)) {
      throw new TProtocolException("Bad protocol id in TCompact message");
    }
    $verType = 0;
    $result += $this->readUByte($verType);
    $type = ($verType & self::TYPE_MASK) >> self::TYPE_SHIFT_AMOUNT;
    $this->version = $verType & self::VERSION_MASK;
    if (!(($this->version <= self::VERSION) &&
          ($this->version >= self::VERSION_LOW))) {
      throw new TProtocolException("Bad version in TCompact message");
    }
    $result += $this->readVarint($seqid);
    $result += $this->readString($name);
    return $result;
  }
  public function readMessageEnd() {
    return 0;
  }
  public function readStructBegin(&$name) {
    $name = "";
    $this->structs[] = array($this->state, $this->lastFid);
    $this->state = self::STATE_FIELD_READ;
    $this->lastFid = 0;
    return 0;
  }
  public function readStructEnd() {
    $last = array_pop($this->structs);
    $this->state = $last[0];
    $this->lastFid = $last[1];
    return 0;
  }
  public function readCollectionBegin(&$type, &$size) {
    $sizeType = 0;
    $result = $this->readUByte($sizeType);
    $size = $sizeType >> 4;
    $type = $this->getTType($sizeType);
    if (\hacklib_equals($size, 15)) {
      $result += $this->readVarint($size);
    }
    $this->containers[] = $this->state;
    $this->state = self::STATE_CONTAINER_READ;
    return $result;
  }
  public function readMapBegin(&$key_type, &$val_type, &$size) {
    $result = $this->readVarint($size);
    $types = 0;
    if ($size > 0) {
      $result += $this->readUByte($types);
    }
    $val_type = $this->getTType($types);
    $key_type = $this->getTType($types >> 4);
    $this->containers[] = $this->state;
    $this->state = self::STATE_CONTAINER_READ;
    return $result;
  }
  public function readCollectionEnd() {
    $this->state = array_pop($this->containers);
    return 0;
  }
  public function readMapEnd() {
    return $this->readCollectionEnd();
  }
  public function readListBegin(&$elem_type, &$size) {
    return $this->readCollectionBegin($elem_type, $size);
  }
  public function readListEnd() {
    return $this->readCollectionEnd();
  }
  public function readSetBegin(&$elem_type, &$size) {
    return $this->readCollectionBegin($elem_type, $size);
  }
  public function readSetEnd() {
    return $this->readCollectionEnd();
  }
  public function readBool(&$value) {
    if (\hacklib_equals($this->state, self::STATE_BOOL_READ)) {
      $value = $this->boolValue;
      return 0;
    } else {
      if (\hacklib_equals($this->state, self::STATE_CONTAINER_READ)) {
        $result = $this->readByte($value);
        $value = \hacklib_equals($value, self::COMPACT_TRUE);
        return $result;
      } else {
        throw new TProtocolException("Invalid state in compact protocol");
      }
    }
  }
  public function readI16(&$value) {
    return $this->readZigZag($value);
  }
  public function readI32(&$value) {
    return $this->readZigZag($value);
  }
  public function readDouble(&$value) {
    $data = $this->trans_->readAll(8);
    if ($this->version >= self::VERSION_DOUBLE_BE) {
      $data = strrev($data);
    }
    $arr = unpack("d", $data);
    $value = $arr[1];
    return 8;
  }
  public function readFloat(&$value) {
    $data = $this->trans_->readAll(4);
    $data = strrev($data);
    $arr = unpack("f", $data);
    $value = $arr[1];
    return 4;
  }
  public function readString(&$value) {
    $len = 0;
    $result = $this->readVarint($len);
    if (\hacklib_cast_as_boolean($len)) {
      $value = $this->trans_->readAll($len);
    } else {
      $value = "";
    }
    return $result + $len;
  }
  public function getTType($byte) {
    return self::$ttypes[$byte & 0x0f];
  }
  public function readI64(&$value) {
    $hi = 0;
    $lo = 0;
    $idx = 0;
    $shift = 0;
    $arr = array();
    while (true) {
      $x = $this->trans_->readAll(1);
      $byte = ord($x);
      $idx += 1;
      if ($shift < 32) {
        $lo |= (($byte & 0x7f) << $shift) & 0x00000000ffffffff;
      }
      if ($shift >= 32) {
        $hi |= ($byte & 0x7f) << ($shift - 32);
      } else {
        if ($shift > 24) {
          $hi |= ($byte & 0x7f) >> ($shift - 24);
        }
      }
      if (($byte >> 7) === 0) {
        break;
      }
      $shift += 7;
    }
    $xorer = 0;
    if ($lo & 1) {
      $xorer = 0xffffffff;
    }
    $lo = ($lo >> 1) & 0x7fffffff;
    $lo = $lo | (($hi & 1) << 31);
    $hi = ($hi >> 1) ^ $xorer;
    $lo = $lo ^ $xorer;
    if (true) {
      $isNeg = $hi < 0;
      if (\hacklib_cast_as_boolean($isNeg)) {
        $hi = (~$hi) & ((int) 0xffffffff);
        $lo = (~$lo) & ((int) 0xffffffff);
        if (\hacklib_equals($lo, (int) 0xffffffff)) {
          $hi++;
          $lo = 0;
        } else {
          $lo++;
        }
      }
      if ($hi & ((int) 0x80000000)) {
        $hi &= (int) 0x7fffffff;
        $hi += 0x80000000;
      }
      if ($lo & ((int) 0x80000000)) {
        $lo &= (int) 0x7fffffff;
        $lo += 0x80000000;
      }
      $value = ($hi * 4294967296) + $lo;
      if (\hacklib_cast_as_boolean($isNeg)) {
        $value = 0 - $value;
      }
    } else {
      if ($arr[2] & 0x80000000) {
        $arr[2] = $arr[2] & 0xffffffff;
      }
      if ($arr[1] & 0x80000000) {
        $arr[1] = $arr[1] & 0xffffffff;
        $arr[1] = $arr[1] ^ 0xffffffff;
        $arr[2] = $arr[2] ^ 0xffffffff;
        $value = ((0 - ($arr[1] * 4294967296)) - $arr[2]) - 1;
      } else {
        $value = ($arr[1] * 4294967296) + $arr[2];
      }
    }
    return $idx;
  }
  public function writeI64($value) {
    if (($value > 4294967296) || ($value < (-4294967296))) {
      $neg = $value < 0;
      if (\hacklib_cast_as_boolean($neg)) {
        $value *= -1;
      }
      $hi = ((int) $value) >> 32;
      $lo = ((int) $value) & 0xffffffff;
      if (\hacklib_cast_as_boolean($neg)) {
        $hi = ~$hi;
        $lo = ~$lo;
        if (($lo & ((int) 0xffffffff)) == ((int) 0xffffffff)) {
          $lo = 0;
          $hi++;
        } else {
          $lo++;
        }
      }
      $xorer = 0;
      if (\hacklib_cast_as_boolean($neg)) {
        $xorer = 0xffffffff;
      }
      $lowbit = ($lo >> 31) & 1;
      $hi = ($hi << 1) | $lowbit;
      $lo = $lo << 1;
      $lo = ($lo ^ $xorer) & 0xffffffff;
      $hi = ($hi ^ $xorer) & 0xffffffff;
      $out = "";
      while (true) {
        if ((($lo & (~0x7f)) === 0) && ($hi === 0)) {
          $out .= chr($lo);
          break;
        } else {
          $out .= chr(($lo & 0xff) | 0x80);
          $lo = $lo >> 7;
          $lo = $lo | ($hi << 25);
          $hi = $hi >> 7;
          $hi = $hi & (127 << 25);
        }
      }
      $this->trans_->write($out);
      return strlen($out);
    } else {
      return $this->writeVarint($this->toZigZag($value, 64));
    }
  }
}
