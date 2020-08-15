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

#include <folly/container/Foreach.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/BadFieldAccess.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/test/gen-cpp2/optionals_types.h>
#include <thrift/test/gen-cpp2/optionals_types_custom_protocol.h>

using namespace apache::thrift;

template <class T>
static std::string objToJSON(T& obj) {
  SimpleJSONProtocolWriter writer;
  const auto size = obj.serializedSize(&writer);
  folly::IOBufQueue queue(IOBufQueue::cacheChainLength());
  writer.setOutput(&queue, size);
  obj.write(&writer);
  auto buf = queue.move();
  auto ret = buf->moveToFbString().toStdString();
  return ret;
}

template <class T>
static T jsonToObj(const std::string& json) {
  SimpleJSONProtocolReader reader;
  T ret;
  ret.__clear();
  auto iobuf = folly::IOBuf::copyBuffer(json);
  reader.setInput(iobuf.get());
  ret.read(&reader);
  return ret;
}

TEST(OptionalsTest, SerDesTests) {
  std::string json1;
  std::string json2;

  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;
  obj1.__clear();
  obj2.__clear();

  // first try with only the default fields, leave all optionals empty
  obj1.int64Def_ref() = 42;
  obj1.stringDef_ref() = "hello";
  obj1.setDef_ref() = std::set<int64_t>{10, 20, 30};
  obj1.listDef_ref() = std::vector<int64_t>{40, 50, 60};
  obj1.mapDef_ref() = std::map<int64_t, int64_t>{{100, 101}, {102, 103}};
  obj1.enumDef_ref() = cpp2::HasOptionalsTestEnum::FOO;
  obj1.structDef_ref() = cpp2::HasOptionalsExtra();
  obj1.structDef_ref()->__clear();
  obj1.structDef_ref()->extraInt64Def_ref() = 69;
  obj1.structDef_ref()->extraStringDef_ref() = "world";
  obj1.structDef_ref()->extraSetDef_ref() = std::set<int64_t>{210, 220, 230};
  obj1.structDef_ref()->extraListDef_ref() =
      std::vector<int64_t>{240, 250, 260};
  obj1.structDef_ref()->extraMapDef_ref() =
      std::map<int64_t, int64_t>{{1000, 1001}, {1002, 1003}};
  obj1.structDef_ref()->extraEnumDef_ref() = cpp2::HasOptionalsTestEnum::BAR;
  json1 = objToJSON(obj1);

  obj2 = jsonToObj<cpp2::HasOptionals>(json1);

  EXPECT_EQ(obj1, obj2);
  json2 = objToJSON(obj2);
  EXPECT_EQ(json1, json2);

  // Then try with the required fields, leave all optionals empty
  obj1.int64Req_ref() = 42;
  obj1.stringReq_ref() = "helloREQUIRED";
  obj1.setReq_ref() = std::set<int64_t>{10, 20, 30};
  obj1.listReq_ref() = std::vector<int64_t>{40, 50, 60};
  obj1.mapReq_ref() = std::map<int64_t, int64_t>{{100, 101}, {102, 103}};
  obj1.enumReq_ref() = cpp2::HasOptionalsTestEnum::FOO;
  obj1.structReq_ref() = cpp2::HasOptionalsExtra();
  obj1.structReq_ref()->__clear();
  obj1.structReq_ref()->extraInt64Req_ref() = 69;
  obj1.structReq_ref()->extraStringReq_ref() = "world";
  obj1.structReq_ref()->extraSetReq_ref() = std::set<int64_t>{210, 220, 230};
  obj1.structReq_ref()->extraListReq_ref() =
      std::vector<int64_t>{240, 250, 260};
  obj1.structReq_ref()->extraMapReq_ref() =
      std::map<int64_t, int64_t>{{1000, 1001}, {1002, 1003}};
  obj1.structReq_ref()->extraEnumReq_ref() = cpp2::HasOptionalsTestEnum::BAR;
  json1 = objToJSON(obj1);

  obj2 = jsonToObj<cpp2::HasOptionals>(json1);

  EXPECT_EQ(obj1, obj2);
  json2 = objToJSON(obj2);
  EXPECT_EQ(json1, json2);

  // now set optionals
  obj1.int64Opt_ref() = 42;
  obj1.stringOpt_ref().value_unchecked() = "helloOPTIONAL";
  obj1.setOpt_ref() = std::set<int64_t>{10, 20, 30};
  obj1.listOpt_ref() = std::vector<int64_t>{40, 50, 60};
  obj1.mapOpt_ref() = std::map<int64_t, int64_t>{{100, 101}, {102, 103}};
  obj1.enumOpt_ref() = cpp2::HasOptionalsTestEnum::FOO;
  obj1.structOpt_ref() = cpp2::HasOptionalsExtra();
  obj1.structOpt_ref()->__clear();
  obj1.structOpt_ref()->extraInt64Opt_ref() = 69;
  obj1.structOpt_ref()->extraStringOpt_ref() = "world";
  obj1.structOpt_ref()->extraSetOpt_ref() = std::set<int64_t>{210, 220, 230};
  obj1.structOpt_ref()->extraListOpt_ref() =
      std::vector<int64_t>{240, 250, 260};
  obj1.structOpt_ref()->extraMapOpt_ref() =
      std::map<int64_t, int64_t>{{1000, 1001}, {1002, 1003}};
  obj1.structOpt_ref()->extraEnumOpt_ref() = cpp2::HasOptionalsTestEnum::BAR;

  // Note: we did NOT set all the __isset's for the above!
  // Verify optionals WITHOUT isset are not serialized.
  json1 = objToJSON(obj1);
  EXPECT_EQ(std::string::npos, json1.find("helloOPTIONAL"));

  // ok, set the __isset's properly
  obj1.__isset.stringOpt = true;

  json1 = objToJSON(obj1);
  EXPECT_NE(std::string::npos, json1.find("helloOPTIONAL"));
  obj2 = jsonToObj<cpp2::HasOptionals>(json1);
  EXPECT_EQ(obj1, obj2);
  json2 = objToJSON(obj2);
  EXPECT_EQ(json1, json2);
}

