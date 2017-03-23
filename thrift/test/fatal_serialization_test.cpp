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

#include <thrift/test/gen-cpp2/simple_reflection_fatal_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_types_custom_protocol.h>

#include <thrift/test/gen-cpp2/service_reflection_fatal_types.h>
#include <thrift/test/gen-cpp2/service_reflection_types_custom_protocol.h>

#include <thrift/test/gen-cpp2/EnumTest_fatal_types.h>
#include <thrift/test/gen-cpp2/EnumTest_types_custom_protocol.h>

#include <thrift/lib/cpp2/fatal/serializer.h>
#include <thrift/lib/cpp2/fatal/pretty_print.h>
#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>

#include <folly/Memory.h>

#include <thrift/test/fatal_serialization_common.h>

#include <iomanip>


TYPED_TEST_CASE(MultiProtocolTest, protocol_type_pairs);
TYPED_TEST_CASE(CompareProtocolTest, protocol_type_pairs);

// using apache::thrift::serializer_read;
// using apache::thrift::serializer_write;
// using apache::thrift::serializer_serialized_size;
// using apache::thrift::serializer_serialized_size_zc;

template <typename Type, typename Protocol>
void expect_same_serialized_size(Type& type, Protocol& protocol) {
  EXPECT_EQ(
    Cpp2Ops<Type>::serializedSize(&protocol, &type),
    serializer_serialized_size(type, protocol)
  );
  EXPECT_EQ(
    Cpp2Ops<Type>::serializedSizeZC(&protocol, &type),
    serializer_serialized_size_zc(type, protocol)
  );
}

namespace cpp2 {
TYPED_TEST(MultiProtocolTest, test_compat) {
  // from EnumTest.thrift (compiled with fatal,compatibility)
  MyStruct a;
  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();

  MyStruct b;
  serializer_read(a, this->reader);
  EXPECT_EQ(a, b);
  expect_same_serialized_size(a, this->writer);
}
} // namespace cpp2

// simply tests if we can compile the structs related to services
namespace service_reflection { namespace cpp2 {
TYPED_TEST(MultiProtocolTest, service_reflection_test) {
  struct1 a;
  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();

  struct1 b;
  serializer_read(b, this->reader);
  EXPECT_EQ(a, b);
  expect_same_serialized_size(a, this->writer);
}
}} // namespcae service_reflection::cpp2

