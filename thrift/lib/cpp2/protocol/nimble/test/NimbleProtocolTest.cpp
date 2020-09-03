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

#include <thrift/lib/cpp2/protocol/NimbleProtocol.h>
#include <folly/Random.h>
#include <folly/container/Array.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/test/gen-cpp2/modules_types.h>
#include <random>

using namespace cpp2;

namespace apache {
namespace thrift {
namespace detail {

TEST(NimbleProtocolTest, BasicTypesTest) {
  auto array1 = folly::make_array<std::int8_t>(
      -128,
      -1,
      0,
      127,
      -101,
      73,
      INT8_MAX,
      INT8_MIN,
      folly::Random::rand32(256) - 128,
      folly::Random::rand32(256) - 128);
  auto array2 = folly::make_array<std::int16_t>(
      -32768,
      -12,
      0,
      258,
      INT16_MAX,
      -591,
      -1,
      INT16_MAX,
      folly::Random::rand32(65536) - 32768,
      folly::Random::rand32(65536) - 32768);
  auto array3 = folly::make_array<std::int32_t>(
      -2147483648,
      -70,
      0,
      67359,
      2147483647,
      INT32_MAX - 1,
      INT32_MIN,
      INT32_MAX,
      folly::Random::rand32() - 2147483648,
      folly::Random::rand32() - 2147483648);
  auto array4 = folly::make_array<std::int64_t>(
      INT64_MIN,
      -66778831,
      -1,
      3315153,
      INT64_MAX,
      0,
      -274,
      folly::Random::rand64(INT64_MIN, INT64_MAX),
      folly::Random::rand64(INT64_MIN, INT64_MAX),
      folly::Random::rand64(INT64_MIN, INT64_MAX));
  auto array5 = folly::make_array<bool>(
      true, true, false, false, true, true, 1, 0, true, false);
  auto array6 = folly::make_array<uint8_t>(
      0,
      2,
      124,
      221,
      255,
      UINT8_MAX,
      56,
      folly::Random::rand32(256),
      folly::Random::rand32(256),
      folly::Random::rand32(256));
  auto array7 = folly::make_array<uint16_t>(
      16,
      258,
      0,
      39102,
      65535,
      UINT16_MAX,
      UINT8_MAX,
      folly::Random::rand32(65536),
      folly::Random::rand32(65536),
      folly::Random::rand32(65536));
  auto array8 = folly::make_array<uint32_t>(
      298,
      0,
      2147483647,
      1114111,
      UINT32_MAX,
      2,
      UINT16_MAX,
      333666,
      folly::Random::rand32(),
      folly::Random::rand32());
  auto array9 = folly::make_array<uint64_t>(
      32767,
      4294967295,
      9223372036854775807,
      0,
      UINTMAX_MAX,
      UINT64_MAX,
      UINT8_MAX,
      121314,
      folly::Random::rand64(),
      folly::Random::rand64());
  auto array10 = folly::make_array<float>(
      FLT_MIN,
      0.001,
      -FLT_MAX,
      FLT_MAX,
      123.456789,
      0.0,
      -0.0,
      std::numeric_limits<float>::infinity(),
      -std::numeric_limits<float>::infinity(),
      NAN);
  auto array11 = folly::make_array<double>(
      DBL_MIN,
      -DBL_MAX,
      131415161718192.2250738585072014,
      -0.0000000000000001,
      DBL_MAX,
      0.0,
      -0.0,
      std::numeric_limits<double>::infinity(),
      -std::numeric_limits<double>::infinity(),
      NAN);

  auto sizes = folly::make_array<size_t>(
      array1.size(),
      array2.size(),
      array3.size(),
      array4.size(),
      array5.size(),
      array6.size(),
      array7.size(),
      array8.size(),
      array9.size(),
      array10.size(),
      array11.size());
  size_t smallest = *std::min_element(sizes.begin(), sizes.end());
  size_t largest = *std::max_element(sizes.begin(), sizes.end());
  EXPECT_EQ(smallest, largest);

  for (size_t i = 0; i < smallest; i++) {
    BasicTypes basicTypes;
    *basicTypes.myByte_ref() = array1[i];
    *basicTypes.myInt16_ref() = array2[i];
    *basicTypes.myInt32_ref() = array3[i];
    *basicTypes.myInt64_ref() = array4[i];
    *basicTypes.myBool_ref() = array5[i];
    *basicTypes.myUint8_ref() = array6[i];
    *basicTypes.myUint16_ref() = array7[i];
    *basicTypes.myUint32_ref() = array8[i];
    *basicTypes.myUint64_ref() = array9[i];
    *basicTypes.myFloat_ref() = array10[i];
    *basicTypes.myDouble_ref() = array11[i];

    NimbleProtocolWriter writer;
    basicTypes.write(&writer);

    std::unique_ptr<folly::IOBuf> message = writer.finalize();
    NimbleProtocolReader reader;
    reader.setInput(folly::io::Cursor{message.get()});

    BasicTypes decodedBasicTypes;
    decodedBasicTypes.read(&reader);

    EXPECT_EQ(*basicTypes.myByte_ref(), *decodedBasicTypes.myByte_ref());
    EXPECT_EQ(*basicTypes.myInt16_ref(), *decodedBasicTypes.myInt16_ref());
    EXPECT_EQ(*basicTypes.myInt32_ref(), *decodedBasicTypes.myInt32_ref());
    EXPECT_EQ(*basicTypes.myInt64_ref(), *decodedBasicTypes.myInt64_ref());
    EXPECT_EQ(*basicTypes.myBool_ref(), *decodedBasicTypes.myBool_ref());
    EXPECT_EQ(*basicTypes.myUint8_ref(), *decodedBasicTypes.myUint8_ref());
    EXPECT_EQ(*basicTypes.myUint16_ref(), *decodedBasicTypes.myUint16_ref());
    EXPECT_EQ(*basicTypes.myUint32_ref(), *decodedBasicTypes.myUint32_ref());
    EXPECT_EQ(*basicTypes.myUint64_ref(), *decodedBasicTypes.myUint64_ref());
    if (std::isnan(*basicTypes.myFloat_ref())) {
      EXPECT_TRUE(std::isnan(*decodedBasicTypes.myFloat_ref()));
    } else {
      EXPECT_FLOAT_EQ(
          *basicTypes.myFloat_ref(), *decodedBasicTypes.myFloat_ref());
    }
    if (std::isnan(*basicTypes.myDouble_ref())) {
      EXPECT_TRUE(std::isnan(*decodedBasicTypes.myDouble_ref()));
    } else {
      EXPECT_DOUBLE_EQ(
          *basicTypes.myDouble_ref(), *decodedBasicTypes.myDouble_ref());
    }
  }
}

TEST(NimbleProtocolTest, StringTypesTest) {
  int kNumStrs = 1000;
  int kMaxStrLength = 1000;

  std::minstd_rand gen;
  std::uniform_int_distribution<char> charDist;
  std::uniform_int_distribution<> sizeDist(0, kMaxStrLength);
  auto randString = [&] {
    std::string result(sizeDist(gen), '\0');
    std::generate(result.begin(), result.end(), [&] { return charDist(gen); });
    return result;
  };

  // random strings with varied length
  std::vector<std::string> binaryBytes(kNumStrs);
  std::generate(binaryBytes.begin(), binaryBytes.end(), randString);

  // some other random strings
  binaryBytes.push_back("");
  binaryBytes.push_back("I am a test string with \0 character.");
  // long string
  std::string temp(1000 * 1000, '\0');
  std::generate(temp.begin(), temp.end(), [&] { return charDist(gen); });
  binaryBytes.push_back(temp);

  for (auto& str : binaryBytes) {
    StringTypes strTypes;
    *strTypes.myStr_ref() = str;
    *strTypes.myBinary_ref() = str;
    NimbleProtocolWriter writer;
    strTypes.write(&writer);

    std::unique_ptr<folly::IOBuf> message = writer.finalize();
    NimbleProtocolReader reader;
    reader.setInput(folly::io::Cursor{message.get()});

    StringTypes decodedStrTypes;
    decodedStrTypes.read(&reader);
    EXPECT_EQ(*strTypes.myStr_ref(), *decodedStrTypes.myStr_ref());
    EXPECT_EQ(*strTypes.myBinary_ref(), *decodedStrTypes.myBinary_ref());
  }
}

TEST(NimbleProtocolTest, BinaryTypeTest) {
  // test IOBuf
  StringTypes strTypes;
  auto buf = folly::IOBuf::copyBuffer("Testing;; Foo bar rand0m $tring.");
  *strTypes.myIOBuf_ref() = std::move(buf);
  NimbleProtocolWriter writer;
  strTypes.write(&writer);

  std::unique_ptr<folly::IOBuf> message = writer.finalize();
  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{message.get()});