TEST(OptionalsTest, ValueUncheckedTest) {
  cpp2::HasOptionals obj;
  obj.stringOpt_ref().value_unchecked() = "helloOPTIONAL";
  EXPECT_FALSE(obj.stringOpt_ref().has_value());
  EXPECT_THROW(obj.stringOpt_ref().value(), bad_field_access);
  EXPECT_EQ(obj.stringOpt_ref().value_unchecked(), "helloOPTIONAL");
}

TEST(OptionalsTest, EqualityTests) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;
  obj1.__clear();
  obj2.__clear();

  // for each of the fields:
  // * set a required field, expect equal.
  // * set an optional field on one; expect not equal.
  // * the the optional field on the other one; equal again.

  // both completely empty
  EXPECT_EQ(obj1, obj2);

  obj1.int64Def_ref() = 1;
  obj2.int64Def_ref() = 1;
  EXPECT_EQ(obj1, obj2);
  obj1.int64Req_ref() = 2;
  EXPECT_NE(obj1, obj2);
  obj2.int64Req_ref() = 2;
  EXPECT_EQ(obj1, obj2);
  obj1.int64Opt_ref() = 3;
  EXPECT_NE(obj1, obj2);
  obj2.int64Opt_ref() = 3;
  EXPECT_EQ(obj1, obj2);

  obj1.stringDef_ref() = "hello";
  obj2.stringDef_ref() = "hello";
  EXPECT_EQ(obj1, obj2);
  obj1.stringReq_ref() = "foo";
  EXPECT_NE(obj1, obj2);
  obj2.stringReq_ref() = "foo";
  EXPECT_EQ(obj1, obj2);
  obj1.stringOpt_ref() = "world";
  EXPECT_NE(obj1, obj2);
  obj2.stringOpt_ref() = "world";
  EXPECT_EQ(obj1, obj2);

  obj1.setDef_ref() = std::set<int64_t>{1, 2};
  obj2.setDef_ref() = std::set<int64_t>{1, 2};
  EXPECT_EQ(obj1, obj2);
  obj1.setReq_ref() = std::set<int64_t>{3, 4};
  EXPECT_NE(obj1, obj2);
  obj2.setReq_ref() = std::set<int64_t>{3, 4};
  EXPECT_EQ(obj1, obj2);
  obj1.setOpt_ref() = std::set<int64_t>{5, 6};
  EXPECT_NE(obj1, obj2);
  obj2.setOpt_ref() = std::set<int64_t>{5, 6};
  EXPECT_EQ(obj1, obj2);

  obj1.listDef_ref() = std::vector<int64_t>{7, 8};
  obj2.listDef_ref() = std::vector<int64_t>{7, 8};
  EXPECT_EQ(obj1, obj2);
  obj1.listReq_ref() = std::vector<int64_t>{9, 10};
  EXPECT_NE(obj1, obj2);
  obj2.listReq_ref() = std::vector<int64_t>{9, 10};
  EXPECT_EQ(obj1, obj2);
  obj1.listOpt_ref() = std::vector<int64_t>{11, 12};
  EXPECT_NE(obj1, obj2);
  obj2.listOpt_ref() = std::vector<int64_t>{11, 12};
  EXPECT_EQ(obj1, obj2);

  obj1.mapDef_ref() = std::map<int64_t, int64_t>{{13, 14}, {15, 16}};
  obj2.mapDef_ref() = std::map<int64_t, int64_t>{{13, 14}, {15, 16}};
  EXPECT_EQ(obj1, obj2);
  obj1.mapReq_ref() = std::map<int64_t, int64_t>{{17, 18}, {19, 20}};
  EXPECT_NE(obj1, obj2);
  obj2.mapReq_ref() = std::map<int64_t, int64_t>{{17, 18}, {19, 20}};
  EXPECT_EQ(obj1, obj2);
  obj1.mapOpt_ref() = std::map<int64_t, int64_t>{{21, 22}, {23, 24}};
  EXPECT_NE(obj1, obj2);
  obj2.mapOpt_ref() = std::map<int64_t, int64_t>{{21, 22}, {23, 24}};
  EXPECT_EQ(obj1, obj2);

  obj1.enumDef_ref() = cpp2::HasOptionalsTestEnum::FOO;
  obj2.enumDef_ref() = cpp2::HasOptionalsTestEnum::FOO;
  EXPECT_EQ(obj1, obj2);
  obj1.enumReq_ref() = cpp2::HasOptionalsTestEnum::BAR;
  EXPECT_NE(obj1, obj2);
  obj2.enumReq_ref() = cpp2::HasOptionalsTestEnum::BAR;
  EXPECT_EQ(obj1, obj2);
  obj1.enumOpt_ref() = cpp2::HasOptionalsTestEnum::BAZ;
  EXPECT_NE(obj1, obj2);
  obj2.enumOpt_ref() = cpp2::HasOptionalsTestEnum::BAZ;
  EXPECT_EQ(obj1, obj2);

  obj1.structDef_ref() = cpp2::HasOptionalsExtra();
  obj1.structDef_ref()->__clear();
  obj2.structDef_ref() = cpp2::HasOptionalsExtra();
  obj2.structDef_ref()->__clear();
  EXPECT_EQ(obj1, obj2);
  obj1.structReq_ref() = cpp2::HasOptionalsExtra();
  obj1.structReq_ref()->__clear();
  obj2.structReq_ref() = cpp2::HasOptionalsExtra();
  obj2.structReq_ref()->__clear();
  EXPECT_EQ(obj1, obj2);
  obj1.structOpt_ref() = cpp2::HasOptionalsExtra();
  obj1.structOpt_ref()->__clear();
  obj1.__isset.structOpt = true;
  EXPECT_NE(obj1, obj2);
  obj2.structOpt_ref() = cpp2::HasOptionalsExtra();
  obj2.structOpt_ref()->__clear();
  obj2.__isset.structOpt = true;
  EXPECT_EQ(obj1, obj2);

  // just one more test: try required/optional fields in the optional struct
  // to verify that recursive checking w/ optional fields works.
  // Don't bother testing all the nested struct's fields, this is enough.
  obj1.structOpt_ref()->extraInt64Opt_ref() = 666;
  obj2.structOpt_ref()->extraInt64Opt_ref() = 666;
  EXPECT_EQ(obj1, obj2);

  obj1.structOpt_ref()->extraInt64Opt_ref() = 1;
  EXPECT_NE(obj1, obj2);
  obj2.structOpt_ref()->extraInt64Opt_ref() = 1;
  EXPECT_EQ(obj1, obj2);

  obj1.structOpt_ref()->extraInt64Def_ref() = 2;
  EXPECT_NE(obj1, obj2);
  obj2.structOpt_ref()->extraInt64Def_ref() = 2;
  EXPECT_EQ(obj1, obj2);

  obj1.structOpt_ref()->extraInt64Req_ref() = 3;
  EXPECT_NE(obj1, obj2);
  obj2.structOpt_ref()->extraInt64Req_ref() = 3;
  EXPECT_EQ(obj1, obj2);

  obj1.structDef_ref()->extraInt64Opt_ref() = 4;
  EXPECT_NE(obj1, obj2);
  obj2.structDef_ref()->extraInt64Opt_ref() = 4;
  EXPECT_EQ(obj1, obj2);

  obj1.structDef_ref()->extraInt64Def_ref() = 5;
  EXPECT_NE(obj1, obj2);
  obj2.structDef_ref()->extraInt64Def_ref() = 5;
  EXPECT_EQ(obj1, obj2);

  obj1.structDef_ref()->extraInt64Req_ref() = 6;
  EXPECT_NE(obj1, obj2);
  obj2.structDef_ref()->extraInt64Req_ref() = 6;
  EXPECT_EQ(obj1, obj2);

  obj1.structReq_ref()->extraInt64Opt_ref() = 7;
  EXPECT_NE(obj1, obj2);
  obj2.structReq_ref()->extraInt64Opt_ref() = 7;
  EXPECT_EQ(obj1, obj2);

  obj1.structReq_ref()->extraInt64Def_ref() = 8;
  EXPECT_NE(obj1, obj2);
  obj2.structReq_ref()->extraInt64Def_ref() = 8;
  EXPECT_EQ(obj1, obj2);

  obj1.structReq_ref()->extraInt64Req_ref() = 9;
  EXPECT_NE(obj1, obj2);
  obj2.structReq_ref()->extraInt64Req_ref() = 9;
  EXPECT_EQ(obj1, obj2);
}

