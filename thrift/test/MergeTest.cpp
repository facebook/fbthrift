/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <gtest/gtest.h>
#include <string>

#include <folly/Format.h>
#include <thrift/test/gen-cpp/OptionalRequiredTest_types.h>
#include <thrift/test/gen-cpp/ThriftTest_types.h>

using namespace apache::thrift;
using namespace thrift::test;
using std::map;
using std::set;
using std::string;
using std::vector;
using std::unordered_map;
using std::unordered_set;

namespace {

void setAll(Xtruct& xtruct) {
  xtruct.__isset.string_thing = true;
  xtruct.__isset.byte_thing = true;
  xtruct.__isset.i32_thing = true;
  xtruct.__isset.i64_thing = true;
}

void setAll(Xtruct2& xtruct) {
  xtruct.__isset.byte_thing = true;
  xtruct.__isset.struct_thing = true;
  xtruct.__isset.i32_thing = true;
}

void setAll(Xtruct3& xtruct) {
  xtruct.__isset.string_thing = true;
  xtruct.__isset.changed = true;
  xtruct.__isset.i32_thing = true;
  xtruct.__isset.i64_thing = true;
}

void setAll(Maps& m) {
  m.__isset.str2str = true;
  m.__isset.str2list = true;
  m.__isset.str2map = true;
  m.__isset.str2struct = true;
}

Insanity makeInsanity(
    int8_t insanityDegree,
    const std::vector<std::pair<std::string, std::string>>& keyValues = {}) {
  Insanity ret;
  ret.userMap[static_cast<Numberz>((insanityDegree - 1)  % 8 + 1)] =
    insanityDegree;
  ret.userMap[static_cast<Numberz>(insanityDegree % 8 + 1)] =
    insanityDegree;
  ret.__isset.userMap = true;

  for (const auto& kv : keyValues) {
    ret.str2str[kv.first] = kv.second;
  }
  ret.__isset.str2str = !keyValues.empty();

  for (int i = 0; i < insanityDegree; i++) {
    ret.xtructs.emplace_back();
    Xtruct& last = ret.xtructs.back();
    last.string_thing =
      folly::format("insanity.xtruct.{}", insanityDegree).str();
    last.byte_thing = insanityDegree;
    last.i32_thing = insanityDegree;
    last.i64_thing = insanityDegree;
    setAll(last);
  }
  ret.__isset.xtructs = true;
  return ret;
}


}  // !namespace

TEST(MergeTest, Basic) {
  Xtruct3 mergeTo;
  mergeTo.string_thing = "mergeTo";
  mergeTo.changed = 0;
  mergeTo.i32_thing = 123;
  mergeTo.i64_thing = 456;
  setAll(mergeTo);

  Xtruct3 mergeFrom;
  mergeFrom.string_thing = "mergeFrom";
  mergeFrom.changed = 1;
  mergeFrom.i32_thing = 789;
  mergeFrom.i64_thing = 1011;
  setAll(mergeFrom);

  merge(mergeFrom, mergeTo);
  EXPECT_EQ(mergeFrom, mergeTo);
}

TEST(MergeTest, Container) {
  Insanity mergeTo =
      makeInsanity(1, {{"nepal", "prabal"}, {"italy", "gianni"}});
  Insanity mergeFrom =
      makeInsanity(2, {{"germany", "karl"}, {"italy", "giorgio"}});
  merge(mergeFrom, mergeTo);

  map<Numberz, UserId> expectedUserMap(
    {
      {Numberz::ONE, 1},     // old
      {Numberz::TWO, 2},    // overwrite
      {Numberz::THREE, 2},  // new
    });
  EXPECT_EQ(expectedUserMap, mergeTo.userMap);
  ASSERT_TRUE(mergeTo.__isset.userMap);

  unordered_map<string, string> expectedStr2Str({
      {"nepal", "prabal"}, // old
      {"italy", "giorgio"}, // overwrite
      {"germany", "karl"}, // new
  });
  EXPECT_EQ(expectedStr2Str, mergeTo.str2str);
  ASSERT_TRUE(mergeTo.__isset.str2str);

  ASSERT_EQ(1 + 2, mergeTo.xtructs.size());
  for (int i = 0; i < mergeTo.xtructs.size(); i++) {
    int8_t insanity = i == 0 ? 1 : 2;
    string string_thing = folly::format("insanity.xtruct.{}",  insanity).str();
    EXPECT_EQ(string_thing, mergeTo.xtructs[i].string_thing);
    EXPECT_EQ(insanity, mergeTo.xtructs[i].byte_thing);
    EXPECT_EQ(insanity, mergeTo.xtructs[i].i32_thing);
    EXPECT_EQ(insanity, mergeTo.xtructs[i].i64_thing);
  }

  std::set<std::string> setA{"alpha", "beta"}, setB{"beta", "gamma"};
  std::set<std::string> expectedSet{"alpha", "beta", "gamma"};
  merge(setA, setB);
  EXPECT_EQ(expectedSet, setB);

  std::unordered_set<std::string> usetA{"aleph", "bet"}, usetB{"bet", "gimel"};
  std::unordered_set<std::string> expectedUnorderedSet{"aleph", "bet", "gimel"};
  merge(usetA, usetB);
  EXPECT_EQ(expectedUnorderedSet, usetB);
}

TEST(MergeTest, Nested) {
  Xtruct2 mergeTo;
  mergeTo.byte_thing = 1;
  mergeTo.struct_thing.string_thing = "nested_xtruct_1";
  setAll(mergeTo);
  setAll(mergeTo.struct_thing);

  Xtruct2 mergeFrom;
  mergeFrom.byte_thing = 2;
  mergeFrom.struct_thing.string_thing = "nested_xtruct_2";
  setAll(mergeFrom);
  setAll(mergeFrom.struct_thing);

  merge(mergeFrom, mergeTo);
  EXPECT_EQ(mergeFrom, mergeTo);
}