  StringTypes decodedStrTypes;
  decodedStrTypes.read(&reader);
  EXPECT_EQ(
      (*strTypes.myIOBuf_ref())->computeChainDataLength(),
      (*decodedStrTypes.myIOBuf_ref())->computeChainDataLength());
  // content of the IOBufs should be the same
  (*strTypes.myIOBuf_ref())->coalesce();
  std::string orig = std::string(
      reinterpret_cast<const char*>((*strTypes.myIOBuf_ref())->data()),
      (*strTypes.myIOBuf_ref())->length());

  (*decodedStrTypes.myIOBuf_ref())->coalesce();
  std::string decoded = std::string(
      reinterpret_cast<const char*>((*decodedStrTypes.myIOBuf_ref())->data()),
      (*decodedStrTypes.myIOBuf_ref())->length());
  EXPECT_EQ(orig, decoded);
}

template <typename T>
void generateCollectionData(
    const std::vector<T>& interestingVals,
    std::vector<T>& out,
    int numContainers) {
  std::minstd_rand gen;
  std::uniform_int_distribution<> dist(0, interestingVals.size() - 1);
  auto generator = [&] { return interestingVals[dist(gen)]; };
  out.resize(numContainers);
  std::generate(out.begin(), out.end(), generator);
}

TEST(NimbleProtocolTest, ContainerTest) {
  int kNumContainers = 5;

  auto interestingIntMaps = std::vector<std::map<int, int>>{
      {},
      {{13, 100}, {14, 200}, {15, 300}, {62, 224}},
      {{-219, 13}, {0, 67280}, {INT16_MAX, -3}, {7, INT32_MIN}},
      {{INT32_MAX, 8713}, {6789, 2}, {-1, -1}}};
  std::vector<std::map<int, int>> intMaps;
  generateCollectionData<std::map<int, int>>(
      interestingIntMaps, intMaps, kNumContainers);

  auto interestingStrMaps = std::vector<std::map<std::string, std::string>>{
      {},
      {{"foo", "1"}, {"bar", "20{Pyye3"}, {"baz", "mehhhhhhhhhh1wea65536"}},
      {{"facEb00k!", ""},
       {"]apego", "als3@#1"},
       {"6e1 f", "m;l3aa3"},
       {"", "  "}}};
  std::vector<std::map<std::string, std::string>> stringMaps;
  generateCollectionData<std::map<std::string, std::string>>(
      interestingStrMaps, stringMaps, kNumContainers);

  auto interestingIntToStrMaps = std::vector<std::map<int, std::string>>{
      {},
      {{3, "yo"}, {50, "heh"}, {250, "suslke@1ks"}, {-1, "yay!"}},
      {{-1, " "}, {INT32_MIN, "hemahemahema"}, {594, "wuekabasice"}}};
  std::vector<std::map<int, std::string>> myMaps;
  generateCollectionData<std::map<int, std::string>>(
      interestingIntToStrMaps, myMaps, kNumContainers);

  auto interestingIntLists = std::vector<std::vector<int>>{
      {}, {1, 2, 312, 8773, -15}, {0, -1021, INT32_MAX, 6551}};
  std::vector<std::vector<int>> intLists;
  generateCollectionData<std::vector<int>>(
      interestingIntLists, intLists, kNumContainers);

  auto interestingStrLists = std::vector<std::vector<std::string>>{
      {},
      {"foooo", "$!someRandomeStringgg", "socialImpact^", "bar", "examples&1"}};
  std::vector<std::vector<std::string>> stringLists;
  generateCollectionData<std::vector<std::string>>(
      interestingStrLists, stringLists, kNumContainers);

  auto interestingListsOfLists = std::vector<std::vector<std::vector<double>>>{
      {},
      {{31.5, 0.00001, -12.3456},
       {525.109, -0.111, 7729.19101},
       {52.1, 0.000241, -42.32}}};
  std::vector<std::vector<std::vector<double>>> listOfLists;
  generateCollectionData<std::vector<std::vector<double>>>(
      interestingListsOfLists, listOfLists, kNumContainers);

  auto interestingI16Sets = std::vector<std::set<int16_t>>{
      {}, {122, 332, 19, 0, -1}, {INT16_MAX, -21, INT16_MIN, 101}};
  std::vector<std::set<int16_t>> I16Sets;
  generateCollectionData<std::set<int16_t>>(
      interestingI16Sets, I16Sets, kNumContainers);

  auto interestingStrSets = std::vector<std::set<std::string>>{
      {}, {"slkje", "ye21!(&", "apps@", "avengers"}, {"", " ", "  "}};
  std::vector<std::set<std::string>> stringSet;
  generateCollectionData<std::set<std::string>>(
      interestingStrSets, stringSet, kNumContainers);

  auto interestingListsOfMaps =
      std::vector<std::vector<std::map<int, std::string>>>{
          {},
          {{{3, "yo"}, {50, "heh"}},
           {{66, "groups"}, {4, "amazing!"}, {999, "message"}}}};
  std::vector<std::vector<std::map<int, std::string>>> listOfMaps;
  generateCollectionData<std::vector<std::map<int, std::string>>>(
      interestingListsOfMaps, listOfMaps, kNumContainers);

  for (int i = 0; i < kNumContainers; i++) {
    ContainerTypes containerTypes;
    *containerTypes.myIntMap_ref() = intMaps[i];
    *containerTypes.myStringMap_ref() = stringMaps[i];
    *containerTypes.myMap_ref() = myMaps[i];
    *containerTypes.myIntList_ref() = intLists[i];
    *containerTypes.myStringList_ref() = stringLists[i];
    *containerTypes.myListOfList_ref() = listOfLists[i];
    *containerTypes.myI16Set_ref() = I16Sets[i];
    *containerTypes.myStringSet_ref() = stringSet[i];
    *containerTypes.myListOfMap_ref() = listOfMaps[i];
    NimbleProtocolWriter writer;
    containerTypes.write(&writer);

    std::unique_ptr<folly::IOBuf> message = writer.finalize();
    NimbleProtocolReader reader;
    reader.setInput(folly::io::Cursor{message.get()});

    ContainerTypes decodedType;
    decodedType.read(&reader);
    EXPECT_EQ(*containerTypes.myIntMap_ref(), *decodedType.myIntMap_ref());
    EXPECT_EQ(
        *containerTypes.myStringMap_ref(), *decodedType.myStringMap_ref());
    EXPECT_EQ(*containerTypes.myMap_ref(), *decodedType.myMap_ref());
    EXPECT_EQ(*containerTypes.myIntList_ref(), *decodedType.myIntList_ref());
    EXPECT_EQ(
        *containerTypes.myStringList_ref(), *decodedType.myStringList_ref());
    EXPECT_EQ(
        *containerTypes.myListOfList_ref(), *decodedType.myListOfList_ref());
    EXPECT_EQ(*containerTypes.myI16Set_ref(), *decodedType.myI16Set_ref());
    EXPECT_EQ(
        *containerTypes.myStringSet_ref(), *decodedType.myStringSet_ref());
    EXPECT_EQ(
        *containerTypes.myListOfMap_ref(), *decodedType.myListOfMap_ref());
  }
}

TEST(NimbleProtocolTest, BigStringTest) {
  // test with a string longer than 2**28
  uint32_t stringSize = 1U << 30;
  StringTypes strTypes;
  *strTypes.myStr_ref() = std::string(stringSize, '1');
  NimbleProtocolWriter writer;
  strTypes.write(&writer);

  std::unique_ptr<folly::IOBuf> message = writer.finalize();
  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{message.get()});

