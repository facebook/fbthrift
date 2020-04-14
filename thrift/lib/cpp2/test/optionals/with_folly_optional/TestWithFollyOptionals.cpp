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

#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/test/optionals/with_folly_optional/gen-cpp2/FollyOptionals_types.h>
#include <thrift/lib/cpp2/test/optionals/with_folly_optional/gen-cpp2/FollyOptionals_types_custom_protocol.h>

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
  auto iobuf = folly::IOBuf::copyBuffer(json);
  reader.setInput(iobuf.get());
  ret.read(&reader);
  return ret;
}

TEST(TestWithFollyOptionals, SerDesTests) {
  std::string json1;
  std::string json2;

  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  // first try with only the required fields, leave all optionals empty
  obj1.int64Req = 42;
  obj1.stringReq = "hello";
  obj1.setReq = std::set<int64_t>{10, 20, 30};
  obj1.listReq = std::vector<int64_t>{40, 50, 60};
  obj1.mapReq = std::map<int64_t, int64_t>{{100, 101}, {102, 103}};
  obj1.enumReq = cpp2::HasOptionalsTestEnum::FOO;
  obj1.structReq = cpp2::HasOptionalsExtra();
  obj1.structReq.extraInt64Req = 69;
  obj1.structReq.extraStringReq = "world";
  obj1.structReq.extraSetReq = std::set<int64_t>{210, 220, 230};
  obj1.structReq.extraListReq = std::vector<int64_t>{240, 250, 260};
  obj1.structReq.extraMapReq =
      std::map<int64_t, int64_t>{{1000, 1001}, {1002, 1003}};
  obj1.structReq.extraEnumReq = cpp2::HasOptionalsTestEnum::BAR;

  json1 = objToJSON(obj1);
  obj2 = jsonToObj<cpp2::HasOptionals>(json1);
  EXPECT_EQ(obj1, obj2);
  EXPECT_FALSE(obj2.int64Opt.has_value());
  EXPECT_FALSE(obj2.listOpt.has_value());
  EXPECT_FALSE(obj2.structOpt.has_value());
  json2 = objToJSON(obj2);
  EXPECT_EQ(json1, json2);

  obj1.int64Opt = 42;
  obj1.stringOpt = "helloOPTIONAL";
  obj1.setOpt = std::set<int64_t>{10, 20, 30};
  obj1.listOpt = std::vector<int64_t>{40, 50, 60};
  obj1.mapOpt = std::map<int64_t, int64_t>{{100, 101}, {102, 103}};
  obj1.enumOpt = cpp2::HasOptionalsTestEnum::FOO;
  obj1.structOpt = cpp2::HasOptionalsExtra();
  obj1.structOpt->extraInt64Opt = 69;
  obj1.structOpt->extraStringOpt = "world";
  obj1.structOpt->extraSetOpt = std::set<int64_t>{210, 220, 230};
  obj1.structOpt->extraListOpt = std::vector<int64_t>{240, 250, 260};
  obj1.structOpt->extraMapOpt =
      std::map<int64_t, int64_t>{{1000, 1001}, {1002, 1003}};
  obj1.structOpt->extraEnumOpt = cpp2::HasOptionalsTestEnum::BAR;

  json1 = objToJSON(obj1);
  EXPECT_NE(std::string::npos, json1.find("helloOPTIONAL"));
  obj2 = jsonToObj<cpp2::HasOptionals>(json1);
  EXPECT_EQ(obj1, obj2);
  json2 = objToJSON(obj2);
  EXPECT_EQ(json1, json2);
}

TEST(TestWithFollyOptionals, EqualityTests) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  // for each of the fields:
  // * set a required field, expect equal.
  // * set an optional field on one; expect not equal.
  // * the the optional field on the other one; equal again.

  // both completely empty
  EXPECT_EQ(obj1, obj2);

  obj1.int64Req = 1;
  obj2.int64Req = 1;
  EXPECT_EQ(obj1, obj2);
  obj1.int64Opt = 2;
  EXPECT_NE(obj1, obj2);
  obj2.int64Opt = 2;
  EXPECT_EQ(obj1, obj2);

  obj1.stringReq = "hello";
  obj2.stringReq = "hello";
  EXPECT_EQ(obj1, obj2);
  obj1.stringOpt = "world";
  EXPECT_NE(obj1, obj2);
  obj2.stringOpt = "world";
  EXPECT_EQ(obj1, obj2);

  obj1.setReq = std::set<int64_t>{1, 2};
  obj2.setReq = std::set<int64_t>{1, 2};
  EXPECT_EQ(obj1, obj2);
  obj1.setOpt = std::set<int64_t>{3, 4};
  EXPECT_NE(obj1, obj2);
  obj2.setOpt = std::set<int64_t>{3, 4};
  EXPECT_EQ(obj1, obj2);

  obj1.listReq = std::vector<int64_t>{5, 6};
  obj2.listReq = std::vector<int64_t>{5, 6};
  EXPECT_EQ(obj1, obj2);
  obj1.listOpt = std::vector<int64_t>{7, 8};
  EXPECT_NE(obj1, obj2);
  obj2.listOpt = std::vector<int64_t>{7, 8};
  EXPECT_EQ(obj1, obj2);

  obj1.mapReq = std::map<int64_t, int64_t>{{9, 10}, {11, 12}};
  obj2.mapReq = std::map<int64_t, int64_t>{{9, 10}, {11, 12}};
  EXPECT_EQ(obj1, obj2);
  obj1.mapOpt = std::map<int64_t, int64_t>{{13, 14}, {15, 16}};
  EXPECT_NE(obj1, obj2);
  obj2.mapOpt = std::map<int64_t, int64_t>{{13, 14}, {15, 16}};
  EXPECT_EQ(obj1, obj2);

  obj1.enumReq = cpp2::HasOptionalsTestEnum::FOO;
  obj2.enumReq = cpp2::HasOptionalsTestEnum::FOO;
  EXPECT_EQ(obj1, obj2);
  obj1.enumOpt = cpp2::HasOptionalsTestEnum::BAR;
  EXPECT_NE(obj1, obj2);
  obj2.enumOpt = cpp2::HasOptionalsTestEnum::BAR;
  EXPECT_EQ(obj1, obj2);

  obj1.structReq = cpp2::HasOptionalsExtra();
  obj2.structReq = cpp2::HasOptionalsExtra();
  EXPECT_EQ(obj1, obj2);
  obj1.structOpt = cpp2::HasOptionalsExtra();
  EXPECT_NE(obj1, obj2);
  obj2.structOpt = cpp2::HasOptionalsExtra();
  EXPECT_EQ(obj1, obj2);

  // just one more test: try required/optional fields in the optional struct
  // to verify that recursive checking w/ optional fields works.
  // Don't bother testing all the nested struct's fields, this is enough.
  obj1.structOpt->extraInt64Req = 666;
  obj2.structOpt->extraInt64Req = 666;
  EXPECT_EQ(obj1, obj2);
  obj1.structOpt->extraInt64Opt = 13;
  EXPECT_NE(obj1, obj2);
  obj2.structOpt->extraInt64Opt = 13;
  EXPECT_EQ(obj1, obj2);
}