TEST(OptionalsTest, emplace) {
  cpp2::HasOptionals obj;
  folly::for_each(
      std::make_tuple(
          obj.stringOpt_ref(), obj.stringReq_ref(), obj.stringDef_ref()),
      [](auto&& i) {
        EXPECT_EQ(i.emplace(3, 'a'), "aaa");
        EXPECT_EQ(i.value(), "aaa");
        EXPECT_EQ(i.emplace(3, 'b'), "bbb");
        EXPECT_EQ(i.value(), "bbb");
        i.emplace() = "ccc";
        EXPECT_EQ(i.value(), "ccc");
        EXPECT_THROW(i.emplace(std::string(""), 1), std::out_of_range);
        if (typeid(i) ==
            typeid(apache::thrift::required_field_ref<std::string&>)) {
          // Required field always has value
          EXPECT_TRUE(i.has_value());
        } else {
          // C++ Standard requires *this to be empty if `emplace(...)` throws
          EXPECT_FALSE(i.has_value());
        }
      });
}

TEST(OptionalsTest, FollyOptionalConversion) {
  cpp2::HasOptionals obj;

  obj.int64Opt_ref() = 1;
  EXPECT_TRUE(obj.int64Opt_ref().has_value());
  EXPECT_EQ(obj.int64Opt_ref().value(), 1);

  folly::Optional<int64_t> f;
  fromFollyOptional(obj.int64Opt_ref(), f);
  EXPECT_FALSE(obj.int64Opt_ref().has_value());
  EXPECT_EQ(copyToFollyOptional(obj.int64Opt_ref()), f);

  fromFollyOptional(obj.int64Opt_ref(), f = 2);
  EXPECT_EQ(obj.int64Opt_ref().value(), 2);
  EXPECT_EQ(copyToFollyOptional(obj.int64Opt_ref()), f);

  auto foo = [](const folly::Optional<int64_t>& opt) { return opt; };
  EXPECT_EQ(foo(copyToFollyOptional(std::as_const(obj).int64Opt_ref())), f);

  fromFollyOptional(obj.int64Opt_ref(), folly::Optional<int64_t>{3});
  EXPECT_EQ(obj.int64Opt_ref().value(), 3);

  static_assert(std::is_same_v<
                decltype(obj.int64Opt_ref()),
                apache::thrift::optional_field_ref<int64_t&>>);
  static_assert(std::is_same_v<
                decltype(copyToFollyOptional(obj.int64Opt_ref())),
                folly::Optional<int64_t>>);
  static_assert(std::is_same_v<
                decltype(moveToFollyOptional(obj.int64Opt_ref())),
                folly::Optional<int64_t>>);
  static_assert(
      std::is_same_v<
          decltype(copyToFollyOptional(std::as_const(obj).int64Opt_ref())),
          folly::Optional<int64_t>>);

  static_assert(!std::is_constructible_v<
                apache::thrift::optional_field_ref<int&>,
                folly::Optional<int>>);
  static_assert(!std::is_convertible_v<
                folly::Optional<int>,
                apache::thrift::optional_field_ref<int&>>);
}

