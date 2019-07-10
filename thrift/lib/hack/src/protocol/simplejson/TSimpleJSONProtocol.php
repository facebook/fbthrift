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
 * Protocol for encoding/decoding simple json
 */
class TSimpleJSONProtocol extends TProtocol {
  const VERSION_1 = 0x80010000;

  private TMemoryBuffer $buffer;
  private Vector<TSimpleJSONProtocolContext> $contexts;

  <<__Rx>>
  public function __construct(TMemoryBuffer $trans) {
    parent::__construct($trans);
    $this->buffer = $trans;
    $this->contexts = Vector {
      new TSimpleJSONProtocolContext($this->buffer),
    };
  }

  protected function pushListWriteContext(): int {
    return $this->pushWriteContext(
      new TSimpleJSONProtocolListContext($this->buffer),
    );
  }

  protected function pushMapWriteContext(): int {
    return $this->pushWriteContext(
      new TSimpleJSONProtocolMapContext($this->buffer),
    );
  }

  protected function pushListReadContext(): void {
    $this->pushReadContext(
      new TSimpleJSONProtocolListContext($this->buffer),
    );
  }

  protected function pushMapReadContext(): void {
    $this->pushReadContext(
      new TSimpleJSONProtocolMapContext($this->buffer),
    );
  }

  protected function pushWriteContext(TSimpleJSONProtocolContext $ctx): int {
    $this->contexts->add($ctx);
    return $ctx->writeStart();
  }

  protected function popWriteContext(): int {
    $ctx = $this->contexts->pop();
    return $ctx->writeEnd();
  }

  protected function pushReadContext(TSimpleJSONProtocolContext $ctx): void {
    $this->contexts->add($ctx);
    $ctx->readStart();
  }

  protected function popReadContext(): void {
    $ctx = $this->contexts->pop();
    $ctx->readEnd();
  }

  protected function getContext(): TSimpleJSONProtocolContext {
    return $this->contexts->at($this->contexts->count() - 1);
  }

  <<__Override>>
  public function writeMessageBegin($name, $type, $seqid): num {
    return
      $this->getContext()->writeSeparator() +
      $this->pushListWriteContext() +
      $this->writeI32(self::VERSION_1) +
      $this->writeString($name) +
      $this->writeI32($type) +
      $this->writeI32($seqid);
  }

  <<__Override>>
  public function writeMessageEnd(): int {
    return $this->popWriteContext();
  }

  <<__Override>>
  public function writeStructBegin($name): int {
    return
      $this->getContext()->writeSeparator() + $this->pushMapWriteContext();
  }

  <<__Override>>
  public function writeStructEnd(): int {
    return $this->popWriteContext();
  }

  <<__Override>>
  public function writeFieldBegin($fieldName, $fieldType, $fieldId): int {
    return $this->writeString($fieldName);
  }

  <<__Override>>
  public function writeFieldEnd(): int {
    return 0;
  }

  <<__Override>>
  public function writeFieldStop(): int {
    return 0;
  }

  <<__Override>>
  public function writeMapBegin($keyType, $valType, $size): int {
    return
      $this->getContext()->writeSeparator() + $this->pushMapWriteContext();
  }

  <<__Override>>
  public function writeMapEnd(): int {
    return $this->popWriteContext();
  }

  <<__Override>>
  public function writeListBegin($elemType, $size): int {
    return
      $this->getContext()->writeSeparator() + $this->pushListWriteContext();
  }

  <<__Override>>
  public function writeListEnd(): int {
    return $this->popWriteContext();
  }

  <<__Override>>
  public function writeSetBegin($elemType, $size): int {
    return
      $this->getContext()->writeSeparator() + $this->pushListWriteContext();
  }

  <<__Override>>
  public function writeSetEnd(): int {
    return $this->popWriteContext();
  }

  <<__Override>>
  public function writeBool($value): int {
    $x = $this->getContext()->writeSeparator();
    if ($value) {
      $this->buffer->write('true');
      $x += 4;
    } else {
      $this->buffer->write('false');
      $x += 5;
    }
    return $x;
  }