TEST(TestWithFollyOptionals, APITests) {
  cpp2::HasOptionals obj;

  obj.int64Opt = 1;
  EXPECT_TRUE(obj.int64Opt.has_value());
  EXPECT_EQ(obj.int64Opt.value(), 1);

  folly::Optional<int64_t> f;
  apache::thrift::fromFollyOptional(obj.int64Opt, f);
  EXPECT_FALSE(obj.int64Opt.has_value());

  apache::thrift::fromFollyOptional(obj.int64Opt, f = 2);
  EXPECT_EQ(obj.int64Opt.value(), 2);

  apache::thrift::fromFollyOptional(obj.int64Opt, folly::Optional<int64_t>{3});
  EXPECT_EQ(obj.int64Opt.value(), 3);

  static_assert(std::is_same_v<
                decltype(obj.int64Opt),
                apache::thrift::DeprecatedOptionalField<int64_t>>);
  static_assert(std::is_same_v<
                decltype(copyToFollyOptional(obj.int64Opt)),
                folly::Optional<int64_t>>);
  static_assert(std::is_same_v<
                decltype(moveToFollyOptional(obj.int64Opt)),
                folly::Optional<int64_t>>);
  static_assert(std::is_same_v<
                decltype(moveToFollyOptional(std::move(obj.int64Opt))),
                folly::Optional<int64_t>>);

  static_assert(!std::is_constructible_v<
                apache::thrift::DeprecatedOptionalField<int>,
                folly::Optional<int>>);
  static_assert(!std::is_convertible_v<
                folly::Optional<int>,
                apache::thrift::DeprecatedOptionalField<int>>);
}

TEST(TestWithFollyOptionals, AddRef) {
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
  EXPECT_EQ(obj.int64Opt, 42);
  auto value = std::map<int64_t, int64_t>{{1, 2}};
  obj.mapOpt = value;
  EXPECT_EQ(*obj.mapOpt_ref(), value);
}

TEST(TestWithFollyOptionals, CopyFrom) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  obj1.int64Opt = 1;
  obj2.int64Opt.copy_from(obj1.int64Opt);
  EXPECT_EQ(obj1.int64Opt.value(), 1);
  EXPECT_EQ(obj2.int64Opt.value(), 1);

  obj1.int64Opt = 2;
  obj2.int64Opt.copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt.value(), 2);
  EXPECT_EQ(obj2.int64Opt.value(), 2);

  obj1.int64Opt = 3;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt);
  EXPECT_EQ(obj1.int64Opt.value(), 3);
  EXPECT_EQ(obj2.int64Opt.value(), 3);

  obj1.int64Opt = 4;
  obj2.int64Opt_ref().copy_from(obj1.int64Opt_ref());
  EXPECT_EQ(obj1.int64Opt.value(), 4);
  EXPECT_EQ(obj2.int64Opt.value(), 4);
}

TEST(TestWithFollyOptionals, MoveFrom) {
  cpp2::HasOptionals obj1;
  cpp2::HasOptionals obj2;

  obj1.int64Opt = 1;
  obj2.int64Opt.move_from(std::move(obj1.int64Opt));
  EXPECT_EQ(obj2.int64Opt.value(), 1);

  obj1.int64Opt = 2;
  obj2.int64Opt.move_from(std::move(obj1).int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt.value(), 2);

  obj1.int64Opt = 3;
  obj2.int64Opt_ref().move_from(std::move(obj1.int64Opt));
  EXPECT_EQ(obj2.int64Opt.value(), 3);

  obj1.int64Opt = 4;
  obj2.int64Opt_ref().move_from(std::move(obj1).int64Opt_ref());
  EXPECT_EQ(obj2.int64Opt.value(), 4);
}

TEST(TestWithFollyOptionals, equalToFollyOptional) {
  cpp2::HasOptionals obj;
  folly::Optional<int64_t> opt;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt = 1;
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt = 1;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt = 2;
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt = 2;
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  obj.int64Opt.reset();
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_FALSE(equalToFollyOptional(obj.int64Opt_ref(), opt));

  opt.reset();
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt, opt));
  EXPECT_TRUE(equalToFollyOptional(obj.int64Opt_ref(), opt));
}