TEST(DeprecatedOptionalField, NulloptComparisons) {
  cpp2::HasOptionals obj;

  EXPECT_TRUE(obj.int64Opt_ref() == std::nullopt);
  EXPECT_TRUE(std::nullopt == obj.int64Opt_ref());

  obj.int64Opt_ref() = 1;
  EXPECT_FALSE(obj.int64Opt_ref() == std::nullopt);
  EXPECT_FALSE(std::nullopt == obj.int64Opt_ref());

  obj.int64Opt_ref().reset();
  EXPECT_FALSE(obj.int64Opt_ref() != std::nullopt);
  EXPECT_FALSE(std::nullopt != obj.int64Opt_ref());

  obj.int64Opt_ref() = 1;
  EXPECT_TRUE(obj.int64Opt_ref() != std::nullopt);
  EXPECT_TRUE(std::nullopt != obj.int64Opt_ref());
}

TEST(TestWithFollyOptionals, equalToFollyOptional) {
  cpp2::HasOptionals obj;
  folly::Optional<int64_t> opt;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt_ref() = 1;
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt = 1;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt = 2;
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt_ref() = 2;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt_ref().reset();
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt.reset();
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));
}

TEST(OptionalsTest, Equality) {
  cpp2::HasOptionals obj;
  obj.int64Opt_ref() = 1;
  EXPECT_EQ(obj.int64Opt_ref(), 1);
  EXPECT_NE(obj.int64Opt_ref(), 2);
  EXPECT_EQ(1, obj.int64Opt_ref());
  EXPECT_NE(2, obj.int64Opt_ref());
  obj.int64Opt_ref().reset();
  EXPECT_NE(obj.int64Opt_ref(), 1);
  EXPECT_NE(1, obj.int64Opt_ref());
}