TEST(MergeTest, OptionalField) {
  Simple mergeTo;
  mergeTo.im_default = 1;
  mergeTo.im_required = 1;
  mergeTo.im_optional = 1;

  Simple mergeFrom;
  mergeFrom.im_default = 2;
  mergeFrom.im_required = 2;
  mergeFrom.im_optional = 2;

  merge(mergeFrom, mergeTo);
  EXPECT_EQ(mergeFrom.im_default, mergeTo.im_default);
  EXPECT_EQ(mergeFrom.im_required, mergeTo.im_required);
  EXPECT_EQ(1, mergeTo.im_optional);
  EXPECT_FALSE(mergeTo.__isset.im_default);
  EXPECT_FALSE(mergeTo.__isset.im_optional);

  mergeFrom.__isset.im_default = true;
  mergeFrom.__isset.im_optional = true;
  merge(mergeFrom, mergeTo);
  EXPECT_EQ(mergeFrom.im_default, mergeTo.im_default);
  EXPECT_EQ(mergeFrom.im_required, mergeTo.im_required);
  EXPECT_EQ(mergeFrom.im_optional, mergeTo.im_optional);
  EXPECT_TRUE(mergeTo.__isset.im_default);
  EXPECT_TRUE(mergeTo.__isset.im_optional);
}

namespace {

template<typename M>
void verifyResursiveMergedMap(const M& from, const M& to, const M& result) {
  for (auto& kv : result) {
    bool inFrom = from.find(kv.first) != from.end();
    bool inTo = to.find(kv.first) != to.end();
    EXPECT_TRUE(inFrom || inTo);
    if (inFrom && inTo) {
      auto expected = to.at(kv.first);
      merge(from.at(kv.first), expected);
      EXPECT_EQ(expected, kv.second);
    } else if (inFrom) {
      EXPECT_EQ(from.at(kv.first), kv.second);
    } else {
      EXPECT_EQ(to.at(kv.first), kv.second);
    }
  }
}

}  // !namespace

TEST(MergeTest, Map) {
  Maps m1;
  m1.str2str["a"] = "a";
  m1.str2str["b"] = "b";
  m1.str2list["a"] = {"a"};
  m1.str2list["b"] = {"b"};
  m1.str2map["a"] = {{"a", "a"}, {"A", "A"}};
  m1.str2map["b"] = {{"b", "b"}, {"B", "B"}};
  m1.str2struct["a"] = makeInsanity(1);
  m1.str2struct["b"] = makeInsanity(2);
  setAll(m1);

  Maps m2;
  m2.str2str["b"] = "bb";
  m2.str2str["c"] = "cc";
  m2.str2list["b"] = {"bb"};
  m2.str2list["c"] = {"cc"};
  m2.str2map["b"] = {{"b", "bb"}, {"B", "BB"}};
  m2.str2map["c"] = {{"c", "cc"}, {"C", "CC"}};
  m2.str2struct["b"] = makeInsanity(3);
  m2.str2struct["c"] = makeInsanity(3);
  setAll(m2);

  Maps m12 = m2;  // Keep a copy of m2 for later verification.
  merge(m1, m12);

  // overwrite for primitive value
  for (auto& kv : m12.str2str) {
    bool inFrom = m1.str2str.find(kv.first) != m1.str2str.end();
    bool inTo = m2.str2str.find(kv.first) != m2.str2str.end();
    EXPECT_TRUE(inFrom || inTo);
    if (inFrom) {
      EXPECT_EQ(m1.str2str[kv.first], kv.second);
    } else {
      EXPECT_EQ(m2.str2str[kv.first], kv.second);
    }
  }

  // concatenate for list
  for (auto& kv : m12.str2list) {
    bool inFrom = m1.str2list.find(kv.first) != m1.str2list.end();
    bool inTo = m2.str2list.find(kv.first) != m2.str2list.end();
    EXPECT_TRUE(inFrom || inTo);
    if (inFrom && inTo) {
      vector<string> all(m2.str2list[kv.first]);
      all.insert(all.end(),
                 m1.str2list[kv.first].begin(), m1.str2list[kv.first].end());
      EXPECT_EQ(all, kv.second);
    } else if (inFrom) {
      EXPECT_EQ(m1.str2list[kv.first], kv.second);
    } else if (inTo) {
      EXPECT_EQ(m2.str2list[kv.first], kv.second);
    }
  }

  // recursive merging to submap and thrift
  verifyResursiveMergedMap(m1.str2map, m2.str2map, m12.str2map);
  verifyResursiveMergedMap(m1.str2struct, m2.str2struct, m12.str2struct);
}

TEST(MergeTest, RvalRef) {
  Insanity from = makeInsanity(3);
  Insanity to;
  // from includes non empty xstructs
  for (auto& xtr : from.xtructs) {
    EXPECT_FALSE(xtr.string_thing.empty());
    // small string will be inlined in fbstring platform, but it big to occupy
    // external buffer
    for (int i = 0; i < 8; i++) {
      xtr.string_thing = xtr.string_thing + xtr.string_thing;
    }
  }
  merge(std::move(from), to);
  EXPECT_EQ(3, to.xtructs.size());
  // to includes non empty xstructs now
  for (const auto& xtr : to.xtructs) {
    EXPECT_FALSE(xtr.string_thing.empty());
  }
  // from is destroyed to make the move
  for (const auto& xtr : from.xtructs) {
    EXPECT_TRUE(xtr.string_thing.empty());
  }
}
