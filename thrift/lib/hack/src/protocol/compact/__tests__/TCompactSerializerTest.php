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
 */

<<Oncalls('thrift')>>
final class TCompactSerializerTest extends WWWTest {

  // The knob samples requests in production; pin it off so other cases never
  // queue a PSP comparison.
  <<__Override>>
  public static async function createData(): Awaitable<void> {
    MockJustKnobs::setBool('thrift/hack:compact_protocol_v2', false);
  }

  public static function provideStructs(): vec<(IThriftStruct)> {
    return vec[
      tuple(thriftshim_Kudo::withDefaultValues()),
      tuple(new thriftshim_Kudo(true, 123)),
      tuple(new thriftshim_Kudo(false, -123)),
      tuple(new thriftshim_FooBar(Vector {1.0, 2.0, 3.0, -1.0})),
    ];
  }

  public static function providerVersions(): vec<(?int)> {
    $versions = vec[tuple(null)];
    for (
      $version = TCompactProtocolBase::VERSION_LOW;
      $version <= TCompactProtocolBase::VERSION;
      $version++
    ) {
      $versions[] = tuple($version);
    }
    return $versions;
  }

  <<DataProvider('provideStructs', 'providerVersions')>>
  public async function testSerializeDeserialize(
    IThriftStruct $struct,
    ?int $version,
  ): Awaitable<void> {
    $cls = Classes::getx($struct);
    $serialized = TCompactSerializer::serialize($struct, $version);
    $deserialized_struct =
      TCompactSerializer::deserialize($serialized, new $cls(), $version);
    $serialized_again =
      TCompactSerializer::serialize($deserialized_struct, $version);
    expect($serialized_again)->toEqual($serialized);
  }

  public static function providerProtocolV2Knob(
  ): dict<string, shape('enabled' => bool)> {
    return dict[
      'protocol v2 comparison off' => shape('enabled' => false),
      'protocol v2 comparison on' => shape('enabled' => true),
    ];
  }

  <<DataProvider('providerProtocolV2Knob')>>
  public function testPureHackPathsAcrossProtocolV2Knob(bool $enabled): void {
    MockJustKnobs::setBool('thrift/hack:compact_protocol_v2', $enabled);
    // shouldCompareProtocolV2() is memoized; earlier cases would otherwise pin
    // its value.
    clear_class_memoization(
      TCompactSerializer::class,
      'shouldCompareProtocolV2',
    );
    $struct = new thriftshim_FooBar(Vector {1.0, -2.5}, 'bar');
    $expected = thrift_protocol_write_compact_struct_to_string(
      $struct,
      TCompactProtocolBase::VERSION,
    );

    $serialized = TCompactSerializer::serialize($struct, null, true);
    expect($serialized)->toEqual($expected);
    $deserialized = TCompactSerializer::deserialize(
      $serialized,
      thriftshim_FooBar::withDefaultValues(),
      null,
      true,
    );
    expect(TCompactSerializer::serialize($deserialized, null, true))
      ->toEqual($expected);

    $i64 = shape('type' => TType::I64);
    $data = TCompactSerializer::serializeData(Math\INT64_MIN, $i64);
    expect(TCompactSerializer::deserializeData($data, $i64))
      ->toEqual(Math\INT64_MIN);

    // V2 decodes this i64 with the opposite sign; the knob only schedules a
    // comparison, so the base protocol's value is still returned.
    $overflow = TCompactSerializer::deserialize(
      "\x36".Str\repeat("\xff", 9)."\x02\x00",
      thriftshim_Pokemon::withDefaultValues(),
      null,
      true,
    );
    expect($overflow->charizard)->toEqual(4611686018427387904);
  }
}