TEST(OptionalsTest, Comparison) {
  cpp2::HasOptionals obj;
  obj.int64Opt_ref() = 2;
  EXPECT_LT(obj.int64Opt_ref(), 3);
  EXPECT_LE(obj.int64Opt_ref(), 2);
  EXPECT_LE(obj.int64Opt_ref(), 3);
  EXPECT_LT(1, obj.int64Opt_ref());
  EXPECT_LE(1, obj.int64Opt_ref());
  EXPECT_LE(2, obj.int64Opt_ref());

  EXPECT_GT(obj.int64Opt_ref(), 1);
  EXPECT_GE(obj.int64Opt_ref(), 1);
  EXPECT_GE(obj.int64Opt_ref(), 2);
  EXPECT_GT(3, obj.int64Opt_ref());
  EXPECT_GE(2, obj.int64Opt_ref());
  EXPECT_GE(3, obj.int64Opt_ref());

  obj.int64Opt_ref().reset();
  EXPECT_LT(obj.int64Opt_ref(), -1);
  EXPECT_LE(obj.int64Opt_ref(), -1);
  EXPECT_GT(-1, obj.int64Opt_ref());
  EXPECT_GE(-1, obj.int64Opt_ref());
}

TEST(OptionalsTest, UnsetUnsafe) {
  cpp2::HasOptionals obj;
  EXPECT_FALSE(obj.int64Def_ref().has_value());
  obj.int64Def_ref() = 1;
  EXPECT_TRUE(obj.int64Def_ref().has_value());
  detail::unset_unsafe(obj.int64Def_ref());
  EXPECT_FALSE(obj.int64Def_ref().has_value());
}

