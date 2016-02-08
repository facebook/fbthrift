<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolException.php';
abstract class TProtocol {
  public static $TBINARYPROTOCOLACCELERATED = "TBinaryProtocolAccelerated";
  public static $TCOMPACTPROTOCOLACCELERATED = "TCompactProtocolAccelerated";
  public static
    $TBINARYPROTOCOLUNACCELERATED = "TBinaryProtocolUnaccelerated";
  public static
    $TCOMPACTPROTOCOLUNACCELERATED = "TCompactProtocolUnaccelerated";
  protected $trans_;
  protected function __construct($trans) {
    $this->trans_ = $trans;
  }
  public function getTransport() {
    return $this->trans_;
  }
  public function getOutputTransport() {
    return $this->trans_;
  }
  public function getInputTransport() {
    return $this->trans_;
  }
  public abstract function writeMessageBegin($name, $type, $seqid);
  public abstract function writeMessageEnd();
  public abstract function writeStructBegin($name);
  public abstract function writeStructEnd();
  public abstract function writeFieldBegin($fieldName, $fieldType, $fieldId);
  public abstract function writeFieldEnd();
  public abstract function writeFieldStop();
  public abstract function writeMapBegin($keyType, $valType, $size);
  public abstract function writeMapEnd();
  public abstract function writeListBegin($elemType, $size);
  public abstract function writeListEnd();
  public abstract function writeSetBegin($elemType, $size);
  public abstract function writeSetEnd();
  public abstract function writeBool($bool);
  public abstract function writeByte($byte);
  public abstract function writeI16($i16);
  public abstract function writeI32($i32);
  public abstract function writeI64($i64);
  public abstract function writeDouble($dub);
  public abstract function writeFloat($flt);
  public abstract function writeString($str);
  public abstract function readMessageBegin(&$name, &$type, &$seqid);
  public abstract function readMessageEnd();
  public abstract function readStructBegin(&$name);
  public abstract function readStructEnd();
  public abstract function readFieldBegin(&$name, &$fieldType, &$fieldId);
  public abstract function readFieldEnd();
  public abstract function readMapBegin(&$keyType, &$valType, &$size);
  public function readMapHasNext() {
    throw new TProtocolException(
      get_called_class()." does not support unknown map sizes"
    );
  }
  public abstract function readMapEnd();
  public abstract function readListBegin(&$elemType, &$size);
  public function readListHasNext() {
    throw new TProtocolException(
      get_called_class()." does not support unknown list sizes"
    );
  }
  public abstract function readListEnd();
  public abstract function readSetBegin(&$elemType, &$size);
  public function readSetHasNext() {
    throw new TProtocolException(
      get_called_class()." does not support unknown set sizes"
    );
  }
  public abstract function readSetEnd();
  public abstract function readBool(&$bool);
  public abstract function readByte(&$byte);
  public abstract function readI16(&$i16);
  public abstract function readI32(&$i32);
  public abstract function readI64(&$i64);
  public abstract function readDouble(&$dub);
  public abstract function readFloat(&$flt);
  public abstract function readString(&$str);
  public function skip($type) {
    $_ref = null;
    switch ($type) {
      case TType::BOOL:
        return $this->readBool($_ref);
      case TType::BYTE:
        return $this->readByte($_ref);
      case TType::I16:
        return $this->readI16($_ref);
      case TType::I32:
        return $this->readI32($_ref);
      case TType::I64:
        return $this->readI64($_ref);
      case TType::DOUBLE:
        return $this->readDouble($_ref);
      case TType::FLOAT:
        return $this->readFloat($_ref);
      case TType::STRING:
        return $this->readString($_ref);
      case TType::STRUCT:
        {
          $result = $this->readStructBegin($_ref);
          while (true) {
            $ftype = null;
            $result += $this->readFieldBegin($_ref, $ftype, $_ref);
            if (\hacklib_equals($ftype, TType::STOP)) {
              break;
            }
            $result += $this->skip($ftype);
            $result += $this->readFieldEnd();
          }
          $result += $this->readStructEnd();
          return $result;
        }
      case TType::MAP:
        {
          $keyType = null;
          $valType = null;
          $size = 0;
          $result = $this->readMapBegin($keyType, $valType, $size);
          for ($i = 0; ($size === null) || ($i < $size); $i++) {
            if (($size === null) &&
                (!\hacklib_cast_as_boolean($this->readMapHasNext()))) {
              break;
            }
            $result += $this->skip($keyType);
            $result += $this->skip($valType);
          }
          $result += $this->readMapEnd();
          return $result;
        }
      case TType::SET:
        {
          $elemType = null;
          $size = 0;
          $result = $this->readSetBegin($elemType, $size);
          for ($i = 0; ($size === null) || ($i < $size); $i++) {
            if (($size === null) &&
                (!\hacklib_cast_as_boolean($this->readSetHasNext()))) {
              break;
            }
            $result += $this->skip($elemType);
          }
          $result += $this->readSetEnd();
          return $result;
        }
      case TType::LST:
        {
          $elemType = null;
          $size = 0;
          $result = $this->readListBegin($elemType, $size);
          for ($i = 0; ($size === null) || ($i < $size); $i++) {
            if (($size === null) &&
                (!\hacklib_cast_as_boolean($this->readSetHasNext()))) {
              break;
            }
            $result += $this->skip($elemType);
          }
          $result += $this->readListEnd();
          return $result;
        }
      default:
        throw new TProtocolException(
          "Unknown field type: ".$type,
          TProtocolException::INVALID_DATA
        );
    }
  }
  public static function skipBinary($itrans, $type) {
    switch ($type) {
      case TType::BOOL:
        return $itrans->readAll(1);
      case TType::BYTE:
        return $itrans->readAll(1);
      case TType::I16:
        return $itrans->readAll(2);
      case TType::I32:
        return $itrans->readAll(4);
      case TType::I64:
        return $itrans->readAll(8);
      case TType::DOUBLE:
        return $itrans->readAll(8);
      case TType::FLOAT:
        return $itrans->readAll(4);
      case TType::STRING:
        $len = unpack("N", $itrans->readAll(4));
        $len = $len[1];
        if ($len > 0x7fffffff) {
          $len = 0 - (($len - 1) ^ 0xffffffff);
        }
        return 4 + $itrans->readAll($len);
      case TType::STRUCT:
        {
          $result = 0;
          while (true) {
            $ftype = 0;
            $fid = 0;
            $data = $itrans->readAll(1);
            $arr = unpack("c", $data);
            $ftype = $arr[1];
            if (\hacklib_equals($ftype, TType::STOP)) {
              break;
            }
            $result += $itrans->readAll(2);
            $result += self::skipBinary($itrans, $ftype);
          }
          return $result;
        }
      case TType::MAP:
        {
          $data = $itrans->readAll(1);
          $arr = unpack("c", $data);
          $ktype = $arr[1];
          $data = $itrans->readAll(1);
          $arr = unpack("c", $data);
          $vtype = $arr[1];
          $data = $itrans->readAll(4);
          $arr = unpack("N", $data);
          $size = $arr[1];
          if ($size > 0x7fffffff) {
            $size = 0 - (($size - 1) ^ 0xffffffff);
          }
          $result = 6;
          for ($i = 0; $i < $size; $i++) {
            $result += self::skipBinary($itrans, $ktype);
            $result += self::skipBinary($itrans, $vtype);
          }
          return $result;
        }
      case TType::SET:
      case TType::LST:
        {
          $data = $itrans->readAll(1);
          $arr = unpack("c", $data);
          $vtype = $arr[1];
          $data = $itrans->readAll(4);
          $arr = unpack("N", $data);
          $size = $arr[1];
          if ($size > 0x7fffffff) {
            $size = 0 - (($size - 1) ^ 0xffffffff);
          }
          $result = 5;
          for ($i = 0; $i < $size; $i++) {
            $result += self::skipBinary($itrans, $vtype);
          }
          return $result;
        }
      default:
        throw new TProtocolException(
          "Unknown field type: ".$type,
          TProtocolException::INVALID_DATA
        );
    }
  }
}
