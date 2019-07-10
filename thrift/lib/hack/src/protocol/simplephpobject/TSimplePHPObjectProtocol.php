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
class TSimplePHPObjectProtocol extends TProtocol implements IRxTProtocol {

  // 'copy-on-write iterator'. tuple tracks (position, iterable).
  const type TCOWIterator = (int, vec<mixed>);
  private vec<this::TCOWIterator> $stack;

  <<__Rx>>
  public function __construct(mixed $val) {
    $this->stack = vec[tuple(0, vec[$val])];
    parent::__construct(new TNullTransport());
  }

  <<__Override>>
  public function readMessageBegin(
    inout $name,
    inout $type,
    inout $seqid,
  ): void {
    throw new TProtocolException('Not Supported');
  }

  <<__Override>>
  public function readMessageEnd(): void {
    throw new TProtocolException('Not Supported');
  }

  <<__Rx, __MutableReturn>>
  public static function toThriftObject<T as IThriftStruct>(
    mixed $simple_object,
    <<__OwnedMutable>> T $thrift_object,
  ): T {
    $thrift_object->read(
      Rx\mutable(new TSimplePHPObjectProtocol($simple_object)),
    );
    return $thrift_object;
  }

  <<__Override, __Rx, __Mutable>>
  public function readStructBegin(inout $name): void {
    $name = null;
    $val = $this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($val is Set<_> || $val is ImmSet<_>) {
      throw new TProtocolException(
        'Unsupported data structure for struct: '.PHP\gettype($val),
      );
    } else if ($val is Vector<_> || $val is ImmVector<_>) {
      if (($val as ConstVector<_>)->isEmpty()) {
        // Empty maps can be (de)serialized incorrectly as vectors
        $itr = vec[];
      } else {
        throw new TProtocolException(
          'Unsupported data structure for struct: '.PHP\gettype($val),
        );
      }
    } else if ($val is dict<_, _>) {
      $itr = PHPArrayism::intishDarrayCast($val);
    } else if ($val is KeyedContainer<_, _>) {
      $itr = $val;
    } else {
      $itr = PHPism_FIXME::arrayCast($val);
    }
    $itr = self::flattenMap($itr);
    $this->stackPush($itr);
  }

  <<__Override, __Rx, __Mutable>>
  public function readStructEnd(): void {
    $this->stackPopBack();
  }

