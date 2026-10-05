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
  ): shape('expected' => string, 'read' => string, 'write' => string) {
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

  <<DataProvider('providerProtocols')>>
  public function testMatchingProtocolsCompareEqual(bool $compact): void {
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

    foreach (
      vec[
        tuple(
          $bytes,
          shape('type' => TType::STRUCT, 'class' => CompactTestStruct::class),
        ),
        tuple($data, $i64),
      ] as list($input, $spec)
    ) {
      $sides = self::sides($input, $spec, $compact);
      expect($sides['read'])->toEqual($sides['expected']);
      expect($sides['write'])->toEqual($sides['expected']);
    }
  }

  // The base protocol's 32-bit i64 emulation flips the sign of a varint with
  // bits beyond the 64th; V2 drops them, so the read side differs.
  public function testDifferentDecodesCompareUnequal(): void {
    $sides = self::sides(
      "\x36".Str\repeat("\xff", 9)."\x02\x00",
      shape('type' => TType::STRUCT, 'class' => thriftshim_Pokemon::class),
      true,
    );
    expect($sides['read'])->toNotEqual($sides['expected']);
  }
}