namespace test_cpp2 { namespace simple_cpp_reflection {

void init_struct_1(struct1& a) {
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

  a.field10 = {true, false, true, false, false, true, true};

  a.__isset.field1 = true;
  a.__isset.field2 = true;
  a.field6.__isset.nfield00 = true;
  a.field6.__isset.nfield01 = true;
  a.__isset.field7 = true;
}

TYPED_TEST(MultiProtocolTest, test_serialization) {
  // test/reflection.thrift
  struct1 a, b;
  init_struct_1(a);

  EXPECT_EQ(a.field4, std::set<int32_t>({1, 2, 3, 4, 6, 10}));

  serializer_write(a, this->writer);
  this->prep_read();

  serializer_read(b, this->reader);

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
  EXPECT_EQ(a.field10, b.field10);

  EXPECT_TRUE(b.__isset.field1);
  EXPECT_TRUE(b.__isset.field2);
  EXPECT_TRUE(b.__isset.field7);
  EXPECT_TRUE(b.__isset.field8);
}

TYPED_TEST(MultiProtocolTest, test_legacy_serialization) {
  // test/reflection.thrift
  struct1 a;
  init_struct_1(a);

  serializer_write(a, this->writer);
  this->prep_read();

  struct1 b;
  b.read(&this->reader);

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

  EXPECT_TRUE(b.__isset.field1);
  EXPECT_TRUE(b.__isset.field2);
  EXPECT_TRUE(b.__isset.field7);
  EXPECT_TRUE(b.__isset.field8);
}

TYPED_TEST(MultiProtocolTest, test_other_containers) {
  struct4 a, b;

  a.um_field = {{42, "answer"}, {5, "five"}};
  a.us_field = {7, 11, 13, 17, 13, 19, 11};
  a.deq_field = {10, 20, 30, 40};

  serializer_write(a, this->writer);
  this->prep_read();
  serializer_read(b, this->reader);

  EXPECT_TRUE(b.__isset.um_field);
  EXPECT_TRUE(b.__isset.us_field);
  EXPECT_TRUE(b.__isset.deq_field);
  EXPECT_EQ(a.um_field, b.um_field);
  EXPECT_EQ(a.us_field, b.us_field);
  EXPECT_EQ(a.deq_field, b.deq_field);
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, test_blank_default_ref_field) {
  struct3 a, b;
  a.opt_nested = std::make_unique<smallstruct>();
  a.req_nested = std::make_unique<smallstruct>();

  a.opt_nested->f1 = 5;
  a.req_nested->f1 = 10;

  // ref fields, interesting enough, do not have an __isset,
  // but are xfered based on the pointer value (nullptr or not)

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(smallstruct(),   *(b.def_nested));
  EXPECT_EQ(*(a.opt_nested), *(b.opt_nested));
  EXPECT_EQ(*(a.req_nested), *(b.req_nested));
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, test_blank_optional_ref_field) {
  struct3 a, b;
  a.def_nested = std::make_unique<smallstruct>();
  a.req_nested = std::make_unique<smallstruct>();

  a.def_nested->f1 = 5;
  a.req_nested->f1 = 10;

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  // null optional fields are deserialized to nullptr
  EXPECT_EQ(*(a.def_nested), *(b.def_nested));
  EXPECT_EQ(nullptr,           b.opt_nested.get());
  EXPECT_EQ(*(a.req_nested), *(b.req_nested));
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, test_blank_required_ref_field) {
  struct3 a, b;
  a.def_nested = std::make_unique<smallstruct>();
  a.opt_nested = std::make_unique<smallstruct>();

  a.def_nested->f1 = 5;
  a.opt_nested->f1 = 10;

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(*(a.def_nested), *(b.def_nested));
  EXPECT_EQ(*(a.opt_nested), *(b.opt_nested));
  EXPECT_EQ(smallstruct(),   *(b.req_nested));
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, test_empty_containers) {
  struct1 a, b;
  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(a, b);
}

TYPED_TEST(CompareProtocolTest, test_struct_xfer) {
  struct1 a1, a2, b1, b2;
  init_struct_1(a1);
  init_struct_1(a2);
  const std::size_t legacy_write_xfer = a1.write(&this->st1.writer);
  const std::size_t new_write_xfer = serializer_write(a2, this->st2.writer);
  EXPECT_EQ(legacy_write_xfer, new_write_xfer);

  this->prep_read();
  this->debug_buffer();

  const std::size_t legacy_read_xfer = b1.read(&this->st1.reader);
  const std::size_t new_read_xfer    = serializer_read(b2, this->st2.reader);

  EXPECT_EQ(legacy_read_xfer, new_read_xfer);
  EXPECT_EQ(b1, b2);

  expect_same_serialized_size(a1, this->st1.writer);
}

TYPED_TEST(CompareProtocolTest, test_larger_containers) {
  struct1 a1;
  struct1 a2;
  init_struct_1(a1);
  init_struct_1(a2);

  std::map<int32_t, std::string> large_map;
  for(int32_t i = 0; i < 1000; i++) {
    large_map.emplace(i, std::string("string"));
  }

  a1.field5 = large_map;
  a2.field5 = large_map;

  EXPECT_EQ(a1, a2);
  EXPECT_EQ(
    a1.write(&this->st1.writer),
    serializer_write(a2, this->st2.writer)
  );

  this->prep_read();
  this->debug_buffer();

  struct1 b1;
  struct1 b2;

  EXPECT_EQ(
    b1.read(&this->st1.reader),
    serializer_read(b2, this->st2.reader)
  );
  EXPECT_EQ(b1, b2);
  expect_same_serialized_size(a1, this->st1.writer);
}

TYPED_TEST(CompareProtocolTest, test_union_xfer) {
  union1 a1, a2, b1, b2;
  a1.set_field_i64(0x1ABBADABAD00);
  a2.set_field_i64(0x1ABBADABAD00);
  const std::size_t lwx = a1.write(&this->st1.writer);
  const std::size_t nwx = serializer_write(a2, this->st2.writer);
  EXPECT_EQ(lwx, nwx);

  this->prep_read();
  this->debug_buffer();

  const std::size_t lrx = b1.read(&this->st1.reader);
  const std::size_t nrx = serializer_read(b2, this->st2.reader);
  EXPECT_EQ(lrx, nrx);
  EXPECT_EQ(b1, b2);

  expect_same_serialized_size(a1, this->st1.writer);
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

TYPED_TEST(MultiProtocolTest, test_binary_containers) {
  struct5 a, b;

  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range);
  a.iobufptr_field = folly::IOBuf::wrapBuffer(test_range2);

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();

  serializer_read(b, this->reader);

  EXPECT_TRUE(b.__isset.def_field);
  EXPECT_TRUE(b.__isset.iobuf_field);
  EXPECT_TRUE(b.__isset.iobufptr_field);
  EXPECT_EQ(a.def_field, b.def_field);

  EXPECT_EQ(test_range, b.iobuf_field.coalesce());
  EXPECT_EQ(test_range2, b.iobufptr_field->coalesce());
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, test_workaround_binary) {
  struct5_workaround a, b;
  a.def_field = test_string.str();
  a.iobuf_field = folly::IOBuf::wrapBufferAsValue(test_range2);

  serializer_write(a, this->writer);
  this->prep_read();
  serializer_read(b, this->reader);

  EXPECT_TRUE(b.__isset.def_field);
  EXPECT_TRUE(b.__isset.iobuf_field);
  EXPECT_EQ(test_string.str(), b.def_field);
  EXPECT_EQ(test_range2, b.iobuf_field.coalesce());
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, shared_ptr_test) {
  struct6 a, b;
  a.def_field = std::make_shared<smallstruct>();
  a.req_field = std::make_shared<smallstruct>();
  a.opt_field = a.req_field;

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(a, b);
  expect_same_serialized_size(a, this->writer);
}

TYPED_TEST(MultiProtocolTest, shared_const_ptr_test) {
  struct8 a, b;

  auto def_field = std::make_unique<smallstruct>();
  def_field->f1 = 10;
  a.def_field = std::move(def_field);

  auto opt_field = std::make_unique<smallstruct>();
  opt_field->f1 = 20;
  a.opt_field = std::move(opt_field);

  auto req_field = std::make_unique<smallstruct>();
  req_field->f1 = 30;
  a.req_field = std::move(req_field);

  serializer_write(a, this->writer);
  this->prep_read();
  this->debug_buffer();
  serializer_read(b, this->reader);

  EXPECT_EQ(a, b);
  expect_same_serialized_size(a, this->writer);
}

template <typename Pair>
class UnionTest : public TypedTestCommon<Pair> {
protected:
  union1 a, b;

