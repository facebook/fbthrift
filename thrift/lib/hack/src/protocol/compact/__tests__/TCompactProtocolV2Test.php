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

<<Oncalls('thrift')>>
final class TCompactProtocolV2Test extends WWWTest {
  use ClassLevelTest;

  // Cases that run against both protocols pin V2 to the bytes the base
  // protocol already produces, not to V2's own output.
  private static function newProtocol(
    string $protocol,
    TMemoryBuffer $buffer,
  ): TCompactProtocolBase {
    return $protocol === 'v2'
      ? new TCompactProtocolV2($buffer)
      : new TCompactProtocolUnaccelerated($buffer);
  }

  public static function providerProtocols(
  ): dict<string, shape('protocol' => string)> {
    return dict[
      'base' => shape('protocol' => 'base'),
      'v2' => shape('protocol' => 'v2'),
    ];
  }

  public static function providerDouble(): dict<string, shape(
    'protocol' => string,
    'version' => int,
    'value' => float,
    'expected' => string,
  )> {
    $cases = dict[
      'big endian positive' => tuple(
        TCompactProtocolBase::VERSION_DOUBLE_BE,
        1.5,
        "\x3f\xf8\x00\x00\x00\x00\x00\x00",
      ),
      'big endian negative' => tuple(
        TCompactProtocolBase::VERSION_DOUBLE_BE,
        -2.0,
        "\xc0\x00\x00\x00\x00\x00\x00\x00",
      ),
      'little endian positive' => tuple(
        TCompactProtocolBase::VERSION_LOW,
        1.5,
        "\x00\x00\x00\x00\x00\x00\xf8\x3f",
      ),
      'little endian negative' => tuple(
        TCompactProtocolBase::VERSION_LOW,
        -2.0,
        "\x00\x00\x00\x00\x00\x00\x00\xc0",
      ),
    ];
    $out = dict[];
    foreach (vec['base', 'v2'] as $protocol) {
      foreach ($cases as $name => list($version, $value, $expected)) {
        $out[$protocol.': '.$name] = shape(
          'protocol' => $protocol,
          'version' => $version,
          'value' => $value,
          'expected' => $expected,
        );
      }
    }
    return $out;
  }

  <<DataProvider('providerDouble')>>
  public function testDoubleWireFormat(
    string $protocol,
    int $version,
    float $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);
    $prot->setWriteVersion($version);

    $prot->writeDouble($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0.0;
    $prot->readDouble(inout $out);
    expect($out)->toEqual($value);
  }

  public static function providerFloat(): dict<
    string,
    shape('protocol' => string, 'value' => float, 'expected' => string),
  > {
    $out = dict[];
    foreach (vec['base', 'v2'] as $protocol) {
      $out[$protocol.': positive'] = shape(
        'protocol' => $protocol,
        'value' => 1.5,
        'expected' => "\x3f\xc0\x00\x00",
      );
      $out[$protocol.': negative'] = shape(
        'protocol' => $protocol,
        'value' => -2.0,
        'expected' => "\xc0\x00\x00\x00",
      );
    }
    return $out;
  }

