<?hh
// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

<<Oncalls('thrift')>>
final class ThriftProtocolV2ComparisonTest extends WWWTest {

  use ClassLevelTest;

  // The base round trip, and V2's decode re-encoded by the base protocol and
  // by V2.
  private static function sides(
    string $bytes,
    ThriftStructTypes::TGenericSpec $spec,
    bool $compact,
  ): shape(
    'expected' => ThriftProtocolComparisonBytesOrException,
    'read' => ThriftProtocolComparisonBytesOrException,
    'write' => ThriftProtocolComparisonBytesOrException,
  ) {
    $version = $compact ? TCompactProtocolBase::VERSION : null;
    $decode = (bool $v2) ==> ThriftProtocolV2Comparison::decode(
      $bytes,
      $spec,
      $compact,
      $v2,
      $version,
      0,
    );
    $encode = (mixed $value, bool $v2) ==> ThriftProtocolV2Comparison::encode(
      $value,
      $spec,
      $compact,
      $v2,
      $version,
    );
    $value = $decode(true);
    return shape(
      'expected' => $encode($decode(false), false),
      'read' => $encode($value, false),
      'write' => $encode($value, true),
    );
  }

  private static function struct(): CompactTestStruct {
    $struct = CompactTestStruct::withDefaultValues();
    $struct->i1 = 42;
    $struct->b2 = true;
    $struct->doubles = vec[1.5, -2.5];
    $struct->m1 = dict['key1' => 10, 'key2' => -20];
    $struct->s = 'test string';
    return $struct;
  }

  public static function providerProtocols(
  ): dict<string, shape('compact' => bool)> {
    return dict[
      'binary' => shape('compact' => false),
      'compact' => shape('compact' => true),
    ];
  }

  private static function version(bool $compact): ?int {
    return $compact ? TCompactProtocolBase::VERSION : null;
  }

  // Bytes the base serializers wrote, with the spec to read them back.
  private static function inputs(
    bool $compact,
  ): vec<(string, ThriftStructTypes::TGenericSpec)> {
    $struct = self::struct();
    $bytes = $compact
      ? thrift_protocol_write_compact_struct_to_string(
          $struct,
          TCompactProtocolBase::VERSION,
        )
      : thrift_protocol_write_binary_struct_to_string($struct);
    $i64 = shape('type' => TType::I64);
    $data = $compact
      ? TCompactSerializer::serializeData(Math\INT64_MIN, $i64)
      : TBinarySerializer::serializeData(Math\INT64_MIN, $i64);
    return vec[
      tuple(
        $bytes,
        shape('type' => TType::STRUCT, 'class' => CompactTestStruct::class),
      ),
      tuple($data, $i64),
    ];
  }

  <<DataProvider('providerProtocols')>>
  public function testMatchingProtocolsCompareEqual(bool $compact): void {
    foreach (self::inputs($compact) as list($input, $spec)) {
      $sides = self::sides($input, $spec, $compact);
      expect($sides['read'])->toEqual($sides['expected']);
      expect($sides['write'])->toEqual($sides['expected']);
      $version = self::version($compact);
      expect(
        ThriftProtocolV2Comparison::compareRead(
          $input,
          $spec,
          $compact,
          $version,
          0,
        ),
      )->toBeNull();
      expect(
        ThriftProtocolV2Comparison::compareWrite(
          $input,
          $spec,
          $compact,
          $version,
          0,
        ),
      )->toBeNull();
    }
  }

  // The base protocol's 32-bit i64 emulation flips the sign of a varint with
  // bits beyond the 64th; V2 drops them, so the read side differs.
  public function testDifferentDecodesCompareUnequal(): void {
    $input = "\x36".Str\repeat("\xff", 9)."\x02\x00";
    $spec =
      shape('type' => TType::STRUCT, 'class' => thriftshim_Pokemon::class);
    $sides = self::sides($input, $spec, true);
    expect($sides['read'])->toNotEqual($sides['expected']);
    expect(
      ThriftProtocolV2Comparison::compareRead(
        $input,
        $spec,
        true,
        TCompactProtocolBase::VERSION,
        0,
      ),
    )->toEqual('V2 differs from the base protocol');
  }

  // Both protocols skip the unknown field, so their reads agree, but no Hack
  // round trip reproduces the input bytes.
  public function testUnknownFieldComparesEqualOnReadOnly(): void {
    $input = "\x08\x7f\xff\x00\x00\x00\x01\x00";
    $spec = shape('type' => TType::STRUCT, 'class' => CompactTestStruct::class);
    expect(
      ThriftProtocolV2Comparison::compareRead($input, $spec, false, null, 0),
    )->toBeNull();
    expect(
      ThriftProtocolV2Comparison::compareWrite($input, $spec, false, null, 0),
    )->toEqual('V2 differs from the base protocol');
  }

  <<DataProvider('providerProtocols')>>
  public function testSameDecodeFailureComparesEqual(bool $compact): void {
    $input = "\x80";
    $spec = shape('type' => TType::I64);
    $version = self::version($compact);
    foreach (vec[false, true] as $v2) {
      expect(
        ThriftProtocolV2Comparison::decode(
          $input,
          $spec,
          $compact,
          $v2,
          $version,
          0,
        ),
      )->toBeInstanceOf(Throwable::class);
    }
    expect(
      ThriftProtocolV2Comparison::compareRead(
        $input,
        $spec,
        $compact,
        $version,
        0,
      ),
    )->toBeNull();
    expect(
      ThriftProtocolV2Comparison::compareWrite(
        $input,
        $spec,
        $compact,
        $version,
        0,
      ),
    )->toBeNull();
  }

  <<DataProvider('providerProtocols')>>
  public function testEncodeFailureIsReturned(bool $compact): void {
    // Without an etype the list header can't be written.
    $spec = shape('type' => TType::LST);
    foreach (vec[false, true] as $v2) {
      expect(
        ThriftProtocolV2Comparison::encode(
          vec[1],
          $spec,
          $compact,
          $v2,
          self::version($compact),
        ),
      )->toBeInstanceOf(Throwable::class);
    }
  }

  // serializeData writes a top-level adapted value in its thrift form, but
  // decode returns it adapted.
  <<DataProvider('providerProtocols')>>
  public function testTopLevelAdapterComparesEqual(bool $compact): void {
    $spec = shape(
      'type' => TType::LST,
      'etype' => TType::I64,
      'elem' => shape('type' => TType::I64),
      'format' => 'harray',
      'adapter' => AdapterTestReverseList::class,
    );
    $bytes = $compact
      ? TCompactSerializer::serializeData(vec[1, 2, 3], $spec)
      : TBinarySerializer::serializeData(vec[1, 2, 3], $spec);
    expect(ThriftProtocolV2Comparison::compareWrite(
      $bytes,
      $spec,
      $compact,
      self::version($compact),
      0,
    ))->toBeNull();
  }
}