  <<__Override>>
  public function writeByte($value): int {
    return $this->writeNum((int) $value);
  }

  <<__Override>>
  public function writeI16($value): int {
    return $this->writeNum((int) $value);
  }

  <<__Override>>
  public function writeI32($value): int {
    return $this->writeNum((int) $value);
  }

  <<__Override>>
  public function writeI64($value): int {
    return $this->writeNum((int) $value);
  }

  <<__Override>>
  public function writeDouble($value): int {
    return $this->writeNum((float) $value);
  }

  <<__Override>>
  public function writeFloat($value): int {
    return $this->writeNum((float) $value);
  }

  protected function writeNum($value): int {
    $ctx = $this->getContext();
    $ret = $ctx->writeSeparator();
    if ($ctx->escapeNum()) {
      $value = (string) $value;
    }

    $enc = PHP\json_encode($value);
    $this->buffer->write($enc);

    return $ret + Str\length($enc);
  }

  <<__Override>>
  public function writeString($value): int {
    $ctx = $this->getContext();
    $ret = $ctx->writeSeparator();
    $value = (string) $value;

    $sb = '"';
    $len = Str\length($value);
    for ($i = 0; $i < $len; $i++) {
      $c = $value[$i];
      $ord = PHP\ord($c);
      switch ($ord) {
        case 8:
          $sb .= '\b';
          break;
        case 9:
          $sb .= '\t';
          break;
        case 10:
          $sb .= '\n';
          break;
        case 12:
          $sb .= '\f';
          break;
        case 13:
          $sb .= '\r';
          break;
        case 34:
          // "
        case 92:
          // \
          $sb .= '\\';
          $sb .= $c;
          break;
        default:
          if ($ord < 32 || $ord > 126) {
            $sb .= '\\u00';
            $sb .= PHP\bin2hex($c);
          } else {
            $sb .= $c;
          }
          break;
      }
    }
    $sb .= '"';
    $this->buffer->write($sb);

    return $ret + Str\length($sb);
  }

  <<__Override>>
  public function readMessageBegin(
    inout $name,
    inout $type,
    inout $seqid,
  ): void {
    throw new TProtocolException(
      'Reading with TSimpleJSONProtocol is not supported. '.
      'Use readFromJSON() on your struct',
    );
  }

  <<__Override>>
  public function readMessageEnd(): void {
    throw new TProtocolException(
      'Reading with TSimpleJSONProtocol is not supported. '.
      'Use readFromJSON() on your struct',
    );
  }