TEST(OptionalsTest, RefForUnqualifiedField) {
  cpp2::HasOptionals obj;
  EXPECT_TRUE(obj.int64Req_ref().has_value());
  obj.int64Req_ref() = 42;
  EXPECT_TRUE(obj.int64Req_ref().has_value());
  EXPECT_EQ(obj.int64Req_ref(), 42);

  EXPECT_TRUE(obj.stringReq_ref().has_value());
  obj.stringReq_ref() = "foo";
  EXPECT_TRUE(obj.stringReq_ref().has_value());
  EXPECT_EQ(obj.stringReq_ref(), "foo");
}

TEST(OptionalsTest, MoveFrom) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  obj1.int64Opt_ref() = 1;
  obj2.int64Opt_ref().move_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt_ref(), 1);

  obj1.int64Opt_ref() = 2;
  obj2.int64Opt_ref().move_from(std::move(obj1).int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt_ref(), 2);

  obj1.int64Opt_ref() = 3;
  obj2.int64Opt_ref().move_from(std::move(obj1).int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt_ref().value(), 3);

  obj1.int64Opt_ref() = 4;
  obj2.int64Opt_ref().move_from(std::move(obj1).int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt_ref().value(), 4);
}

TEST(OptionalsTest, CopyFrom) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  obj1.int64Opt_ref() = 1;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt_ref().value(), 1);
  EXPECT_EQ(obj2.int64Opt_ref().value(), 1);

  obj1.int64Opt_ref() = 2;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt_ref().value(), 2);
  EXPECT_EQ(obj2.int64Opt_ref().value(), 2);

  obj1.int64Opt_ref() = 3;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt_ref().value(), 3);
  EXPECT_EQ(obj2.int64Opt_ref().value(), 3);

  obj1.int64Opt_ref() = 4;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt_ref().value(), 4);
  EXPECT_EQ(obj2.int64Opt_ref().value(), 4);
}

TEST(OptionalsTest, AddRef) {
  cpp2::HasOptionals obj;
  static_assert(std::is_same_v<
                decltype(obj.int64Opt_ref()),
                optional_field_ref<int64_t&>>);
  static_assert(std::is_same_v<
                decltype(std::as_const(obj).int64Opt_ref()),
                optional_field_ref<const int64_t&>>);
  static_assert(std::is_same_v<
                decltype(std::move(obj).int64Opt_ref()),
                optional_field_ref<int64_t&&>>);
  static_assert(std::is_same_v<
                decltype(std::move(std::as_const(obj)).int64Opt_ref()),
                optional_field_ref<const int64_t&&>>);
  obj.int64Opt_ref() = 42;
  EXPECT_EQ(obj.int64Opt_ref(), 42);
  auto value = std::map<int64_t, int64_t>{{1, 2}};
  obj.mapOpt_ref() = value;
  EXPECT_EQ(*obj.mapOpt_ref(), value);
}