  void xfer() {
    serializer_write(this->a, this->writer);
    this->prep_read();

    print_underlying<Pair::printable::value>(*this->underlying);

    serializer_read(b, this->reader);
    EXPECT_EQ(this->b.getType(), this->a.getType());
  }
};

TYPED_TEST_CASE(UnionTest, protocol_type_pairs);

TYPED_TEST(UnionTest, can_read_union_i64s) {
  this->a.set_field_i64(0xFACEB00CFACEDEAD);
  this->xfer();
  EXPECT_EQ(this->b.get_field_i64(), this->a.get_field_i64());
  expect_same_serialized_size(this->a, this->writer);
}
TYPED_TEST(UnionTest, can_read_strings) {
  this->a.set_field_string("test string? oh my!");
  this->xfer();
  EXPECT_EQ(this->b.get_field_string(), this->a.get_field_string());
  expect_same_serialized_size(this->a, this->writer);
}
TYPED_TEST(UnionTest, can_read_refstrings) {
  this->a.set_field_string_ref("also reference strings!");
  this->xfer();
  EXPECT_EQ(
    *(this->b.get_field_string_ref().get()),
    *(this->a.get_field_string_ref().get()));
  expect_same_serialized_size(this->a, this->writer);
}
TYPED_TEST(UnionTest, can_read_iobufs) {
  this->a.set_field_binary(test_string.str());
  this->xfer();
  EXPECT_EQ(test_string.str(), this->b.get_field_binary());
  expect_same_serialized_size(this->a, this->writer);
}
TYPED_TEST(UnionTest, can_read_nestedstructs) {
  smallstruct nested;
  nested.f1 = 6;
  this->a.set_field_smallstruct(nested);
  this->xfer();
  EXPECT_EQ(6, this->b.get_field_smallstruct()->f1);
  expect_same_serialized_size(this->a, this->writer);
}

template <typename Pair>
class BinaryInContainersTest : public TypedTestCommon<Pair> {
protected:
  struct5_listworkaround a, b;