  <<__Override>>
  public function readStructBegin(inout $_name): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushMapReadContext();
  }

  <<__Override>>
  public function readStructEnd(): void {
    $this->popReadContext();
  }

  <<__Override>>
  public function readFieldBegin(
    inout $name,
    inout $fieldType,
    inout $fieldId,
  ): void {
    $fieldId = null;
    $ctx = $this->getContext();
    $name = null;
    while ($name === null) {
      if ($ctx->readContextOver()) {
        $fieldType = TType::STOP;
        break;
      } else {
        $ctx->readSeparator();
        $this->skipWhitespace();
        $name = $this->readJSONString()[0];
        // We need to guess the type of the value, in case the name is bogus or we are in a skip method up the stack
        $offset = $this->skipWhitespace(true);
        $this->expectChar(':', true, $offset);
        $offset += 1 + $this->skipWhitespace(true, $offset + 1);
        $c = $this->buffer->peek(1, $offset);
        if ($c === 'n' && $this->buffer->peek(4, $offset) === 'null') {
          // We actually want to skip this field, but there isn't an appropriate
          // TType to send back. So instead, we will silently skip
          $ctx->readSeparator();
          $this->skipWhitespace();
          $this->buffer->readAll(4);
          $name = null;
          continue;
        } else {
          $fieldType = $this->guessFieldTypeBasedOnByte($c);
        }
      }
    }
  }

  <<__Override>>
  public function readFieldEnd(): void {
    // Do nothing
  }

  <<__Override>>
  public function readMapBegin(
    inout $keyType,
    inout $valType,
    inout $size,
  ): void {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushMapReadContext();
    if ($this->readMapHasNext()) {
      // We need to guess the type of the keys/values, in case we are in a skip method up the stack
      $keyType = TType::STRING;
      $this
        ->skipWhitespace(); // This is not a peek, since we can do this safely again
      $offset = $this->readJSONString(true)[1];
      $offset += $this->skipWhitespace(true, $offset);
      $this->expectChar(':', true, $offset);
      $offset += 1 + $this->skipWhitespace(true, $offset + 1);
      $c = $this->buffer->peek(1, $offset);
      $valType = $this->guessFieldTypeBasedOnByte($c);
    }
  }

  <<__Override>>
  public function readMapHasNext(): bool {
    return !$this->getContext()->readContextOver();
  }

  <<__Override>>
  public function readMapEnd(): void {
    $this->popReadContext();
  }

  <<__Override>>
  public function readListBegin(inout $elemType, inout $size): void {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushListReadContext();
    if ($this->readListHasNext()) {
      // We need to guess the type of the values, in case we are in a skip method up the stack
      $this->skipWhitespace(); // This is not a peek, since we can do this safely again
      $c = $this->buffer->peek(1);
      $elemType = $this->guessFieldTypeBasedOnByte($c);
    }
  }

  <<__Override>>
  public function readListHasNext(): bool {
    return !$this->getContext()->readContextOver();
  }

  <<__Override>>
  public function readListEnd(): void {
    $this->popReadContext();
  }

  <<__Override>>
  public function readSetBegin(inout $elemType, inout $size): void {
    $size = null;
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $this->pushListReadContext();
    if ($this->readSetHasNext()) {
      // We need to guess the type of the values, in case we are in a skip method up the stack
      $this->skipWhitespace(); // This is not a peek, since we can do this safely again
      $c = $this->buffer->peek(1);
      $elemType = $this->guessFieldTypeBasedOnByte($c);
    }
  }

  <<__Override>>
  public function readSetHasNext(): bool {
    return !$this->getContext()->readContextOver();
  }

  <<__Override>>
  public function readSetEnd(): void {
    $this->popReadContext();
  }

  <<__Override>>
  public function readBool(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $c = $this->buffer->readAll(1);
    switch ($c) {
      case 't':
        $value = true;
        $target = 'rue';
        break;
      case 'f':
        $value = false;
        $target = 'alse';
        break;
      case '0':
        $value = false;
        $target = '';
        break;
      case '1':
        $value = true;
        $target = '';
        break;
      default:
        throw new TProtocolException(
          'TSimpleJSONProtocol: Expected t or f, encountered 0x'.
          PHP\bin2hex($c),
        );
    }

    for ($i = 0; $i < Str\length($target); $i++) {
      $this->expectChar($target[$i]);
    }
  }

  protected function readInteger(?int $min, ?int $max): int {
    $val = PHP\intval($this->readNumStr());
    if (($min !== null && $max !== null) && ($val < $min || $val > $max)) {
      throw new TProtocolException(
        'TProtocolException: value '.
        $val.
        ' is outside the expected bounds',
      );
    }
    return $val;
  }

  protected function readNumStr(): string {
    $ctx = $this->getContext();
    if ($ctx->escapeNum()) {
      $this->expectChar('"');
    }
    $count = 0;
    $reading = true;
    while ($reading) {
      $c = $this->buffer->peek(1, $count);
      switch ($c) {
        case '+':
        case '-':
        case '.':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'E':
        case 'e':
          $count++;
          break;
        default:
          $reading = false;
          break;
      }
    }
    $str = $this->buffer->readAll($count);
    if (!PHP\fb\preg_match_simple(
          '/^[+-]?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?$/',
          $str,
        )) {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Invalid json number '.$str,
      );
    }
    if ($ctx->escapeNum()) {
      $this->expectChar('"');
    }

    return $str;
  }

  <<__Override>>
  public function readByte(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x80, 0x7f);
  }

  <<__Override>>
  public function readI16(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x8000, 0x7fff);
  }

  <<__Override>>
  public function readI32(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(-0x80000000, 0x7fffffff);
  }

  <<__Override>>
  public function readI64(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readInteger(null, null);
  }

  <<__Override>>
  public function readDouble(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = (float)$this->readNumStr();
  }

  <<__Override>>
  public function readFloat(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = (float)$this->readNumStr();
  }

  <<__Override>>
  public function readString(inout $value): void {
    $this->getContext()->readSeparator();
    $this->skipWhitespace();
    $value = $this->readJSONString()[0];
  }

  protected function readJSONString(
    bool $peek = false,
    int $start = 0,
  ): Pair<string, int> {
    if (!$peek) {
      $start = 0;
    }
    $this->expectChar('"', $peek, $start);
    $count = $peek ? 1 : 0;
    $sb = '';
    $reading = true;
    while ($reading) {
      $c = $this->buffer->peek(1, $start + $count);
      switch ($c) {
        case '"':
          $reading = false;
          break;
        case '\\':
          $count++;
          $c = $this->buffer->peek(1, $start + $count);
          switch ($c) {
            case '\\':
              $count++;
              $sb .= '\\';
              break;
            case '"':
              $count++;
              $sb .= '"';
              break;
            case 'b':
              $count++;
              $sb .= PHP\chr(0x08);
              break;
            case '/':
              $count++;
              $sb .= '/';
              break;
            case 'f':
              $count++;
              $sb .= "\f";
              break;
            case 'n':
              $count++;
              $sb .= "\n";
              break;
            case 'r':
              $count++;
              $sb .= "\r";
              break;
            case 't':
              $count++;
              $sb .= "\t";
              break;
            case 'u':
              $count++;
              $this->expectChar('0', true, $start + $count);
              $this->expectChar('0', true, $start + $count + 1);
              $count += 2;
              $sb .= PHP\hex2bin($this->buffer->peek(2, $start + $count));
              $count += 2;
              break;
            default:
              throw new TProtocolException(
                'TSimpleJSONProtocol: Expected Control Character, found 0x'.
                PHP\bin2hex($c),
              );
          }
          break;
        case '':
          // end of buffer, this string is unclosed
          $reading = false;
          break;
        default:
          $count++;
          $sb .= $c;
          break;
      }
    }

    if (!$peek) {
      $this->buffer->readAll($count);
    }

    $this->expectChar('"', $peek, $start + $count);
    return Pair {$sb, $count + 1};
  }

  protected function skipWhitespace(bool $peek = false, int $start = 0): int {
    if (!$peek) {
      $start = 0;
    }
    $count = 0;
    $reading = true;
    while ($reading) {
      $byte = $this->buffer->peek(1, $count + $start);
      switch ($byte) {
        case ' ':
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
    if (!$peek) {
      $this->buffer->readAll($count);
    }

    return $count;
  }

  protected function expectChar(
    string $char,
    bool $peek = false,
    int $start = 0,
  ): void {
    if (!$peek) {
      $start = 0;
    }
    if ($peek) {
      $c = $this->buffer->peek(1, $start);
    } else {
      $c = $this->buffer->readAll(1);
    }

    if ($c !== $char) {
      throw new TProtocolException(
        'TSimpleJSONProtocol: Expected '.
        $char.
        ', but encountered 0x'.
        PHP\bin2hex($c),
      );
    }
  }

  protected function guessFieldTypeBasedOnByte(string $byte): ?int {
    switch ($byte) {
      case '{':
        return TType::STRUCT;
      case '[':
        return TType::LST;
      case 't':
      case 'f':
        return TType::BOOL;
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        // These technically aren't allowed to start JSON floats, but are here for backwards compatibility
      case '+':
      case '.':
        return TType::DOUBLE;
      case '"':
        return TType::STRING;
      case ']':
      case '}':
        // We can get here with empty lists/maps, returning a dummy value
        return TType::STOP;
    }

    throw new TProtocolException(
      'TSimpleJSONProtocol: Unable to guess TType for character 0x'.
      PHP\bin2hex($byte),
    );
  }
}
