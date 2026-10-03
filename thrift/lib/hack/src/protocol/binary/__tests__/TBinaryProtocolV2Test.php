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
final class TBinaryProtocolV2Test extends WWWTest {
  use ClassLevelTest;

  // Every wire-format case also runs against the protocol V2 replaces, so the
  // expected bytes are existing behavior, not V2's own output.
  private static function newProtocol(
    string $protocol,
    TMemoryBuffer $buffer,
  ): TBinaryProtocolBase {
    return $protocol === 'v2'
      ? new TBinaryProtocolV2($buffer)
      : new TBinaryProtocolUnaccelerated($buffer);
  }

  private static function acrossProtocols<Tv>(
    dict<string, (Tv, string)> $cases,
  ): dict<
    string,
    shape('protocol' => string, 'value' => Tv, 'expected' => string),
  > {
    $out = dict[];
    foreach (vec['base', 'v2'] as $protocol) {
      foreach ($cases as $name => list($value, $expected)) {
        $out[$protocol.': '.$name] = shape(
          'protocol' => $protocol,
          'value' => $value,
          'expected' => $expected,
        );
      }
    }
    return $out;
  }

  public static function providerI16(): dict<
    string,
    shape('protocol' => string, 'value' => int, 'expected' => string),
  > {
    return self::acrossProtocols(dict[
      'zero' => tuple(0, "\x00\x00"),
      'one' => tuple(1, "\x00\x01"),
      'negative one' => tuple(-1, "\xff\xff"),
      'max' => tuple(Math\INT16_MAX, "\x7f\xff"),
      'min' => tuple(Math\INT16_MIN, "\x80\x00"),
    ]);
  }