  <<__Override, __Rx, __Mutable>>
  public function readFieldBegin(
    inout $name,
    inout $fieldType,
    inout $fieldId,
  ): void {
    $val = null;
    while ($val === null) {
      if (!$this->stackTopIterValid()) {
        $fieldType = TType::STOP;
        return;
      }

      $fieldId = null;
      $name = $this->stackTopIterCurrent();
      $this->stackTopIterNext();
      $val = $this->stackTopIterCurrent();
      if ($val === null) {
        $this->stackTopIterNext();
        continue;
      }

      $fieldType = self::guessTypeForValue($val);
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readFieldEnd(): void {}

  <<__Override, __Rx, __Mutable>>
  public function readMapBegin(
    inout $key_type,
    inout $val_type,
    inout $size,
  ): void {
    $val = $this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($val is Set<_> || $val is ImmSet<_>) {
      throw new TProtocolException(
        'Unsupported data structure for map: '.PHP\gettype($val),
      );
    } else if ($val is Vector<_> || $val is ImmVector<_>) {
      if (($val as ConstVector<_>)->isEmpty()) {
        // Empty maps can be (de)serialized incorrectly as vectors
        $itr = vec[];
      } else {
        throw new TProtocolException(
          'Unsupported data structure for map: '.PHP\gettype($val),
        );
      }
    } else if ($val is dict<_, _>) {
      $itr = PHPArrayism::intishDarrayCast($val);
    } else if ($val is KeyedContainer<_, _>) {
      $itr = $val;
    } else {
      $itr = PHPism_FIXME::arrayCast($val);
    }
    $size = C\count($itr);
    $itr = self::flattenMap($itr);
    $this->stackPush($itr);

    if ($this->stackTopIterValid()) {
      $key_sample = $this->stackTopIterCurrent();
      $this->stackTopIterNext();
      $val_sample = $this->stackTopIterCurrent();
      $this->stackTopIterRewind();

      $key_type = self::guessTypeForValue($key_sample);
      $val_type = self::guessTypeForValue($val_sample);
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readMapEnd(): void {
    $this->stackPopBack();
  }

  <<__Override, __Rx, __Mutable>>
  public function readListBegin(inout $elem_type, inout $size): void {
    $val = $this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($val is Set<_> || $val is ImmSet<_> || $val is string) {
      throw new TProtocolException(
        'Unsupported data structure for list: '.PHP\gettype($val),
      );
    } else if ($val is dict<_, _>) {
      $itr = PHPArrayism::intishDarrayCast($val);
    } else if ($val is KeyedContainer<_, _>) {
      $itr = $val;
    } else {
      $itr = PHPism_FIXME::arrayCast($val);
    }
    $size = C\count($itr);
    $itr = vec($itr);
    $this->stackPush($itr);

    if ($this->stackTopIterValid()) {
      $val_sample = $this->stackTopIterCurrent();
      $elem_type = self::guessTypeForValue($val_sample);
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readListEnd(): void {
    $this->stackPopBack();
  }

  <<__Override, __Rx, __Mutable>>
  public function readSetBegin(inout $elem_type, inout $size): void {
    $val = $this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($val is dict<_, _>) {
      $itr = PHP\fb\varray_map(fun('HH\\array_key_cast'), PHP\array_keys($val));
    } else if ($val is Container<_>) {
      $itr = $val;
    } else {
      $itr = PHP\array_keys(PHPism_FIXME::arrayCast($val));
    }
    $size = C\count($itr);
    $itr = vec($itr);
    $this->stackPush($itr);

    if ($this->stackTopIterValid()) {
      $val_sample = $this->stackTopIterCurrent();
      $elem_type = self::guessTypeForValue($val_sample);
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readSetEnd(): void {
    $this->stackPopBack();
  }

  <<__Override, __Rx, __Mutable>>
  public function readBool(inout $value): void {
    $value = (bool)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
  }

  <<__Override, __Rx, __Mutable>>
  public function readByte(inout $value): void {
    $value = (int)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($value < -0x80 || $value > 0x7F) {
      throw new TProtocolException('Value is outside of valid range');
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readI16(inout $value): void {
    $value = (int)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($value < -0x8000 || $value > 0x7FFF) {
      throw new TProtocolException('Value is outside of valid range');
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readI32(inout $value): void {
    $value = (int)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if ($value < -0x80000000 || $value > 0x7FFFFFFF) {
      throw new TProtocolException('Value is outside of valid range');
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readI64(inout $value): void {
    $value = (int)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
    if (
      $value < (-0x80000000 << 32) || $value > (0x7FFFFFFF << 32 | 0xFFFFFFFF)
    ) {
      throw new TProtocolException('Value is outside of valid range');
    }
  }

  <<__Override, __Rx, __Mutable>>
  public function readDouble(inout $value): void {
    $value = (float)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
  }

  <<__Override, __Rx, __Mutable>>
  public function readFloat(inout $value): void {
    $value = (float)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
  }

  <<__Override, __Rx, __Mutable>>
  public function readString(inout $value): void {
    $value = (string)$this->stackTopIterCurrent();
    $this->stackTopIterNext();
  }

  <<__Override>>
  public function writeMessageBegin($name, $type, $seqid): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeMessageEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeStructBegin($name): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeStructEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeFieldBegin($fieldName, $fieldType, $fieldId): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeFieldEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeFieldStop(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeMapBegin($keyType, $valType, $size): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeMapEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeListBegin($elemType, $size): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeListEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeSetBegin($elemType, $size): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeSetEnd(): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeBool($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeByte($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeI16($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeI32($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeI64($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeDouble($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeFloat($value): void {
    throw new TProtocolException('Not Implemented');
  }

  <<__Override>>
  public function writeString($value): void {
    throw new TProtocolException('Not Implemented');
  }

  /**
   * Returns the index of the topmost element in the stack.
   */
  <<__Rx, __MaybeMutable>>
  private function stackTopIdx(): int {
    return C\count($this->stack) - 1;
  }

  /**
   * Returns true if the position of the iterator at the top of the stack is
   * valid, or false if the position is out of bounds.
   */
  <<__Rx, __MaybeMutable>>
  private function stackTopIterValid(): bool {
    list($top_pos, $top_iter) = $this->stack[$this->stackTopIdx()];
    return $top_pos >= 0 && $top_pos < C\count($top_iter);
  }

  /**
   * Returns the current element for the iterator at the top of the stack.
   */
  <<__Rx, __MaybeMutable>>
  private function stackTopIterCurrent(): mixed {
    list($top_pos, $top_iter) = $this->stack[$this->stackTopIdx()];
    return $top_iter[$top_pos];
  }

  /**
   * Moves the position for the iterator at the top of the stack forward.
   */
  <<__Rx, __Mutable>>
  private function stackTopIterNext(): void {
    $this->stack[$this->stackTopIdx()][0]++;
  }

  /**
   * Moves the position for the iterator at the top of the stack backward.
   */
  <<__Rx, __Mutable>>
  private function stackTopIterRewind(): void {
    $this->stack[$this->stackTopIdx()][0]--;
  }

  /**
   * Turns an iterable into a COW iterator and pushes it onto the stack.
   */
  <<__Rx, __Mutable>>
  private function stackPush(vec<mixed> $iterable): void {
    $this->stack[] = tuple(0, $iterable);
  }

  /**
   * Pops the iterator at the top of the stack.
   */
  <<__Rx, __Mutable>>
  private function stackPopBack(): void {
    /* HH_FIXME[4135] Exposed by banning unset and isset in partial mode */
    unset($this->stack[$this->stackTopIdx()]);
    invariant(
      C\last($this->stack) !== null,
      'Stack is empty, this shouldn\'t happen!',
    );
  }

  /**
   * Flattens the keys and values of a map sequentially into a vec.
   *
   * e.g. dict['foo' => 123, 'bar' => 456] -> vec['foo', 123, 'bar', 456]
   */
  <<__Rx>>
  private static function flattenMap(
    KeyedContainer<arraykey, mixed> $map,
  ): vec<mixed> {
    $res = vec[];
    foreach ($map as $key => $value) {
      $res[] = $key;
      $res[] = $value;
    }
    return $res;
  }

  <<__Rx>>
  private static function guessTypeForValue(mixed $val): int {
    if ($val is bool) {
      return TType::BOOL;
    } else if ($val is int) {
      return TType::I64;
    } else if ($val is float) {
      return TType::DOUBLE;
    } else if ($val is string) {
      return TType::STRING;
    } else if (
      PHP\is_object($val) ||
      is_array($val) ||
      $val is Map<_, _> ||
      $val is dict<_, _>
    ) {
      return TType::STRUCT;
    } else if ($val is Iterable<_> || $val is vec<_>) {
      return TType::LST;
    }

    throw new TProtocolException(
      'Unable to guess thrift type for '.PHP\gettype($val),
    );
  }
}
