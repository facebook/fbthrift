<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.binary
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
abstract class TBinaryProtocolBase extends TProtocol {
  const VERSION_MASK = 0xffff0000;
  const VERSION_1 = 0x80010000;
  protected $strictRead_ = false;
  protected $strictWrite_ = true;
  protected $littleendian_ = false;
  protected $memory_limit = 128000000;
  protected $sequenceID = null;
  public function __construct(
    $trans,
    $strictRead = false,
    $strictWrite = true
  ) {
    parent::__construct($trans);
    $this->strictRead_ = $strictRead;
    $this->strictWrite_ = $strictWrite;
    if (\hacklib_equals(pack("S", 1), "\001\000")) {
      $this->littleendian_ = true;
    }
    $this->memory_limit = self::getBytes(ini_get("memory_limit"));
  }
  public static function getBytes($notation) {
    $val = trim($notation);
    $last = strtolower($val[strlen($val) - 1]);
    switch ($last) {
      case "g":
        $val *= 1024; // FALLTHROUGH
      case "m":
        $val *= 1024; // FALLTHROUGH
      case "k":
        $val *= 1024;
    }
    return $val;
  }
  public function writeMessageBegin($name, $type, $seqid) {
    if (\hacklib_cast_as_boolean($this->strictWrite_)) {
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
  public function writeMessageEnd() {
    return 0;
  }
  public function writeStructBegin($name) {
    return 0;
  }
  public function writeStructEnd() {
    return 0;
  }
  public function writeFieldBegin($fieldName, $fieldType, $fieldId) {
    return $this->writeByte($fieldType) + $this->writeI16($fieldId);
  }
  public function writeFieldEnd() {
    return 0;
  }
  public function writeFieldStop() {
    return $this->writeByte(TType::STOP);
  }
  public function writeMapBegin($keyType, $valType, $size) {
    return
      $this->writeByte($keyType) +
      $this->writeByte($valType) +
      $this->writeI32($size);
  }
  public function writeMapEnd() {
    return 0;
  }
  public function writeListBegin($elemType, $size) {
    return $this->writeByte($elemType) + $this->writeI32($size);
  }
  public function writeListEnd() {
    return 0;
  }
  public function writeSetBegin($elemType, $size) {
    return $this->writeByte($elemType) + $this->writeI32($size);
  }
  public function writeSetEnd() {
    return 0;
  }
  public function writeBool($value) {
    $data = pack("c", \hacklib_cast_as_boolean($value) ? 1 : 0);
    $this->trans_->write($data);
    return 1;
  }
  public function writeByte($value) {
    $this->trans_->write(chr($value));
    return 1;
  }
  public function writeI16($value) {
    $data = chr($value).chr($value >> 8);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $this->trans_->write($data);
    return 2;
  }
  public function writeI32($value) {
    $data = chr($value).chr($value >> 8).chr($value >> 16).chr($value >> 24);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $this->trans_->write($data);
    return 4;
  }
  public function writeI64($value) {
    $data = "";
    if (PHP_INT_SIZE == 4) {
      $neg = $value < 0;
      if (\hacklib_cast_as_boolean($neg)) {
        $value *= -1;
      }
      $hi = (int) ($value / 4294967296);
      $lo = (int) $value;
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
      $data = pack("N2", $hi, $lo);
    } else {
      $data =
        chr($value).
        chr($value >> 8).
        chr($value >> 16).
        chr($value >> 24).
        chr($value >> 32).
        chr($value >> 40).
        chr($value >> 48).
        chr($value >> 56);
      if (\hacklib_cast_as_boolean($this->littleendian_)) {
        $data = strrev($data);
      }
    }
    $this->trans_->write($data);
    return 8;
  }
  public function writeDouble($value) {
    $data = pack("d", $value);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $this->trans_->write($data);
    return 8;
  }
  public function writeFloat($value) {
    $data = pack("f", $value);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $this->trans_->write($data);
    return 4;
  }
  public function writeString($value) {
    $len = strlen($value);
    $result = $this->writeI32($len);
    if (\hacklib_cast_as_boolean($len)) {
      $this->trans_->write($value);
    }
    return $result + $len;
  }
  private function unpackI32($data) {
    $value =
      ord($data[3]) |
      (ord($data[2]) << 8) |
      (ord($data[1]) << 16) |
      (ord($data[0]) << 24);
    if ($value > 0x7fffffff) {
      $value = 0 - (($value - 1) ^ 0xffffffff);
    }
    return $value;
  }
  public function peekSequenceID() {
    $trans = $this->trans_;
    if (!($trans instanceof IThriftBufferedTransport)) {
      throw new TProtocolException(
        get_class($this->trans_)." does not support peek",
        TProtocolException::BAD_VERSION
      );
    }
    if ($this->sequenceID !== null) {
      throw new TProtocolException(
        "peekSequenceID can only be called "."before readMessageBegin",
        TProtocolException::INVALID_DATA
      );
    }
    $data = $trans->peek(4);
    $sz = $this->unpackI32($data);
    $start = 4;
    if ($sz < 0) {
      $version = $sz & self::VERSION_MASK;
      if (\hacklib_not_equals($version, self::VERSION_1)) {
        throw new TProtocolException(
          "Bad version identifier: ".$sz,
          TProtocolException::BAD_VERSION
        );
      }
      $data = $trans->peek(4, $start);
      $name_len = $this->unpackI32($data);
      $start += 4 + $name_len;
      $data = $trans->peek(4, $start);
      $seqid = $this->unpackI32($data);
    } else {
      if (\hacklib_cast_as_boolean($this->strictRead_)) {
        throw new TProtocolException(
          "No version identifier, old protocol client?",
          TProtocolException::BAD_VERSION
        );
      } else {
        if (($this->memory_limit > 0) && ($sz > $this->memory_limit)) {
          throw new TProtocolException(
            "Length overflow: ".$sz,
            TProtocolException::SIZE_LIMIT
          );
        }
        $start += $sz;
        $start += 1;
        $data = $trans->peek(4, $start);
        $seqid = $this->unpackI32($data);
      }
    }
    return $seqid;
  }
  public function readMessageBegin(&$name, &$type, &$seqid) {
    $sz = 0;
    $result = $this->readI32($sz);
    if ($sz < 0) {
      $version = $sz & self::VERSION_MASK;
      if (\hacklib_not_equals($version, self::VERSION_1)) {
        throw new TProtocolException(
          "Bad version identifier: ".$sz,
          TProtocolException::BAD_VERSION
        );
      }
      $type = $sz & 0x000000ff;
      $result += $this->readString($name) + $this->readI32($seqid);
    } else {
      if (\hacklib_cast_as_boolean($this->strictRead_)) {
        throw new TProtocolException(
          "No version identifier, old protocol client?",
          TProtocolException::BAD_VERSION
        );
      } else {
        if (($this->memory_limit > 0) && ($sz > $this->memory_limit)) {
          throw new TProtocolException(
            "Length overflow: ".$sz,
            TProtocolException::SIZE_LIMIT
          );
        }
        $name = $this->trans_->readAll($sz);
        $result += $sz + $this->readByte($type) + $this->readI32($seqid);
      }
    }
    $this->sequenceID = $seqid;
    return $result;
  }
  public function readMessageEnd() {
    $this->sequenceID = null;
    return 0;
  }
  public function readStructBegin(&$name) {
    $name = "";
    return 0;
  }
  public function readStructEnd() {
    return 0;
  }
  public function readFieldBegin(&$name, &$fieldType, &$fieldId) {
    $result = $this->readByte($fieldType);
    if (\hacklib_equals($fieldType, TType::STOP)) {
      $fieldId = 0;
      return $result;
    }
    $result += $this->readI16($fieldId);
    return $result;
  }
  public function readFieldEnd() {
    return 0;
  }
  public function readMapBegin(&$keyType, &$valType, &$size) {
    return
      $this->readByte($keyType) +
      $this->readByte($valType) +
      $this->readI32($size);
  }
  public function readMapEnd() {
    return 0;
  }
  public function readListBegin(&$elemType, &$size) {
    return $this->readByte($elemType) + $this->readI32($size);
  }
  public function readListEnd() {
    return 0;
  }
  public function readSetBegin(&$elemType, &$size) {
    return $this->readByte($elemType) + $this->readI32($size);
  }
  public function readSetEnd() {
    return 0;
  }
  public function readBool(&$value) {
    $data = $this->trans_->readAll(1);
    $arr = unpack("c", $data);
    $value = \hacklib_equals($arr[1], 1);
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
  public function readI16(&$value) {
    $data = $this->trans_->readAll(2);
    $value = ord($data[1]) | (ord($data[0]) << 8);
    if ($value > 0x7fff) {
      $value = 0 - (($value - 1) ^ 0xffff);
    }
    return 2;
  }
  public function readI32(&$value) {
    $data = $this->trans_->readAll(4);
    $value = $this->unpackI32($data);
    return 4;
  }
  public function readI64(&$value) {
    $data = $this->trans_->readAll(8);
    $arr = unpack("N2", $data);
    if (PHP_INT_SIZE == 4) {
      $hi = $arr[1];
      $lo = $arr[2];
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
    return 8;
  }
  public function readDouble(&$value) {
    $data = $this->trans_->readAll(8);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $arr = unpack("d", $data);
    $value = $arr[1];
    return 8;
  }
  public function readFloat(&$value) {
    $data = $this->trans_->readAll(4);
    if (\hacklib_cast_as_boolean($this->littleendian_)) {
      $data = strrev($data);
    }
    $arr = unpack("f", $data);
    $value = $arr[1];
    return 4;
  }
  public function readString(&$value) {
    $len = 0;
    $result = $this->readI32($len);
    if (\hacklib_cast_as_boolean($len)) {
      $value = $this->trans_->readAll($len);
    } else {
      $value = "";
    }
    return $result + $len;
  }
}
