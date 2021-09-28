/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp2/omnithrift/transcoder/Transcoder.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "thrift/lib/cpp2/omnithrift/transcoder/test/gen-cpp2/test_structs_types.h"

using namespace apache::thrift;
using namespace apache::thrift::omniclient;
using namespace transcoder::test_structs;
using apache::thrift::protocol::TType;

Warning makeWarning() {
  Warning warning;
  warning.message_ref() = "something went wrong!";
  warning.code_ref() = 123;
  return warning;
}

Country makeCountry() {
  Country country;
  country.name_ref() = "USA";
  country.continent_ref() = Continent::NorthAmerica;
  country.capital_ref() = "Washington DC";
  country.population_ref() = 327.2;
  return country;
}

City makeCity(const std::string& name = "Boston") {
  City city;
  city.name_ref() = name;
  city.country_ref() = makeCountry();
  return city;
}

std::vector<City> makeCities() {
  return {makeCity("City1"), makeCity("City2")};
}

std::map<std::string, City> makeCitiesMap() {
  return {{"city1", makeCity("City1")}, {"city2", makeCity("City2")}};
}

std::map<int64_t, std::string> makeZipMap() {
  return {{94002, "belmont"}, {94403, "san mateo"}};
}

std::map<std::string, std::vector<City>> makeMapOfCities() {
  return {{"county", makeCities()}};
}

// Helper structure to package up Reader and Writer into one type.
template <class ProtocolReader>
struct Protocol {
  using Reader = ProtocolReader;
  using Writer = typename Reader::ProtocolWriter;
};
using CompactProtocol = Protocol<CompactProtocolReader>;
using BinaryProtocol = Protocol<BinaryProtocolReader>;
using JSONProtocol = Protocol<JSONProtocolReader>;

/**
 * Perform the following serialization (->) and transcoding (~>) chain:
 *
 *   T -> SProtocol ~> TProtocol ~> SProtocol -> T
 *
 * Verify equality of the input T with the output T.
 */
template <class TProtocol, class SProtocol, class T>
void transcoderTest(TType type, const T& input) {
  using S = Serializer<typename SProtocol::Reader, typename SProtocol::Writer>;

  // T -> SProtocol
  auto serialized = S::template serialize<std::string>(input);
  auto serializedBytes = folly::ByteRange(folly::StringPiece(serialized));

  // SProtocol ~> TProtocol
  folly::IOBuf bufIn(folly::IOBuf::WRAP_BUFFER, serializedBytes);
  folly::IOBufQueue bufOut(folly::IOBufQueue::cacheChainLength());
  typename SProtocol::Reader in1;
  typename TProtocol::Writer out1;
  in1.setInput(&bufIn);
  out1.setOutput(&bufOut);
  Transcoder<typename SProtocol::Reader, typename TProtocol::Writer>::transcode(
      in1, out1, type);

  // TProtocol ~> SProtocol
  folly::IOBufQueue bufFinal(folly::IOBufQueue::cacheChainLength());
  typename TProtocol::Reader in2;
  typename SProtocol::Writer out2;
  in2.setInput(bufOut.front());
  out2.setOutput(&bufFinal);
  Transcoder<typename TProtocol::Reader, typename SProtocol::Writer>::transcode(
      in2, out2, type);

  // SProtocol -> T
  T output;
  S::deserialize(bufFinal.front(), output);

  // Verify input == output.
  ASSERT_EQ(input, output);
}

template <class T>
void transcoderTestProtocols(TType type, const T& input) {
  // Compact
  transcoderTest<CompactProtocol, CompactProtocol>(type, input);
  transcoderTest<CompactProtocol, BinaryProtocol>(type, input);

  // Binary
  transcoderTest<BinaryProtocol, CompactProtocol>(type, input);
  transcoderTest<BinaryProtocol, BinaryProtocol>(type, input);

  // JSON
  transcoderTest<JSONProtocol, CompactProtocol>(type, input);
  transcoderTest<JSONProtocol, BinaryProtocol>(type, input);
}

//
// Struct
//

TEST(TranscoderTest, Struct) {
  transcoderTestProtocols(TType::T_STRUCT, makeCity());
}

TEST(TranscoderTest, ComplexStruct) {
  Request request;
  request.cityList_ref() = makeCities();
  request.stringSet_ref()->insert("hello");
  request.cityMap_ref() = makeCitiesMap();
  transcoderTestProtocols(TType::T_STRUCT, request);
}

//
// Exception (represented as a struct)
//

TEST(TranscoderTest, Exception) {
  transcoderTestProtocols(TType::T_STRUCT, makeWarning());
}

//
// List
//

TEST(TranscoderTest, List) {
  transcoderTestProtocols(TType::T_LIST, makeCities());
  transcoderTestProtocols(TType::T_LIST, std::vector<City>{});
}

//
// Map
//

TEST(TranscoderTest, MapWithStringKey) {
  transcoderTestProtocols(TType::T_MAP, makeCitiesMap());
  transcoderTestProtocols(TType::T_MAP, std::map<std::string, City>{});
}

TEST(TranscoderTest, MapWithIntKey) {
  transcoderTestProtocols(TType::T_MAP, makeZipMap());
  transcoderTestProtocols(TType::T_MAP, std::map<int64_t, std::string>{});
}

TEST(TranscoderTest, MapWithListValue) {
  transcoderTestProtocols(TType::T_MAP, makeMapOfCities());
}
