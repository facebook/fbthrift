/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/test/gen-cpp2/simple_reflection_fatal.h>
#include <thrift/test/gen-cpp2/simple_reflection_types.tcc>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_struct.h>

#include <thrift/lib/cpp2/fatal/serializer.h>
#include <thrift/lib/cpp2/fatal/pretty_print.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/JSONProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <gtest/gtest.h>
#include <folly/Memory.h>

namespace test_cpp2 { namespace simple_cpp_reflection {

// Build with, or without, the new templated serializer
// (Useful for a crude benchmark of compile time, and for verifying
// that the legacy serializer behaves the same under these unittests)
// #undef USE_NEW_SERIALIZER
#define USE_NEW_SERIALIZER

using test_cpp2::simple_cpp_reflection::struct1;
using namespace apache::thrift;

template <typename Reader, typename Writer>
struct RWPair {
  using reader = Reader;
  using writer = Writer;
};

using protocol_type_pairs = ::testing::Types<
  RWPair<SimpleJSONProtocolReader, SimpleJSONProtocolWriter>,
  RWPair<JSONProtocolReader, JSONProtocolWriter>,
  RWPair<BinaryProtocolReader, BinaryProtocolWriter>,
  RWPair<CompactProtocolReader, CompactProtocolWriter>
>;

template <typename Pair>
class TypedTestCommon : public ::testing::Test {
protected:
  typename Pair::reader reader;
  typename Pair::writer writer;
  folly::IOBufQueue buffer;
  std::unique_ptr<folly::IOBuf> underlying;

  TypedTestCommon() {
    this->writer.setOutput(&this->buffer, 1024);
  }

  void prep_read() {
    this->underlying = this->buffer.move();
    this->reader.setInput(this->underlying.get());
  }
};

template <typename Pair>
class StructTest : public TypedTestCommon<Pair> {};

TYPED_TEST_CASE(StructTest, protocol_type_pairs);

TYPED_TEST(StructTest, test_serialization) {
  // test/reflection.thrift
  struct1 a, b;
  a.field0 = 10;
  a.field1 = "this is a string";
  a.field2 = enum1::field1;
  a.field3 = {{1, 2, 3}, {4, 5, 6, 7}};
  a.field4 = {1, 1, 2, 3, 4, 10, 4, 6};
  a.field5 = {{42, "<answer>"}, {55, "schwifty five"}};
  a.field6.nfield00 = 5.678;
  a.field6.nfield01 = 0x42;
  a.field7 = 0xCAFEBABEA4DAFACE;
  a.field8 = "this field isn't set";

  a.__isset.field1 = true;
  a.__isset.field2 = true;
  a.field6.__isset.nfield00 = true;
  a.field6.__isset.nfield01 = true;
  a.__isset.field7 = true;

  EXPECT_EQ(a.field4, std::set<int32_t>({1, 2, 3, 4, 6, 10}));

  a.write(&this->writer);
  this->prep_read();

#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(b, this->reader);
#else
  b.read(&this->reader);
#endif

  EXPECT_EQ(a.field0, b.field0);
  EXPECT_EQ(a.field1, b.field1);
  EXPECT_EQ(a.field2, b.field2);
  EXPECT_EQ(a.field3, b.field3);
  EXPECT_EQ(a.field4, b.field4);
  EXPECT_EQ(a.field5, b.field5);

  EXPECT_EQ(a.field6.nfield00, b.field6.nfield00);
  EXPECT_EQ(a.field6.nfield01, b.field6.nfield01);
  EXPECT_EQ(a.field6, b.field6);
  EXPECT_EQ(a.field7, b.field7);
  EXPECT_EQ(a.field8, b.field8); // default fields are always written out

  EXPECT_EQ(true, b.__isset.field1);
  EXPECT_EQ(true, b.__isset.field2);
  EXPECT_EQ(true, b.__isset.field7);
  EXPECT_EQ(true, b.__isset.field8);
}

TYPED_TEST(StructTest, test_other_containers) {
  struct4 a, b;

  a.um_field = {{42, "answer"}, {5, "five"}};
  a.us_field = {7, 11, 13, 17, 13, 19, 11};
  a.deq_field = {10, 20, 30, 40};

  a.write(&this->writer);
  this->prep_read();

#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(b, this->reader);
#else
  b.read(&this->reader);
#endif

  EXPECT_EQ(true, b.__isset.um_field);
  EXPECT_EQ(true, b.__isset.us_field);
  EXPECT_EQ(true, b.__isset.deq_field);
  EXPECT_EQ(a.um_field, b.um_field);
  EXPECT_EQ(a.us_field, b.us_field);
  EXPECT_EQ(a.deq_field, b.deq_field);
}

namespace {
  const std::array<uint8_t, 5> test_buffer{{0xBA, 0xDB, 0xEE, 0xF0, 0x42}};
  const folly::ByteRange test_range(test_buffer.begin(), test_buffer.end());
  const folly::StringPiece test_string(test_range);

  const std::array<uint8_t, 6> test_buffer2
    {{0xFA, 0xCE, 0xB0, 0x01, 0x10, 0x0C}};
  const folly::ByteRange test_range2(test_buffer2.begin(), test_buffer2.end());
  const folly::StringPiece test_string2(test_range2);
}

TYPED_TEST(StructTest, test_binary_containers) {
  struct5 a, b;

  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range);
  a.iobufptr_field = folly::IOBuf::wrapBuffer(test_range2);

  a.write(&this->writer);
  this->prep_read();