  <<DataProvider('providerFloat')>>
  public function testFloatWireFormat(
    string $protocol,
    float $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeFloat($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0.0;
    $prot->readFloat(inout $out);
    expect($out)->toEqual($value);
  }

  // V2 only: the base getVarint never terminates on negative input.
  public static function providerVarint(
  ): dict<string, shape('value' => int, 'expected' => string)> {
    return dict[
      'zero' => shape('value' => 0, 'expected' => "\x00"),
      'one byte max' => shape('value' => 127, 'expected' => "\x7f"),
      'two bytes min' => shape('value' => 128, 'expected' => "\x80\x01"),
      'two bytes' => shape('value' => 300, 'expected' => "\xac\x02"),
      'i32 max' => shape(
        'value' => Math\INT32_MAX,
        'expected' => "\xff\xff\xff\xff\x07",
      ),
      'i64 max' => shape(
        'value' => Math\INT64_MAX,
        'expected' => "\xff\xff\xff\xff\xff\xff\xff\xff\x7f",
      ),
      'negative one' => shape(
        'value' => -1,
        'expected' => "\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01",
      ),
      'i64 min' => shape(
        'value' => Math\INT64_MIN,
        'expected' => "\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01",
      ),
    ];
  }

  // getVarint packs each encoded length differently; the base protocol's
  // byte-at-a-time encoder is the reference for non-negative values.
  public function testEveryVarintLengthMatchesBase(): void {
    $base = new TCompactProtocolUnaccelerated(new TMemoryBuffer());
    $v2 = new TCompactProtocolV2(new TMemoryBuffer());
    for ($bytes = 1; $bytes <= 9; $bytes++) {
      $min = $bytes === 1 ? 0 : 1 << (7 * ($bytes - 1));
      $max = $bytes === 9 ? Math\INT64_MAX : (1 << (7 * $bytes)) - 1;
      foreach (
        vec[$min, $min + 1, $min + (($max - $min) >> 1), $max] as $value
      ) {
        $expected = $base->getVarint($value);
        expect(Str\length($expected))->toEqual($bytes);
        expect($v2->getVarint($value))->toEqual($expected, '%d', $value);
      }
    }
  }

  <<DataProvider('providerVarint')>>
  public function testVarintRoundTrip(int $value, string $expected): void {
    $buffer = new TMemoryBuffer();
    $prot = new TCompactProtocolV2($buffer);

    expect($prot->getVarint($value))->toEqual($expected);
    $prot->writeVarint($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0;
    expect($prot->readVarint(inout $out))->toEqual(Str\length($expected));
    expect($out)->toEqual($value);
  }

  public static function providerI64(
  ): dict<string, shape('protocol' => string, 'value' => int)> {
    $values = dict[
      'zero' => 0,
      'negative one' => -1,
      'i32 max' => Math\INT32_MAX,
      'just past u32' => 4294967297,
      'just below negative u32' => -4294967297,
      'large positive' => 0x0123456789abcdef,
      'large negative' => -0x0123456789abcdef,
      'i64 max' => Math\INT64_MAX,
      'i64 min' => Math\INT64_MIN,
    ];
    $out = dict[];
    foreach (vec['base', 'v2'] as $protocol) {
      foreach ($values as $name => $value) {
        $out[$protocol.': '.$name] =
          shape('protocol' => $protocol, 'value' => $value);
      }
    }
    return $out;
  }

  <<DataProvider('providerI64')>>
  public function testI64MatchesExtension(string $protocol, int $value): void {
    $pokemon = thriftshim_Pokemon::withDefaultValues();
    $pokemon->charizard = $value;
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $pokemon->write($prot);
    expect($buffer->getBuffer())->toEqual(
      thrift_protocol_write_compact_struct_to_string(
        $pokemon,
        TCompactProtocolBase::VERSION,
      ),
    );

    $pokemon_in = thriftshim_Pokemon::withDefaultValues();
    $pokemon_in->read($prot);
    expect($pokemon_in->charizard)->toEqual($value);
  }

  <<DataProvider('providerProtocols')>>
  public function testNestedStructRestoresFieldDelta(string $protocol): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeStructBegin('outer');
    $prot->writeFieldBegin('a', TType::I32, 1);
    $prot->writeI32(7);
    $prot->writeFieldEnd();
    $prot->writeFieldBegin('nested', TType::STRUCT, -5);
    $prot->writeStructBegin('inner');
    $prot->writeFieldBegin('x', TType::I32, 2);
    $prot->writeI32(1);
    $prot->writeFieldEnd();
    $prot->writeFieldStop();
    $prot->writeStructEnd();
    $prot->writeFieldEnd();
    // Encodes as delta 1 only if the outer lastFid (-5) was restored.
    $prot->writeFieldBegin('b', TType::I32, -4);
    $prot->writeI32(3);
    $prot->writeFieldEnd();
    $prot->writeFieldStop();
    $prot->writeStructEnd();
    expect($buffer->getBuffer())
      ->toEqual("\x15\x0e\x0c\x09\x25\x02\x00\x15\x06\x00");

    $name = null;
    $type = null;
    $id = null;
    $value = 0;
    $prot->readStructBegin(inout $name);
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    $prot->readI32(inout $value);
    expect(tuple($type, $id, $value))->toEqual(tuple(TType::I32, 1, 7));
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    expect(tuple($type, $id))->toEqual(tuple(TType::STRUCT, -5));
    $prot->readStructBegin(inout $name);
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    $prot->readI32(inout $value);
    expect(tuple($type, $id, $value))->toEqual(tuple(TType::I32, 2, 1));
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    expect($type)->toEqual(TType::STOP);
    $prot->readStructEnd();
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    $prot->readI32(inout $value);
    expect(tuple($type, $id, $value))->toEqual(tuple(TType::I32, -4, 3));
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    expect($type)->toEqual(TType::STOP);
    $prot->readStructEnd();
  }

  <<DataProvider('providerProtocols')>>
  public function testListOfStructsRestoresFieldDelta(string $protocol): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeStructBegin('outer');
    $prot->writeFieldBegin('items', TType::LST, 1);
    $prot->writeListBegin(TType::STRUCT, 2);
    foreach (vec[5, 6] as $item) {
      $prot->writeStructBegin('item');
      $prot->writeFieldBegin('v', TType::I32, 1);
      $prot->writeI32($item);
      $prot->writeFieldEnd();
      $prot->writeFieldStop();
      $prot->writeStructEnd();
    }
    $prot->writeListEnd();
    $prot->writeFieldEnd();
    // Encodes as delta 1 only if leaving the list restored lastFid to 1.
    $prot->writeFieldBegin('after', TType::I32, 2);
    $prot->writeI32(7);
    $prot->writeFieldEnd();
    $prot->writeFieldStop();
    $prot->writeStructEnd();
    expect($buffer->getBuffer())
      ->toEqual("\x19\x2c\x15\x0a\x00\x15\x0c\x00\x15\x0e\x00");

    $name = null;
    $type = null;
    $id = null;
    $size = null;
    $value = 0;
    $prot->readStructBegin(inout $name);
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    expect(tuple($type, $id))->toEqual(tuple(TType::LST, 1));
    $prot->readListBegin(inout $type, inout $size);
    expect(tuple($type, $size))->toEqual(tuple(TType::STRUCT, 2));
    $items = vec[];
    for ($i = 0; $i < 2; $i++) {
      $prot->readStructBegin(inout $name);
      $prot->readFieldBegin(inout $name, inout $type, inout $id);
      $prot->readI32(inout $value);
      $items[] = $value;
      $prot->readFieldEnd();
      $prot->readFieldBegin(inout $name, inout $type, inout $id);
      $prot->readStructEnd();
    }
    expect($items)->toEqual(vec[5, 6]);
    $prot->readListEnd();
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    $prot->readI32(inout $value);
    expect(tuple($type, $id, $value))->toEqual(tuple(TType::I32, 2, 7));
    $prot->readFieldEnd();
    $prot->readFieldBegin(inout $name, inout $type, inout $id);
    expect($type)->toEqual(TType::STOP);
    $prot->readStructEnd();
  }

