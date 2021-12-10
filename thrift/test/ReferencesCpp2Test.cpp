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

#include <thrift/lib/cpp2/BadFieldAccess.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/test/gen-cpp2/References_types.h>
#include <thrift/test/gen-cpp2/References_types.tcc>

using namespace apache::thrift;

namespace cpp2 {

TEST(References, recursive_ref_fields) {
  SimpleJSONProtocolWriter writer;
  folly::IOBufQueue buff;
  writer.setOutput(&buff, 1024);

  EXPECT_EQ(nullptr, buff.front());

  cpp2::RecursiveStruct a;
  // Normally non-optional fields are present in a default-constructed object,
  // here we check the special-case of a recursive data type with a non-optional
  // or even required reference to its own type: obviously this doesn't make a
  // lot of sense since any chain of such structure must either contain a cycle
  // (meaning we can't possibly serialize it) or a nullptr (meaning it's in fact
  // optional), but for historical reasons we allow this and default the
  // value to `nullptr`.
  EXPECT_EQ(nullptr, a.def_field_ref().get());
  EXPECT_EQ(nullptr, a.req_field_ref().get());
  // Check that optional fields are absent from a default-constructed object
  EXPECT_EQ(nullptr, a.opt_field_ref().get());

  EXPECT_EQ(nullptr, a.def_field_ref().get());
  EXPECT_EQ(nullptr, a.req_field_ref().get());
  EXPECT_EQ(nullptr, a.opt_field_ref().get());

  // this isn't the correct serialized size, but it's what simple json returns.
  // it is the correct length for a manually inspected, correct serializedSize
  EXPECT_EQ(120, a.serializedSize(&writer));
  EXPECT_EQ(120, a.serializedSizeZC(&writer));

  if (buff.front()) {
    EXPECT_EQ(0, buff.front()->length());
  }

  a.def_field_ref() = std::make_unique<cpp2::RecursiveStruct>();
  a.opt_field_ref() = std::make_unique<cpp2::RecursiveStruct>();
  EXPECT_EQ(415, a.serializedSize(&writer));
  EXPECT_EQ(415, a.serializedSizeZC(&writer));

  cpp2::RecursiveStruct b;
  b.def_field_ref() = std::make_unique<cpp2::RecursiveStruct>();
  b.opt_field_ref() = std::make_unique<cpp2::RecursiveStruct>();
  EXPECT_EQ(415, b.serializedSize(&writer));
  EXPECT_EQ(415, b.serializedSizeZC(&writer));
}

TEST(References, ref_struct_fields) {
  ReferringStruct a;

  // Test that non-optional ref struct fields are initialized.
  EXPECT_NE(nullptr, a.def_field_ref());
  EXPECT_NE(nullptr, a.req_field_ref());
  EXPECT_NE(nullptr, a.def_unique_field_ref());
  EXPECT_NE(nullptr, a.req_unique_field_ref());
  EXPECT_NE(nullptr, a.def_shared_field_ref());
  EXPECT_NE(nullptr, a.req_shared_field_ref());
  EXPECT_NE(nullptr, a.def_shared_const_field_ref());
  EXPECT_NE(nullptr, a.req_shared_const_field_ref());

  // Check that optional fields are absent from a default-constructed object.
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_struct_fields_clear) {
  ReferringStruct a;

  auto s1 = std::make_shared<cpp2::PlainStruct>();
  auto s2 = std::make_shared<cpp2::PlainStruct>();
  auto s3 = std::make_shared<cpp2::PlainStruct>();
  s1->field() = 10;
  s2->field() = 11;
  s3->field() = 12;

  a.def_field_ref()->field_ref() = 1;
  a.req_field_ref()->field_ref() = 2;
  a.opt_field_ref() = std::make_unique<cpp2::PlainStruct>();
  a.opt_field_ref()->field_ref() = 3;
  a.def_unique_field_ref()->field_ref() = 4;
  a.req_unique_field_ref()->field_ref() = 5;
  a.opt_unique_field_ref() = std::make_unique<cpp2::PlainStruct>();
  a.opt_unique_field_ref()->field_ref() = 6;
  a.def_shared_field_ref()->field_ref() = 7;
  a.req_shared_field_ref()->field_ref() = 8;
  a.opt_shared_field_ref() = std::make_shared<cpp2::PlainStruct>();
  a.opt_shared_field_ref()->field_ref() = 9;
  a.def_shared_const_field_ref() = std::move(s1);
  a.req_shared_const_field_ref() = std::move(s2);
  a.opt_shared_const_field_ref() = std::move(s3);
  a.opt_box_field_ref() = cpp2::PlainStruct();
  a.opt_box_field_ref()->field_ref() = 13;

  EXPECT_EQ(a.def_field_ref()->field_ref(), 1);
  EXPECT_EQ(a.req_field_ref()->field_ref(), 2);
  EXPECT_EQ(a.opt_field_ref()->field_ref(), 3);
  EXPECT_EQ(a.def_unique_field_ref()->field_ref(), 4);
  EXPECT_EQ(a.req_unique_field_ref()->field_ref(), 5);
  EXPECT_EQ(a.opt_unique_field_ref()->field_ref(), 6);
  EXPECT_EQ(a.def_shared_field_ref()->field_ref(), 7);
  EXPECT_EQ(a.req_shared_field_ref()->field_ref(), 8);
  EXPECT_EQ(a.opt_shared_field_ref()->field_ref(), 9);
  EXPECT_EQ(a.def_shared_const_field_ref()->field_ref(), 10);
  EXPECT_EQ(a.req_shared_const_field_ref()->field_ref(), 11);
  EXPECT_EQ(a.opt_shared_const_field_ref()->field_ref(), 12);
  EXPECT_EQ(a.opt_box_field_ref()->field_ref(), 13);

  a = {};

  EXPECT_EQ(a.def_field_ref()->field_ref(), 0);
  EXPECT_EQ(a.req_field_ref()->field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(a.def_unique_field_ref()->field_ref(), 0);
  EXPECT_EQ(a.req_unique_field_ref()->field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(a.def_shared_field_ref()->field_ref(), 0);
  EXPECT_EQ(a.req_shared_field_ref()->field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(a.def_shared_const_field_ref()->field_ref(), 0);
  EXPECT_EQ(a.req_shared_const_field_ref()->field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_struct_base_fields) {
  ReferringStructWithBaseTypeFields a;

  // Test that non-optional ref basetype fields are initialized.
  EXPECT_NE(nullptr, a.def_field_ref());
  EXPECT_NE(nullptr, a.req_field_ref());
  EXPECT_NE(nullptr, a.def_unique_field_ref());
  EXPECT_NE(nullptr, a.req_unique_field_ref());
  EXPECT_NE(nullptr, a.def_shared_field_ref());
  EXPECT_NE(nullptr, a.req_shared_field_ref());
  EXPECT_NE(nullptr, a.def_shared_const_field_ref());
  EXPECT_NE(nullptr, a.req_shared_const_field_ref());

  // Check that optional fields are absent from a default-constructed object.
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_struct_base_fields_clear) {
  ReferringStructWithBaseTypeFields a;

  a.def_field_ref() = std::make_unique<int64_t>(1);
  a.req_field_ref() = std::make_unique<int64_t>(2);
  a.opt_field_ref() = std::make_unique<int64_t>(3);
  a.def_unique_field_ref() = std::make_unique<int64_t>(4);
  a.req_unique_field_ref() = std::make_unique<int64_t>(5);
  a.opt_unique_field_ref() = std::make_unique<int64_t>(6);
  a.def_shared_field_ref() = std::make_shared<int64_t>(7);
  a.req_shared_field_ref() = std::make_shared<int64_t>(8);
  a.opt_shared_field_ref() = std::make_shared<int64_t>(9);
  a.def_shared_const_field_ref() = std::make_shared<int64_t>(10);
  a.req_shared_const_field_ref() = std::make_shared<int64_t>(11);
  a.opt_shared_const_field_ref() = std::make_shared<int64_t>(12);
  a.opt_box_field_ref() = "13";

  EXPECT_EQ(*a.def_field_ref(), 1);
  EXPECT_EQ(*a.req_field_ref(), 2);
  EXPECT_EQ(*a.opt_field_ref(), 3);
  EXPECT_EQ(*a.def_unique_field_ref(), 4);
  EXPECT_EQ(*a.req_unique_field_ref(), 5);
  EXPECT_EQ(*a.opt_unique_field_ref(), 6);
  EXPECT_EQ(*a.def_shared_field_ref(), 7);
  EXPECT_EQ(*a.req_shared_field_ref(), 8);
  EXPECT_EQ(*a.opt_shared_field_ref(), 9);
  EXPECT_EQ(*a.def_shared_const_field_ref(), 10);
  EXPECT_EQ(*a.req_shared_const_field_ref(), 11);
  EXPECT_EQ(*a.opt_shared_const_field_ref(), 12);
  EXPECT_EQ(*a.opt_box_field_ref(), "13");

  a = {};

  EXPECT_EQ(*a.def_field_ref(), 0);
  EXPECT_EQ(*a.req_field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(*a.def_unique_field_ref(), 0);
  EXPECT_EQ(*a.req_unique_field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(*a.def_shared_field_ref(), 0);
  EXPECT_EQ(*a.req_shared_field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(*a.def_shared_const_field_ref(), 0);
  EXPECT_EQ(*a.req_shared_const_field_ref(), 0);
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_struct_string_fields) {
  ReferringStructWithStringFields a;

  // Test that non-optional ref string fields are initialized.
  EXPECT_NE(nullptr, a.def_field_ref());
  EXPECT_NE(nullptr, a.req_field_ref());
  EXPECT_NE(nullptr, a.def_unique_field_ref());
  EXPECT_NE(nullptr, a.req_unique_field_ref());
  EXPECT_NE(nullptr, a.def_shared_field_ref());
  EXPECT_NE(nullptr, a.req_shared_field_ref());
  EXPECT_NE(nullptr, a.def_shared_const_field_ref());
  EXPECT_NE(nullptr, a.req_shared_const_field_ref());

  // Check that optional fields are absent from a default-constructed object.
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_struct_string_fields_clear) {
  ReferringStructWithStringFields a;

  a.def_field_ref() = std::make_unique<std::string>("1");
  a.req_field_ref() = std::make_unique<std::string>("2");
  a.opt_field_ref() = std::make_unique<std::string>("3");
  a.def_unique_field_ref() = std::make_unique<std::string>("4");
  a.req_unique_field_ref() = std::make_unique<std::string>("5");
  a.opt_unique_field_ref() = std::make_unique<std::string>("6");
  a.def_shared_field_ref() = std::make_shared<std::string>("7");
  a.req_shared_field_ref() = std::make_shared<std::string>("8");
  a.opt_shared_field_ref() = std::make_shared<std::string>("9");
  a.def_shared_const_field_ref() = std::make_shared<std::string>("10");
  a.req_shared_const_field_ref() = std::make_shared<std::string>("11");
  a.opt_shared_const_field_ref() = std::make_shared<std::string>("12");
  a.opt_box_field_ref() = "13";

  EXPECT_EQ(*a.def_field_ref(), "1");
  EXPECT_EQ(*a.req_field_ref(), "2");
  EXPECT_EQ(*a.opt_field_ref(), "3");
  EXPECT_EQ(*a.def_unique_field_ref(), "4");
  EXPECT_EQ(*a.req_unique_field_ref(), "5");
  EXPECT_EQ(*a.opt_unique_field_ref(), "6");
  EXPECT_EQ(*a.def_shared_field_ref(), "7");
  EXPECT_EQ(*a.req_shared_field_ref(), "8");
  EXPECT_EQ(*a.opt_shared_field_ref(), "9");
  EXPECT_EQ(*a.def_shared_const_field_ref(), "10");
  EXPECT_EQ(*a.req_shared_const_field_ref(), "11");
  EXPECT_EQ(*a.opt_shared_const_field_ref(), "12");
  EXPECT_EQ(*a.opt_box_field_ref(), "13");

  a = {};

  EXPECT_EQ(*a.def_field_ref(), "");
  EXPECT_EQ(*a.req_field_ref(), "");
  EXPECT_EQ(nullptr, a.opt_field_ref());
  EXPECT_EQ(*a.def_unique_field_ref(), "");
  EXPECT_EQ(*a.req_unique_field_ref(), "");
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(*a.def_shared_field_ref(), "");
  EXPECT_EQ(*a.req_shared_field_ref(), "");
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(*a.def_shared_const_field_ref(), "");
  EXPECT_EQ(*a.req_shared_const_field_ref(), "");
  EXPECT_EQ(nullptr, a.opt_shared_const_field_ref());
  EXPECT_FALSE(a.opt_box_field_ref().has_value());
}

TEST(References, ref_container_fields) {
  StructWithContainers a;

  // Test that non-optional ref container fields are initialized.
  EXPECT_NE(nullptr, a.def_list_ref_ref());
  EXPECT_NE(nullptr, a.def_set_ref_ref());
  EXPECT_NE(nullptr, a.def_map_ref_ref());
  EXPECT_NE(nullptr, a.def_list_ref_unique_ref());
  EXPECT_NE(nullptr, a.def_set_ref_shared_ref());
  EXPECT_NE(nullptr, a.def_list_ref_shared_const_ref());
  EXPECT_NE(nullptr, a.req_list_ref_ref());
  EXPECT_NE(nullptr, a.req_set_ref_ref());
  EXPECT_NE(nullptr, a.req_map_ref_ref());
  EXPECT_NE(nullptr, a.req_list_ref_unique_ref());
  EXPECT_NE(nullptr, a.req_set_ref_shared_ref());
  EXPECT_NE(nullptr, a.req_list_ref_shared_const_ref());

  // Check that optional fields are absent from a default-constructed object.
  EXPECT_EQ(nullptr, a.opt_list_ref_ref());
  EXPECT_EQ(nullptr, a.opt_set_ref_ref());
  EXPECT_EQ(nullptr, a.opt_map_ref_ref());
  EXPECT_EQ(nullptr, a.opt_list_ref_unique_ref());
  EXPECT_EQ(nullptr, a.opt_set_ref_shared_ref());
  EXPECT_EQ(nullptr, a.opt_list_ref_shared_const_ref());
  EXPECT_FALSE(a.opt_box_list_ref_ref().has_value());
  EXPECT_FALSE(a.opt_box_set_ref_ref().has_value());
  EXPECT_FALSE(a.opt_box_map_ref_ref().has_value());
}

TEST(References, ref_container_fields_clear) {
  StructWithContainers a;

  a.def_list_ref_ref()->push_back(1);
  a.def_set_ref_ref()->insert(2);
  a.def_map_ref_ref()->insert({3, 3});
  a.def_list_ref_unique_ref()->push_back(4);
  a.def_set_ref_shared_ref()->insert(5);
  a.def_list_ref_shared_const_ref() =
      std::make_shared<std::vector<int32_t>>(1, 6);
  a.req_list_ref_ref()->push_back(7);
  a.req_set_ref_ref()->insert(8);
  a.req_map_ref_ref()->insert({9, 9});
  a.req_list_ref_unique_ref()->push_back(10);
  a.req_set_ref_shared_ref()->insert(11);
  a.req_list_ref_shared_const_ref() =
      std::make_shared<std::vector<int32_t>>(1, 12);
  a.opt_list_ref_ref() = std::make_unique<std::vector<int32_t>>(1, 13);
  auto s1 = std::set<int32_t>();
  s1.insert(14);
  a.opt_set_ref_ref() = std::make_unique<std::set<int32_t>>(s1);
  auto m1 = std::map<int32_t, int32_t>();
  m1.insert({15, 15});
  a.opt_map_ref_ref() = std::make_unique<std::map<int32_t, int32_t>>(m1);
  a.opt_list_ref_unique_ref() = std::make_unique<std::vector<int32_t>>(1, 16);
  auto s2 = std::set<int32_t>();
  s2.insert(17);
  a.opt_set_ref_shared_ref() = std::make_shared<std::set<int32_t>>(s2);
  a.opt_list_ref_shared_const_ref() =
      std::make_unique<std::vector<int32_t>>(1, 18);
  a.opt_box_list_ref_ref() = std::vector<int32_t>(1, 19);
  auto s3 = std::set<int32_t>();
  s3.insert(20);
  a.opt_box_set_ref_ref() = std::move(s3);
  auto m2 = std::map<int32_t, int32_t>();
  m2.insert({21, 21});
  a.opt_box_map_ref_ref() = std::move(m2);

  EXPECT_EQ(a.def_list_ref_ref()->at(0), 1);
  EXPECT_EQ(a.def_set_ref_ref()->count(2), 1);
  EXPECT_EQ(a.def_map_ref_ref()->at(3), 3);
  EXPECT_EQ(a.def_list_ref_unique_ref()->at(0), 4);
  EXPECT_EQ(a.def_set_ref_shared_ref()->count(5), 1);
  EXPECT_EQ(a.def_list_ref_shared_const_ref()->at(0), 6);
  EXPECT_EQ(a.req_list_ref_ref()->at(0), 7);
  EXPECT_EQ(a.req_set_ref_ref()->count(8), 1);
  EXPECT_EQ(a.req_map_ref_ref()->at(9), 9);
  EXPECT_EQ(a.req_list_ref_unique_ref()->at(0), 10);
  EXPECT_EQ(a.req_set_ref_shared_ref()->count(11), 1);
  EXPECT_EQ(a.req_list_ref_shared_const_ref()->at(0), 12);
  EXPECT_EQ(a.opt_list_ref_ref()->at(0), 13);
  EXPECT_EQ(a.opt_set_ref_ref()->count(14), 1);
  EXPECT_EQ(a.opt_map_ref_ref()->at(15), 15);
  EXPECT_EQ(a.opt_list_ref_unique_ref()->at(0), 16);
  EXPECT_EQ(a.opt_set_ref_shared_ref()->count(17), 1);
  EXPECT_EQ(a.opt_list_ref_shared_const_ref()->at(0), 18);
  EXPECT_EQ(a.opt_box_list_ref_ref()->at(0), 19);
  EXPECT_EQ(a.opt_box_set_ref_ref()->count(20), 1);
  EXPECT_EQ(a.opt_box_map_ref_ref()->at(21), 21);

  a = {};

  EXPECT_EQ(a.def_list_ref_ref()->size(), 0);
  EXPECT_EQ(a.def_set_ref_ref()->size(), 0);
  EXPECT_EQ(a.def_map_ref_ref()->size(), 0);
  EXPECT_EQ(a.def_list_ref_unique_ref()->size(), 0);
  EXPECT_EQ(a.def_set_ref_shared_ref()->size(), 0);
  EXPECT_EQ(a.def_list_ref_shared_const_ref()->size(), 0);
  EXPECT_EQ(a.req_list_ref_ref()->size(), 0);
  EXPECT_EQ(a.req_set_ref_ref()->size(), 0);
  EXPECT_EQ(a.req_map_ref_ref()->size(), 0);
  EXPECT_EQ(a.req_list_ref_unique_ref()->size(), 0);
  EXPECT_EQ(a.req_set_ref_shared_ref()->size(), 0);
  EXPECT_EQ(a.req_list_ref_shared_const_ref()->size(), 0);
  EXPECT_EQ(nullptr, a.opt_list_ref_ref());
  EXPECT_EQ(nullptr, a.opt_set_ref_ref());
  EXPECT_EQ(nullptr, a.opt_map_ref_ref());
  EXPECT_EQ(nullptr, a.opt_list_ref_unique_ref());
  EXPECT_EQ(nullptr, a.opt_set_ref_shared_ref());
  EXPECT_EQ(nullptr, a.opt_list_ref_shared_const_ref());
  EXPECT_FALSE(a.opt_box_list_ref_ref().has_value());
  EXPECT_FALSE(a.opt_box_set_ref_ref().has_value());
  EXPECT_FALSE(a.opt_box_map_ref_ref().has_value());
}

TEST(References, field_ref) {
  cpp2::ReferringStruct a;

  static_assert(std::is_same_v<
                decltype(a.def_field_ref()),
                std::unique_ptr<PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(std::move(a).def_field_ref()),
                std::unique_ptr<PlainStruct>&&>);
  static_assert(std::is_same_v<
                decltype(std::as_const(a).def_field_ref()),
                const std::unique_ptr<PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(std::move(std::as_const(a)).def_field_ref()),
                const std::unique_ptr<PlainStruct>&&>);
  static_assert(std::is_same_v<
                decltype(a.def_shared_field_ref()),
                std::shared_ptr<PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(std::move(a).def_shared_field_ref()),
                std::shared_ptr<PlainStruct>&&>);
  static_assert(std::is_same_v<
                decltype(std::as_const(a).def_shared_field_ref()),
                const std::shared_ptr<PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(std::move(std::as_const(a)).def_shared_field_ref()),
                const std::shared_ptr<PlainStruct>&&>);
  static_assert(std::is_same_v<
                decltype(a.def_shared_const_field_ref()),
                std::shared_ptr<const PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(std::move(a).def_shared_const_field_ref()),
                std::shared_ptr<const PlainStruct>&&>);
  static_assert(std::is_same_v<
                decltype(std::as_const(a).def_shared_const_field_ref()),
                const std::shared_ptr<const PlainStruct>&>);
  static_assert(
      std::is_same_v<
          decltype(std::move(std::as_const(a)).def_shared_const_field_ref()),
          const std::shared_ptr<const PlainStruct>&&>);

  a.def_field_ref() = std::make_unique<PlainStruct>();
  a.def_field_ref()->field_ref() = 10;
  auto x = std::move(a).def_field_ref();
  EXPECT_EQ(x->field_ref(), 10);
  EXPECT_FALSE(a.def_field_ref());

  a.def_field_ref() = std::make_unique<PlainStruct>();
  a.def_field_ref()->field_ref() = 20;
  auto y = std::move(a.def_field_ref());
  EXPECT_EQ(y->field_ref(), 20);
  EXPECT_FALSE(a.def_field_ref());
}

TEST(References, structured_annotation) {
  StructuredAnnotation a;
  EXPECT_EQ(nullptr, a.opt_unique_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_field_ref());
  EXPECT_EQ(nullptr, a.opt_shared_mutable_field_ref());
  static_assert(std::is_same_v<
                decltype(a.opt_unique_field_ref()),
                std::unique_ptr<PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(a.opt_shared_field_ref()),
                std::shared_ptr<const PlainStruct>&>);
  static_assert(std::is_same_v<
                decltype(a.opt_shared_mutable_field_ref()),
                std::shared_ptr<PlainStruct>&>);

  PlainStruct plain;
  plain.field_ref() = 10;
  a.opt_unique_field_ref() = std::make_unique<PlainStruct>(plain);
  plain.field_ref() = 20;
  a.opt_shared_field_ref() = std::make_shared<const PlainStruct>(plain);
  plain.field_ref() = 30;
  a.opt_shared_mutable_field_ref() = std::make_shared<PlainStruct>(plain);

  EXPECT_EQ(10, a.opt_unique_field_ref()->field_ref());
  EXPECT_EQ(20, a.opt_shared_field_ref()->field_ref());
  EXPECT_EQ(30, a.opt_shared_mutable_field_ref()->field_ref());
}

TEST(References, string_ref) {
  StructWithString a;
  static_assert(std::is_same_v<
                decltype(a.def_unique_string_ref_ref()),
                std::unique_ptr<std::string>&>);
  static_assert(std::is_same_v<
                decltype(a.def_shared_string_ref_ref()),
                std::shared_ptr<std::string>&>);
  static_assert(std::is_same_v<
                decltype(a.def_shared_string_const_ref_ref()),
                std::shared_ptr<const std::string>&>);
  EXPECT_EQ("...", *a.def_unique_string_ref_ref());
  EXPECT_EQ("...", *a.def_shared_string_ref_ref());
  EXPECT_EQ("...", *a.def_shared_string_const_ref_ref());

  *a.def_unique_string_ref_ref() = "a";
  *a.def_shared_string_ref_ref() = "b";

  auto data = SimpleJSONSerializer::serialize<std::string>(a);
  StructWithString b;
  SimpleJSONSerializer::deserialize(data, b);
  EXPECT_EQ(a, b);
}

TEST(References, CppRefUnionLessThan) {
  auto check = [](ReferringUnionWithCppRef& smallerAddress,
                  ReferringUnionWithCppRef& largerAddress) {
    *smallerAddress.get_box_string() = "2";
    *largerAddress.get_box_string() = "1";

    EXPECT_LT(smallerAddress.get_box_string(), largerAddress.get_box_string());
    EXPECT_GT(
        *smallerAddress.get_box_string(), *largerAddress.get_box_string());
    EXPECT_GT(smallerAddress, largerAddress);
  };

  ReferringUnionWithCppRef a;
  ReferringUnionWithCppRef b;
  a.set_box_string("");
  b.set_box_string("");
  if (a.get_box_string() < b.get_box_string()) {
    check(a, b);
  } else {
    check(b, a);
  }
}

TEST(References, CppRefUnionSetterGetter) {
  ReferringUnionWithCppRef a;
  PlainStruct p;
  p.field() = 42;

  a.set_box_plain(p);

  EXPECT_THROW(a.get_box_string(), bad_field_access);
  EXPECT_THROW(a.get_box_self(), bad_field_access);
  EXPECT_EQ(a.get_box_plain()->field(), 42);

  a.set_box_string("foo");

  EXPECT_THROW(a.get_box_plain(), bad_field_access);
  EXPECT_THROW(a.get_box_self(), bad_field_access);
  EXPECT_EQ(*a.get_box_string(), "foo");

  ReferringUnionWithCppRef b;

  b.set_box_self(a);

  EXPECT_THROW(b.get_box_string(), bad_field_access);
  EXPECT_THROW(b.get_box_plain(), bad_field_access);
  EXPECT_THROW(*b.get_box_self()->get_box_plain(), bad_field_access);
  EXPECT_THROW(*b.get_box_self()->get_box_self(), bad_field_access);
  EXPECT_EQ(*b.get_box_self()->get_box_string(), "foo");
}

TEST(References, UnionFieldRef) {
  ReferringUnion a;
  PlainStruct p;
  p.field() = 42;

  a.box_plain_ref() = p;

  EXPECT_FALSE(a.box_string_ref());
  EXPECT_TRUE(a.box_plain_ref());
  EXPECT_FALSE(a.box_self_ref());
  EXPECT_EQ(a.box_plain_ref()->field(), 42);
  EXPECT_THROW(a.box_string_ref().value(), bad_field_access);

  a.box_string_ref().emplace("foo");

  EXPECT_TRUE(a.box_string_ref());
  EXPECT_FALSE(a.box_plain_ref());
  EXPECT_FALSE(a.box_self_ref());
  EXPECT_EQ(a.box_string_ref(), "foo");

  ReferringUnion b;

  b.box_self_ref() = a;

  EXPECT_FALSE(b.box_string_ref());
  EXPECT_FALSE(b.box_plain_ref());
  EXPECT_TRUE(b.box_self_ref());
  EXPECT_TRUE(b.box_self_ref()->box_string_ref());
  EXPECT_FALSE(b.box_self_ref()->box_plain_ref());
  EXPECT_FALSE(b.box_self_ref()->box_self_ref());
  EXPECT_EQ(b.box_self_ref()->box_string_ref(), "foo");
}

TEST(References, UnionLessThan) {
  auto check = [](ReferringUnion& smallerAddress,
                  ReferringUnion& largerAddress) {
    smallerAddress.box_string_ref() = "2";
    largerAddress.box_string_ref() = "1";
    EXPECT_LT(
        &*smallerAddress.box_string_ref(), &*largerAddress.box_string_ref());
    EXPECT_GT(smallerAddress.box_string_ref(), largerAddress.box_string_ref());
    EXPECT_GT(smallerAddress, largerAddress);
  };

  ReferringUnion a;
  ReferringUnion b;
  a.box_string_ref() = "";
  b.box_string_ref() = "";
  if (&*a.box_string_ref() < &*b.box_string_ref()) {
    check(a, b);
  } else {
    check(b, a);
  }
}

} // namespace cpp2