  StringTypes decodedStrTypes;
  decodedStrTypes.read(&reader);
  EXPECT_EQ(*strTypes.myStr_ref(), *decodedStrTypes.myStr_ref());
}

TEST(NimbleProtocolTest, BigListTest) {
  // test with a list whose size is greater than 2**24
  ContainerTypes containerTypes;
  *containerTypes.myIntList_ref() = std::vector<int32_t>(1U << 24, 0);
  NimbleProtocolWriter writer;
  containerTypes.write(&writer);

  std::unique_ptr<folly::IOBuf> message = writer.finalize();
  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{message.get()});

  ContainerTypes decodedType;
  decodedType.read(&reader);
  EXPECT_EQ(*containerTypes.myIntList_ref(), *decodedType.myIntList_ref());
}

TEST(NimbleProtocolTest, StructOfStructsTest) {
  // some test structs
  BasicTypes basicTypes;
  *basicTypes.myByte_ref() = folly::Random::rand32(256) - 128;
  *basicTypes.myInt16_ref() = folly::Random::rand32(65536) - 32768;
  *basicTypes.myInt32_ref() = folly::Random::rand32() - 2147483648;
  *basicTypes.myInt64_ref() = folly::Random::rand64(INT64_MIN, INT64_MAX);
  *basicTypes.myBool_ref() = folly::Random::rand32(1);
  *basicTypes.myUint8_ref() = folly::Random::rand32(256);
  *basicTypes.myUint16_ref() = folly::Random::rand32(65536);
  *basicTypes.myUint32_ref() = folly::Random::rand32();
  *basicTypes.myUint64_ref() = folly::Random::rand64();
  *basicTypes.myFloat_ref() = 0.0001;
  *basicTypes.myDouble_ref() = std::numeric_limits<double>::infinity();

  StringTypes strTypes;
  *strTypes.myStr_ref() = "somerandomestring with *@(!)$+_characters:>,<";
  *strTypes.myBinary_ref() = "test random string";

  ContainerTypes containerTypes;
  *containerTypes.myIntMap_ref() = {{6, 10012}, {7901, 54298}, {-21, 342001}};
  *containerTypes.myStringMap_ref() = {
      {"foo", "oops"}, {"bar", "20"}, {"baz", "meh"}};
  *containerTypes.myMap_ref() = {{3, "yo"}, {50, "heh"}};
  *containerTypes.myIntList_ref() = {1, 2, 312, 8773, -15};
  *containerTypes.myStringList_ref() = {
      "foooo", "$!someRandomeStringgg", "socialImpact^", "bar", "examples&1"};
  *containerTypes.myListOfList_ref() = {{341.5, 0.00001, -12.3456},
                                        {25.109, -0.111, 7129.00101},
                                        {512.17171, 0.0100241, -42.32}};
  *containerTypes.myI16Set_ref() = {1212, -332, 19, 0, -1, 6667};
  *containerTypes.myStringSet_ref() = {"slkje", "ye21!(&", "apps@", "avengers"};
  *containerTypes.myListOfMap_ref() = {{{3, "yo"}, {50, "heh"}},
                                       {{65, "yoyoyo"}, {189, "heh>!@$S666"}}};
  StructOfStruct structOfStructs;
  *structOfStructs.basicTypes_ref() = basicTypes;
  *structOfStructs.strTypes_ref() = strTypes;
  *structOfStructs.containerTypes_ref() = containerTypes;
  *structOfStructs.myStr_ref() = "omg!#@#TTS1";
  *structOfStructs.myMap_ref() = {{"so many tests", 3198099},
                                  {"ohhahaha", -1992948}};

  NimbleProtocolWriter writer;
  structOfStructs.write(&writer);

  std::unique_ptr<folly::IOBuf> message = writer.finalize();
  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{message.get()});

  StructOfStruct decodedStruct;
  decodedStruct.read(&reader);
  EXPECT_EQ(*structOfStructs.basicTypes_ref(), *decodedStruct.basicTypes_ref());
  EXPECT_EQ(
      *structOfStructs.strTypes_ref()->myStr_ref(),
      *decodedStruct.strTypes_ref()->myStr_ref());
  EXPECT_EQ(
      *structOfStructs.strTypes_ref()->myBinary_ref(),
      *decodedStruct.strTypes_ref()->myBinary_ref());
  EXPECT_EQ(
      *structOfStructs.containerTypes_ref(),
      *decodedStruct.containerTypes_ref());
  EXPECT_EQ(*structOfStructs.myStr_ref(), *decodedStruct.myStr_ref());
  EXPECT_EQ(*structOfStructs.myMap_ref(), *decodedStruct.myMap_ref());
}

TEST(NimbleProtocolTest, UnionTest) {
  SimpleUnion myUnion;
  myUnion.set_simpleI32(729);

  NimbleProtocolWriter writer;
  myUnion.write(&writer);

  std::unique_ptr<folly::IOBuf> message = writer.finalize();
  NimbleProtocolReader reader;
  reader.setInput(folly::io::Cursor{message.get()});

  SimpleUnion decodedUnion;
  decodedUnion.read(&reader);

  EXPECT_EQ(myUnion.get_simpleI32(), decodedUnion.get_simpleI32());
}
} // namespace detail
} // namespace thrift
} // namespace apache