  public static function providerStructs(
  ): dict<string, shape('struct' => IThriftSyncStruct)> {
    $kudo = thriftshim_Kudo::withDefaultValues();
    $kudo->is_awesome = true;
    $kudo->awesome_score = -7;
    $pokemon = thriftshim_Pokemon::withDefaultValues();
    $pokemon->charmander = -43;
    $pokemon->charmeleon = 107373182;
    $pokemon->charizard = Math\INT64_MIN;
    $foobar = thriftshim_FooBar::withDefaultValues();
    $foobar->foo = Vector {4.5, -2.3};
    $foobar->bar = 'Test';
    return dict[
      'bool and byte' => shape('struct' => $kudo),
      'integers' => shape('struct' => $pokemon),
      'double list and string' => shape('struct' => $foobar),
    ];
  }

  <<DataProvider('providerStructs')>>
  public function testStructMatchesExtension(IThriftSyncStruct $struct): void {
    $buffer = new TMemoryBuffer();
    $prot = new TCompactProtocolV2($buffer);

    $struct->write($prot);
    expect($buffer->getBuffer())->toEqual(
      thrift_protocol_write_compact_struct_to_string(
        $struct,
        TCompactProtocolBase::VERSION,
      ),
    );

    $struct_in = $struct::withDefaultValues();
    $struct_in->read($prot);
    expect($struct_in)->toBePHPEqual($struct);
  }

  // A comparable description of decoding $bytes: the exception class, or the
  // decoded struct re-serialized by the base protocol.
  private static function decodeOutcome(
    string $protocol,
    string $bytes,
    IThriftSyncStruct $template,
  ): string {
    $struct = $template::withDefaultValues();
    try {
      $struct->read(self::newProtocol($protocol, new TMemoryBuffer($bytes)));
    } catch (Exception $e) {
      return 'threw '.Classnames::getx($e);
    }
    $buffer = new TMemoryBuffer();
    $struct->write(new TCompactProtocolUnaccelerated($buffer));
    return 'decoded '.PHP\bin2hex($buffer->getBuffer());
  }