  void xfer() {
    serializer_write(this->a, this->writer);
    this->prep_read();
    serializer_read(this->b, this->reader);
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
    this->b.binary_list_field
  );
  expect_same_serialized_size(this->a, this->writer);
}

struct SimpleJsonTest : public ::testing::Test {
  SimpleJSONProtocolReader reader;
  std::unique_ptr<folly::IOBuf> underlying;

  void set_input(std::string&& str) {
    underlying = folly::IOBuf::copyBuffer(str);
    reader.setInput(underlying.get());

    if(VLOG_IS_ON(5)) {
      auto range = underlying->coalesce();
      VLOG(5) << "buffer: "
        << std::string((const char*)range.data(), range.size());
    }
  }
};

TEST_F(SimpleJsonTest, throws_on_unset_required_value) {
  set_input("{}");
  try {
    struct2 a;
    serializer_read(a, reader);
    EXPECT_TRUE(false) << "didn't throw!";
  }
  catch(TProtocolException& e) {
    EXPECT_EQ(TProtocolException::MISSING_REQUIRED_FIELD, e.getType());
  }
}

// wrap in quotes
#define Q(val) "\"" val "\""
// emit key/value json pair
#define KV(key, value) "\"" key "\":" value
// emit key/value json pair, where value is a string
#define KVS(key, value) KV(key, Q(value))

TEST_F(SimpleJsonTest, handles_unset_default_member) {
  set_input("{" KVS("req_string", "required") "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_FALSE(a.__isset.opt_string); // gcc bug?
  EXPECT_FALSE(a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_opt_members) {
  set_input("{"
    KVS("req_string","required")","
    KVS("opt_string","optional")
  "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_TRUE(a.__isset.opt_string); // gcc bug?
  EXPECT_FALSE(a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("optional", a.opt_string);
  EXPECT_EQ("", a.def_string);
}
TEST_F(SimpleJsonTest, sets_def_members) {
  set_input("{"
    KVS("req_string","required")","
    KVS("def_string", "default")
  "}");
  struct2 a;
  serializer_read(a, reader);
  EXPECT_FALSE(a.__isset.opt_string);
  EXPECT_TRUE( a.__isset.def_string);
  EXPECT_EQ("required", a.req_string);
  EXPECT_EQ("", a.opt_string);
  EXPECT_EQ("default", a.def_string);
}
TEST_F(SimpleJsonTest, throws_on_missing_required_ref) {
  set_input("{"
    KV("opt_nested", "{"
      KV("f1", "10")
    "}")","
    KV("def_nested", "{"
      KV("f1", "5")
    "}")
  "}");

  struct3 a;

  try {
    serializer_read(a, reader);
    EXPECT_TRUE(false) << "didn't throw!";
  }
  catch(TProtocolException& e) {
    EXPECT_EQ(TProtocolException::MISSING_REQUIRED_FIELD, e.getType());
  }
}
TEST_F(SimpleJsonTest, doesnt_throw_when_req_field_present) {
  set_input("{"
    KV("opt_nested", "{"
      KV("f1", "10")
    "}")","
    KV("def_nested", "{"
      KV("f1", "5")
    "}")","
    KV("req_nested", "{"
      KV("f1", "15")
    "}")
  "}");

  struct3 a;
  serializer_read(a, reader);
  EXPECT_EQ(10, a.opt_nested->f1);
  EXPECT_EQ(5, a.def_nested->f1);
  EXPECT_EQ(15, a.req_nested->f1);
}
#undef KV
} } /* namespace cpp_reflection::test_cpp2 */
