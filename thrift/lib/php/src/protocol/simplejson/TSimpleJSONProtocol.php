<?php

/**
* Copyright (c) 2006- Facebook
* Distributed under the Thrift Software License
*
* See accompanying file LICENSE or visit the Thrift site at:
* http://developers.facebook.com/thrift/
*
* @package thrift.protocol.simplejson
*/

require_once ($GLOBALS["HACKLIB_ROOT"]);
if (!isset($GLOBALS['THRIFT_ROOT'])) {
  $GLOBALS['THRIFT_ROOT'] = __DIR__.'/../..';
}
require_once $GLOBALS['THRIFT_ROOT'].'/TType.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocol.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/TProtocolException.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocolContext.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocolListContext.php';
require_once $GLOBALS['THRIFT_ROOT'].'/protocol/simplejson/TSimpleJSONProtocolMapContext.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/IThriftBufferedTransport.php';
require_once $GLOBALS['THRIFT_ROOT'].'/transport/TBufferedTransport.php';
class TSimpleJSONProtocol extends TProtocol {
  const VERSION_1 = 0x80010000;
  private $bufTrans;
  private $contexts;
  public function __construct($trans) {
    $this->contexts = new \HH\Vector(array());
    if (!($trans instanceof IThriftBufferedTransport)) {
      $trans = new TBufferedTransport($trans);
    }
    $this->bufTrans = $trans;
    parent::__construct($trans);
    $this->contexts
      ->add(new TSimpleJSONProtocolContext($this->trans_, $this->bufTrans));
  }
  private function pushListWriteContext() {
    return $this->pushWriteContext(
      new TSimpleJSONProtocolListContext($this->trans_, $this->bufTrans)
    );
  }
  private function pushMapWriteContext() {
    return $this->pushWriteContext(
      new TSimpleJSONProtocolMapContext($this->trans_, $this->bufTrans)
    );
  }
  private function pushListReadContext() {
    $this->pushReadContext(
      new TSimpleJSONProtocolListContext($this->trans_, $this->bufTrans)
    );
  }
  private function pushMapReadContext() {
    $this->pushReadContext(
      new TSimpleJSONProtocolMapContext($this->trans_, $this->bufTrans)
    );
  }
  private function pushWriteContext($ctx) {
    $this->contexts->add($ctx);
    return $ctx->writeStart();
  }
  private function popWriteContext() {
    $ctx = $this->contexts->pop();
    return $ctx->writeEnd();
  }
  private function pushReadContext($ctx) {
    $this->contexts->add($ctx);
    $ctx->readStart();
  }
  private function popReadContext() {
    $ctx = $this->contexts->pop();
    $ctx->readEnd();
  }
  private function getContext() {
    return $this->contexts->at($this->contexts->count() - 1);
  }
  public function writeMessageBegin($name, $type, $seqid) {
    return
      $this->getContext()->writeSeparator() +
      $this->pushListWriteContext() +
      $this->writeI32(self::VERSION_1) +
      $this->writeString($name) +
      $this->writeI32($type) +
      $this->writeI32($seqid);
  }
  public function writeMessageEnd() {
    return $this->popWriteContext();
  }
  public function writeStructBegin($name) {
    return
      $this->getContext()->writeSeparator() + $this->pushMapWriteContext();
  }
  public function writeStructEnd() {
    return $this->popWriteContext();
  }
  public function writeFieldBegin($fieldName, $fieldType, $fieldId) {
    return $this->writeString($fieldName);
  }
  public function writeFieldEnd() {
    return 0;
  }
  public function writeFieldStop() {
    return 0;
  }
  public function writeMapBegin($keyType, $valType, $size) {
    return
      $this->getContext()->writeSeparator() + $this->pushMapWriteContext();
  }
  public function writeMapEnd() {
    return $this->popWriteContext();
  }
  public function writeListBegin($elemType, $size) {
    return
      $this->getContext()->writeSeparator() + $this->pushListWriteContext();
  }
  public function writeListEnd() {
    return $this->popWriteContext();
  }
  public function writeSetBegin($elemType, $size) {
    return
      $this->getContext()->writeSeparator() + $this->pushListWriteContext();
  }
  public function writeSetEnd() {
    return $this->popWriteContext();
  }
  public function writeBool($value) {
    $x = $this->getContext()->writeSeparator();
    if (\hacklib_cast_as_boolean($value)) {
      $this->trans_->write("true");
      $x += 4;
    } else {
      $this->trans_->write("false");
      $x += 5;
    }
    return $x;
  }
  public function writeByte($value) {
    return $this->writeNum((int) $value);
  }
  public function writeI16($value) {
    return $this->writeNum((int) $value);
  }
  public function writeI32($value) {
    return $this->writeNum((int) $value);
  }
  public function writeI64($value) {
    return $this->writeNum((int) $value);
  }
  public function writeDouble($value) {
    return $this->writeNum((float) $value);
  }
  public function writeFloat($value) {
    return $this->writeNum((float) $value);
  }
  private function writeNum($value) {
    $ctx = $this->getContext();
    $ret = $ctx->writeSeparator();
    if (\hacklib_cast_as_boolean($ctx->escapeNum())) {
      $value = (string) $value;
    }
    $enc = json_encode($value);
    $this->trans_->write($enc);
    return $ret + strlen($enc);
  }
  public function writeString($value) {
    $ctx = $this->getContext();
    $ret = $ctx->writeSeparator();
    $value = (string) $value;
    $sb = new StringBuffer();
    $sb->append("\"");
    $len = strlen($value);
    for ($i = 0; $i < $len; $i++) {
      $c = $value[$i];
      $ord = ord($c);
      switch ($ord) {
        case 8:
          $sb->append("\\b");
          break;
        case 9:
          $sb->append("\\t");
          break;
        case 10:
          $sb->append("\\n");
          break;
        case 12:
          $sb->append("\\f");
          break;
        case 13:
          $sb->append("\\r");
          break;
        case 34:
        case 92:
          $sb->append("\\");
          $sb->append($c);
          break;
        default:
          if (($ord < 32) || ($ord > 126)) {
            $sb->append("\\u00");
            $sb->append(bin2hex($c));
          } else {
            $sb->append($c);
          }
          break;
      }
    }
    $sb->append("\"");
    $enc = $sb->detach();
    $this->trans_->write($enc);
    return $ret + strlen($enc);
  }
  public function readMessageBegin(&$name, &$type, &$seqid) {
    throw new TProtocolException(
      "Reading with TSimpleJSONProtocol is not supported. ".
      "Use readFromJSON() on your struct"
    );
  }
  public function readMessageEnd() {
    throw new TProtocolException(
      "Reading with TSimpleJSONProtocol is not supported. ".
      "Use readFromJSON() on your struct"
    );
  }
  public function readStructBegin(&$name) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushMapReadContext();
  }
  public function readStructEnd() {
    $this->popReadContext();
  }
  public function readFieldBegin(&$name, &$fieldType, &$fieldId) {
    $fieldId = null;
    $ctx = $this->getContext();
    $name = null;
    while ($name === null) {
      if (\hacklib_cast_as_boolean($ctx->readContextOver())) {
        $fieldType = TType::STOP;
        break;
      } else {
        $ctx->readSeparator();
        $this->skipWhitespace();
        $name = $this->readJSONString()[0];
        $offset = $this->skipWhitespace(true);
        $this->expectChar(":", true, $offset);
        $offset += 1 + $this->skipWhitespace(true, $offset + 1);
        $c = $this->bufTrans->peek(1, $offset);
        if (($c === "n") && ($this->bufTrans->peek(4, $offset) === "null")) {
          $ctx->readSeparator();
          $this->skipWhitespace();
          $this->trans_->readAll(4);
          $name = null;
          continue;
        } else {
          $fieldType = $this->guessFieldTypeBasedOnByte($c);
        }
      }
    }
  }
  public function readFieldEnd() {}
  public function readMapBegin(&$keyType, &$valType, &$size) {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushMapReadContext();
    if (\hacklib_cast_as_boolean($this->readMapHasNext())) {
      $keyType = TType::STRING;
      $this->skipWhitespace();
      $offset = $this->readJSONString(true)[1];
      $offset += $this->skipWhitespace(true, $offset);
      $this->expectChar(":", true, $offset);
      $offset += 1 + $this->skipWhitespace(true, $offset + 1);
      $c = $this->bufTrans->peek(1, $offset);
      $valType = $this->guessFieldTypeBasedOnByte($c);
    }
  }
  public function readMapHasNext() {
    return !\hacklib_cast_as_boolean($this->getContext()->readContextOver());
  }
  public function readMapEnd() {
    $this->popReadContext();
  }
  public function readListBegin(&$elemType, &$size) {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushListReadContext();
    if (\hacklib_cast_as_boolean($this->readListHasNext())) {
      $this->skipWhitespace();
      $c = $this->bufTrans->peek(1);
      $elemType = $this->guessFieldTypeBasedOnByte($c);
    }
  }
  public function readListHasNext() {
    return !\hacklib_cast_as_boolean($this->getContext()->readContextOver());
  }
  public function readListEnd() {
    $this->popReadContext();
  }
  public function readSetBegin(&$elemType, &$size) {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushListReadContext();
    if (\hacklib_cast_as_boolean($this->readSetHasNext())) {
      $this->skipWhitespace();
      $c = $this->bufTrans->peek(1);
      $elemType = $this->guessFieldTypeBasedOnByte($c);
    }
  }
  public function readSetHasNext() {
    return !\hacklib_cast_as_boolean($this->getContext()->readContextOver());
  }
  public function readSetEnd() {
    $this->popReadContext();
  }
  public function readBool(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $c = $this->trans_->readAll(1);
    $target = null;
    switch ($c) {
      case "t":
        $value = true;
        $target = "rue";
        break;
      case "f":
        $value = false;
        $target = "alse";
        break;
      default:
        throw new TProtocolException(
          "TSimpleJSONProtocol: Expected t or f, encountered 0x".bin2hex($c)
        );
    }
    for ($i = 0; $i < strlen($target); $i++) {
      $this->expectChar($target[$i]);
    }
  }
  private function readInteger($min, $max) {
    $val = intval($this->readNumStr());
    if (($min !== null) &&
        ($max !== null) &&
        (($val < $min) || ($val > $max))) {
      throw new TProtocolException(
        "TProtocolException: value ".$val." is outside the expected bounds"
      );
    }
    return $val;
  }
  private function readNumStr() {
    $ctx = $this->getContext();
    if (\hacklib_cast_as_boolean($ctx->escapeNum())) {
      $this->expectChar("\"");
    }
    $count = 0;
    $reading = true;
    while (\hacklib_cast_as_boolean($reading)) {
      $c = $this->bufTrans->peek(1, $count);
      switch ($c) {
        case "+":
        case "-":
        case ".":
        case "0":
        case "1":
        case "2":
        case "3":
        case "4":
        case "5":
        case "6":
        case "7":
        case "8":
        case "9":
        case "E":
        case "e":
          $count++;
          break;
        default:
          $reading = false;
          break;
      }
    }
    $str = $this->trans_->readAll($count);
    if (!\hacklib_cast_as_boolean(
          preg_match(
            "/^[+-]?(?:0|[1-9]\\d*)(?:\\.\\d+)?(?:[eE][+-]?\\d+)?\044/",
            $str
          )
        )) {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Invalid json number ".$str
      );
    }
    if (\hacklib_cast_as_boolean($ctx->escapeNum())) {
      $this->expectChar("\"");
    }
    return $str;
  }
  public function readByte(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x80, 0x7f);
  }
  public function readI16(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x8000, 0x7fff);
  }
  public function readI32(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x80000000, 0x7fffffff);
  }
  public function readI64(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(null, null);
  }
  public function readDouble(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = (float)$this->readNumStr();
  }
  public function readFloat(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = (float)$this->readNumStr();
  }
  public function readString(&$value) {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readJSONString()[0];
  }
  private function readJSONString($peek = false, $start = 0) {
    if (!\hacklib_cast_as_boolean($peek)) {
      $start = 0;
    }
    $this->expectChar("\"", $peek, $start);
    $count = \hacklib_cast_as_boolean($peek) ? 1 : 0;
    $sb = new StringBuffer();
    $reading = true;
    while (\hacklib_cast_as_boolean($reading)) {
      $c = $this->bufTrans->peek(1, $start + $count);
      switch ($c) {
        case "\"":
          $reading = false;
          break;
        case "\\":
          $count++;
          $c = $this->bufTrans->peek(1, $start + $count);
          switch ($c) {
            case "\\":
              $count++;
              $sb->append("\\");
              break;
            case "\"":
              $count++;
              $sb->append("\"");
              break;
            case "b":
              $count++;
              $sb->append(chr(0x08));
              break;
            case "/":
              $count++;
              $sb->append("/");
              break;
            case "f":
              $count++;
              $sb->append("\014");
              break;
            case "n":
              $count++;
              $sb->append("\n");
              break;
            case "r":
              $count++;
              $sb->append("\r");
              break;
            case "t":
              $count++;
              $sb->append("\t");
              break;
            case "u":
              $count++;
              $this->expectChar("0", true, $start + $count);
              $this->expectChar("0", true, $start + $count + 1);
              $count += 2;
              $sb->append(hex2bin($this->bufTrans->peek(2, $start + $count)));
              $count += 2;
              break;
            default:
              throw new TProtocolException(
                "TSimpleJSONProtocol: Expected Control Character, found 0x".
                bin2hex($c)
              );
          }
          break;
        case '':
          // end of buffer, this string is unclosed
          $reading = false;
          break;
        default:
          $count++;
          $sb->append($c);
          break;
      }
    }
    if (!\hacklib_cast_as_boolean($peek)) {
      $this->trans_->readAll($count);
    }
    $this->expectChar("\"", $peek, $start + $count);
    return \HH\Pair::hacklib_new($sb->detach(), $count + 1);
  }
  private function skipWhitespace($peek = false, $start = 0) {
    if (!\hacklib_cast_as_boolean($peek)) {
      $start = 0;
    }
    $count = 0;
    $reading = true;
    while (\hacklib_cast_as_boolean($reading)) {
      $byte = $this->bufTrans->peek(1, $count + $start);
      switch ($byte) {
        case " ":
        case "\t":
        case "\n":
        case "\r":
          $count++;
          break;
        default:
          $reading = false;
          break;
      }
    }
    if (!\hacklib_cast_as_boolean($peek)) {
      $this->trans_->readAll($count);
    }
    return $count;
  }
  private function expectChar($char, $peek = false, $start = 0) {
    if (!\hacklib_cast_as_boolean($peek)) {
      $start = 0;
    }
    $c = null;
    if (\hacklib_cast_as_boolean($peek)) {
      $c = $this->bufTrans->peek(1, $start);
    } else {
      $c = $this->trans_->readAll(1);
    }
    if ($c !== $char) {
      throw new TProtocolException(
        "TSimpleJSONProtocol: Expected ".
        $char.
        ", but encountered 0x".
        bin2hex($c)
      );
    }
  }
  private function guessFieldTypeBasedOnByte($byte) {
    switch ($byte) {
      case "{":
        return TType::STRUCT;
      case "[":
        return TType::LST;
      case "t":
      case "f":
        return TType::BOOL;
      case "-":
      case "0":
      case "1":
      case "2":
      case "3":
      case "4":
      case "5":
      case "6":
      case "7":
      case "8":
      case "9":
      case "+":
      case ".":
        return TType::DOUBLE;
      case "\"":
        return TType::STRING;
      case "]":
      case "}":
        return TType::STOP;
    }
    throw new TProtocolException(
      "TSimpleJSONProtocol: Unable to guess TType for character 0x".
      bin2hex($byte)
    );
  }
}