  <<DataProvider('providerI16')>>
  public function testI16WireFormat(
    string $protocol,
    int $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeI16($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0;
    $prot->readI16(inout $out);
    expect($out)->toEqual($value);
  }

  public static function providerI32(): dict<
    string,
    shape('protocol' => string, 'value' => int, 'expected' => string),
  > {
    return self::acrossProtocols(dict[
      'zero' => tuple(0, "\x00\x00\x00\x00"),
      'byte order' => tuple(0x01020304, "\x01\x02\x03\x04"),
      'negative one' => tuple(-1, "\xff\xff\xff\xff"),
      'max' => tuple(Math\INT32_MAX, "\x7f\xff\xff\xff"),
      'min' => tuple(Math\INT32_MIN, "\x80\x00\x00\x00"),
    ]);
  }

  <<DataProvider('providerI32')>>
  public function testI32WireFormat(
    string $protocol,
    int $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeI32($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0;
    $prot->readI32(inout $out);
    expect($out)->toEqual($value);
  }

  public static function providerI64(): dict<
    string,
    shape('protocol' => string, 'value' => int, 'expected' => string),
  > {
    return self::acrossProtocols(dict[
      'zero' => tuple(0, "\x00\x00\x00\x00\x00\x00\x00\x00"),
      'byte order' =>
        tuple(0x0102030405060708, "\x01\x02\x03\x04\x05\x06\x07\x08"),
      'negative one' => tuple(-1, "\xff\xff\xff\xff\xff\xff\xff\xff"),
      'max' => tuple(Math\INT64_MAX, "\x7f\xff\xff\xff\xff\xff\xff\xff"),
      'min' => tuple(Math\INT64_MIN, "\x80\x00\x00\x00\x00\x00\x00\x00"),
    ]);
  }

  <<DataProvider('providerI64')>>
  public function testI64WireFormat(
    string $protocol,
    int $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    $prot->writeI64($value);
    expect($buffer->getBuffer())->toEqual($expected);

    $out = 0;
    $prot->readI64(inout $out);
    expect($out)->toEqual($value);
  }

  public static function providerDouble(): dict<
    string,
    shape('protocol' => string, 'value' => float, 'expected' => string),
  > {
    return self::acrossProtocols(dict[
      'positive' => tuple(1.5, "\x3f\xf8\x00\x00\x00\x00\x00\x00"),
      'negative' => tuple(-2.0, "\xc0\x00\x00\x00\x00\x00\x00\x00"),
    ]);
  }

  <<DataProvider('providerDouble')>>
  public function testDoubleWireFormat(
    string $protocol,
    float $value,
    string $expected,
  ): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

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
    return self::acrossProtocols(dict[
      'positive' => tuple(1.5, "\x3f\xc0\x00\x00"),
      'negative' => tuple(-2.0, "\xc0\x00\x00\x00"),
    ]);
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

  public static function providerProtocols(
  ): dict<string, shape('protocol' => string)> {
    return dict[
      'base' => shape('protocol' => 'base'),
      'v2' => shape('protocol' => 'v2'),
    ];
  }

  <<DataProvider('providerProtocols')>>
  public function testHeaderWireFormat(string $protocol): void {
    $buffer = new TMemoryBuffer();
    $prot = self::newProtocol($protocol, $buffer);

    expect($prot->writeFieldBegin('f', TType::I32, -3))->toEqual(3);
    expect($prot->writeListBegin(TType::STRING, 70000))->toEqual(5);
    expect($prot->writeSetBegin(TType::I64, 2))->toEqual(5);
    expect($prot->writeMapBegin(TType::STRING, TType::I32, 1))->toEqual(6);
    expect($prot->writeFieldStop())->toEqual(1);
    expect($buffer->getBuffer())->toEqual(
      "\x08\xff\xfd".
      "\x0b\x00\x01\x11\x70".
      "\x0a\x00\x00\x00\x02".
      "\x0b\x08\x00\x00\x00\x01".
      "\x00",
    );

    $name = null;
    $type = null;
    $key_type = null;
    $id = null;
    $size = null;
    expect($prot->readFieldBegin(inout $name, inout $type, inout $id))
      ->toEqual(3);
    expect(tuple($type, $id))->toEqual(tuple(TType::I32, -3));
    expect($prot->readListBegin(inout $type, inout $size))->toEqual(5);
    expect(tuple($type, $size))->toEqual(tuple(TType::STRING, 70000));
    expect($prot->readSetBegin(inout $type, inout $size))->toEqual(5);
    expect(tuple($type, $size))->toEqual(tuple(TType::I64, 2));
    expect($prot->readMapBegin(inout $key_type, inout $type, inout $size))
      ->toEqual(6);
    expect(tuple($key_type, $type, $size))
      ->toEqual(tuple(TType::STRING, TType::I32, 1));
    expect($prot->readFieldBegin(inout $name, inout $type, inout $id))
      ->toEqual(1);
    expect($type)->toEqual(TType::STOP);
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
    $prot = new TBinaryProtocolV2($buffer);

    $struct->write($prot);
    expect($buffer->getBuffer())
      ->toEqual(thrift_protocol_write_binary_struct_to_string($struct));

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
    $struct->write(new TBinaryProtocolUnaccelerated($buffer));
    return 'decoded '.PHP\bin2hex($buffer->getBuffer());
  }

  public static function providerCorrupted(): dict<
    string,
    shape('protocol' => string, 'value' => string, 'expected' => string),
  > {
    return self::acrossProtocols(dict[
      'unknown field type' =>
        tuple("\x7f\x00\x01", 'threw UnexpectedValueException'),
      'negative field type' =>
        tuple("\xff\x00\x01", 'threw UnexpectedValueException'),
      'truncated field id' => tuple("\x06\x00", 'threw TTransportException'),
      'truncated value' =>
        tuple("\x08\x00\x02\x00\x00", 'threw TTransportException'),
      'string longer than input' =>
        tuple("\x0b\x00\x05\x00\x00\x00\x10abc", 'threw TTransportException'),
      'list with unknown element type' => tuple(
        "\x0f\x00\x05\x7f\x00\x00\x00\x01",
        'threw UnexpectedValueException',
      ),
      'truncated map header' =>
        tuple("\x0d\x00\x05\x0b\x08\x00", 'threw TTransportException'),
      'missing stop' =>
        tuple("\x06\x00\x01\x00\x05", 'threw TTransportException'),
    ]);
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

  <<DataProvider('providerStructs')>>
  public function testMutatedInputMatchesBase(IThriftSyncStruct $struct): void {
    $buffer = new TMemoryBuffer();
    $struct->write(new TBinaryProtocolUnaccelerated($buffer));
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
      expect(self::decodeOutcome('v2', $input, $struct))
        ->toEqual(self::decodeOutcome('base', $input, $struct));
    }
  }
}