  auto range = this->underlying->coalesce();
  DLOG(INFO) << "buffer: " <<
    std::string((const char*)range.data(), range.size());

#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(b, this->reader);
#else
  b.read(&this->reader);
#endif

  EXPECT_EQ(true, b.__isset.def_field);
  EXPECT_EQ(true, b.__isset.iobuf_field);
  EXPECT_EQ(true, b.__isset.iobufptr_field);
  EXPECT_EQ(a.def_field, b.def_field);

  EXPECT_EQ(test_range, b.iobuf_field.coalesce());
  EXPECT_EQ(test_range2, b.iobufptr_field->coalesce());
}

TYPED_TEST(StructTest, test_workaround_binary) {
  struct5_workaround a, b;
  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range2);

  a.write(&this->writer);
  this->prep_read();

#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(b, this->reader);
#else
  b.read(&this->reader);
#endif

  EXPECT_EQ(true, b.__isset.def_field);
  EXPECT_EQ(true, b.__isset.iobuf_field);
  EXPECT_EQ(test_string.str(), b.def_field);
  EXPECT_EQ(test_range2, b.iobuf_field.coalesce());
}

template <typename Pair>
class UnionTest : public TypedTestCommon<Pair> {
protected:
  union1 a, b;

  void xfer() {
    this->a.write(&this->writer);
    this->prep_read();

    auto range = this->underlying->coalesce();
    DLOG(INFO) << "buffer: " <<
      std::string((const char*)range.data(), range.size());

#ifdef USE_NEW_SERIALIZER
    apache::thrift::serializer_read(this->b, this->reader);
#else
    this->b.read(&this->reader);
#endif
    EXPECT_EQ(this->b.getType(), this->a.getType());
  }
};
TYPED_TEST_CASE(UnionTest, protocol_type_pairs);

TYPED_TEST(UnionTest, can_read_union_i64s) {
  this->a.set_field_i64(0xFACEB00CFACEDEAD);
  this->xfer();
  EXPECT_EQ(this->b.get_field_i64(), this->a.get_field_i64());
}
TYPED_TEST(UnionTest, can_read_strings) {
  this->a.set_field_string("test string? oh my!");
  this->xfer();
  EXPECT_EQ(this->b.get_field_string(), this->a.get_field_string());
}
TYPED_TEST(UnionTest, can_read_refstrings) {
  this->a.set_field_string_ref("also reference strings!");
  this->xfer();
  EXPECT_EQ(
    *(this->b.get_field_string_ref().get()),
    *(this->a.get_field_string_ref().get()));
}
TYPED_TEST(UnionTest, can_read_iobufs) {
  this->a.set_field_binary(test_string.str());
  this->xfer();
  EXPECT_EQ(test_string.str(), this->b.get_field_binary());
}

template <typename Pair>
class BinaryInContainersTest : public TypedTestCommon<Pair> {
protected:
  struct5_listworkaround a, b;

  void xfer() {
    this->a.write(&this->writer);
    this->prep_read();
#ifdef USE_NEW_SERIALIZER
    apache::thrift::serializer_read(this->b, this->reader);
#else
    this->b.read(&this->reader);
#endif
  }
};
TYPED_TEST_CASE(BinaryInContainersTest, protocol_type_pairs);

TYPED_TEST(BinaryInContainersTest, lists_of_binary_fields_work) {
  this->a.binary_list_field = {test_string.str()};
  this->a.binary_map_field1 = {
    {5,     test_string.str()},
    {-9999, test_string2.str()}};

  this->xfer();

  EXPECT_EQ(
    std::vector<std::string>({test_string.str()}),
    this->b.binary_list_field);
}

struct SimpleJsonTest : public ::testing::Test {
  SimpleJSONProtocolReader reader;

  void set_input(std::string&& str) {
    auto buffer = folly::IOBuf::copyBuffer(str);
    reader.setInput(&*buffer);
  }
};

TEST_F(SimpleJsonTest, throws_on_unset_required_value) {
  set_input("{}");
  try {
    struct2 a;
#ifdef USE_NEW_SERIALIZER
    apache::thrift::serializer_read(a, reader);
#else
    a.read(&reader);
#endif
    EXPECT_TRUE(false) << "serializer_read didn't throw";
  }
  catch(TProtocolException& e) {
    EXPECT_EQ(TProtocolException::MISSING_REQUIRED_FIELD, e.getType());
  }
}

#define KV(key, value) "\"" key "\":\"" value "\""
TEST_F(SimpleJsonTest, handles_unset_default_member) {
  set_input("{" KV("req_string", "required") "}");
  struct2 a;
#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(a, reader);
#else
  a.read(&reader);
#endif
  EXPECT_TRUE(false == a.__isset.opt_string); // gcc bug?
  EXPECT_TRUE(false == a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_opt_members) {
  set_input("{" KV("req_string","required")"," KV("opt_string","optional") "}");
  struct2 a;
#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(a, reader);
#else
  a.read(&reader);
#endif
  EXPECT_TRUE(true == a.__isset.opt_string); // gcc bug?
  EXPECT_TRUE(false == a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("optional", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_def_members) {
  set_input("{" KV("req_string","required")"," KV("def_string", "default") "}");
  struct2 a;
#ifdef USE_NEW_SERIALIZER
  apache::thrift::serializer_read(a, reader);
#else
  a.read(&reader);
#endif
  EXPECT_TRUE(false == a.__isset.opt_string);
  EXPECT_EQ(true,  a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("default", a.def_string);
}
#undef KV
} } /* namespace cpp_reflection::test_cpp2 */