  private static function extensionOutcome(
    string $bytes,
    IThriftSyncStruct $template,
  ): string {
    try {
      $struct = thrift_protocol_read_compact_struct_from_string(
        $bytes,
        Classnames::getx($template),
        0,
        TCompactProtocolBase::VERSION,
      ) as IThriftStruct;
    } catch (Exception $e) {
      return 'threw '.Classnames::getx($e);
    }
    $buffer = new TMemoryBuffer();
    $struct->write(new TCompactProtocolUnaccelerated($buffer));
    return 'decoded '.PHP\bin2hex($buffer->getBuffer());
  }

  // The base protocol's 32-bit hi/lo i64 emulation flips the sign when a
  // 10-byte varint sets bits beyond the 64th; V2 and the native extension drop
  // those bits.
  public function testOverflowingI64VarintMatchesExtension(): void {
    $bytes = "\x36".Str\repeat("\xff", 9)."\x02\x00";
    $template = thriftshim_Pokemon::withDefaultValues();

    expect(self::decodeOutcome('v2', $bytes, $template))
      ->toEqual(self::extensionOutcome($bytes, $template));
    expect(self::decodeOutcome('base', $bytes, $template))
      ->toNotEqual(self::extensionOutcome($bytes, $template));
  }

  public static function providerCorrupted(): dict<
    string,
    shape('protocol' => string, 'value' => string, 'expected' => string),
  > {
    $cases = dict[
      'unknown compact type' => tuple("\x1e", 'threw OutOfBoundsException'),
      'list with unknown element type' =>
        tuple("\x19\x1e", 'threw OutOfBoundsException'),
      'truncated varint' => tuple("\x15\x80", 'threw TTransportException'),
      'truncated double' => tuple("\x17\x00\x00", 'threw TTransportException'),
      'truncated collection size' =>
        tuple("\x19\xfc\x80", 'threw TTransportException'),
      'string longer than input' =>
        tuple("\x18\x10abc", 'threw TTransportException'),
      'missing stop' => tuple("\x14\x0e", 'threw TTransportException'),
    ];
    $out = dict[];
    foreach (vec['base', 'v2'] as $protocol) {
      foreach ($cases as $name => list($bytes, $expected)) {
        $out[$protocol.': '.$name] = shape(
          'protocol' => $protocol,
          'value' => $bytes,
          'expected' => $expected,
        );
      }
    }
    return $out;
  }

  <<DataProvider('providerCorrupted')>>
  public function testCorruptedInputFails(
    string $protocol,
    string $value,
    string $expected,
  ): void {
    expect(
      self::decodeOutcome(
        $protocol,
        $value,
        thriftshim_Pokemon::withDefaultValues(),
      ),
    )->toEqual($expected);
  }

  // Not a valid encoding, but both protocols must consume it the same way.
  <<DataProvider('providerProtocols')>>
  public function testOverlongVarintMatchesBase(string $protocol): void {
    $bytes = "\x15".Str\repeat("\x80", 11)."\x01\x00";
    expect(
      self::decodeOutcome(
        $protocol,
        $bytes,
        thriftshim_Pokemon::withDefaultValues(),
      ),
    )->toEqual(
      self::decodeOutcome(
        'base',
        $bytes,
        thriftshim_Pokemon::withDefaultValues(),
      ),
    );
  }

  // See testOverflowingI64VarintMatchesExtension for where base and V2 differ.
  <<DataProvider('providerStructs')>>
  public function testMutatedInputMatchesBaseOrExtension(
    IThriftSyncStruct $struct,
  ): void {
    $buffer = new TMemoryBuffer();
    $struct->write(new TCompactProtocolUnaccelerated($buffer));
    $valid = $buffer->getBuffer();
    $length = Str\length($valid);

    $inputs = vec[];
    for ($i = 0; $i < $length; $i++) {
      $inputs[] = Str\slice($valid, 0, $i);
      $byte = PHP\ord($valid[$i]);
      foreach (vec[0x00, 0xff, $byte ^ 0x80, ($byte + 1) & 0xff] as $new) {
        $inputs[] =
          Str\slice($valid, 0, $i).PHP\chr($new).Str\slice($valid, $i + 1);
      }
    }

    foreach ($inputs as $input) {
      $v2 = self::decodeOutcome('v2', $input, $struct);
      expect(
        $v2 === self::decodeOutcome('base', $input, $struct) ||
          $v2 === self::extensionOutcome($input, $struct),
      )->toBeTrue();
    }
  }
}
