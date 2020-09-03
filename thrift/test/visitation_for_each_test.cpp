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

#include <thrift/test/gen-cpp2/reflection_for_each_field.h>
#include <thrift/test/gen-cpp2/reflection_no_metadata_for_each_field.h>

#include <folly/Overload.h>
#include <folly/portability/GTest.h>

#include <typeindex>

using namespace std;
using namespace apache::thrift;

namespace test_cpp2 {
namespace cpp_reflection {

TEST(struct1, test_metadata) {
  struct1 s;
  for_each_field(s, [i = 0](auto& meta, auto&& ref) mutable {
    EXPECT_EQ(*meta.id_ref(), 1 << i);
    EXPECT_EQ(*meta.name_ref(), "field" + to_string(i));
    EXPECT_EQ(
        meta.type_ref()->getType(),
        (vector{
            meta.type_ref()->t_primitive,
            meta.type_ref()->t_primitive,
            meta.type_ref()->t_enum,
            meta.type_ref()->t_enum,
            meta.type_ref()->t_union,
            meta.type_ref()->t_union,
        })[i]);
    EXPECT_EQ(
        *meta.is_optional_ref(),
        (vector{false, true, false, false, true, false})[i]);
    EXPECT_EQ(
        type_index(typeid(ref)),
        (vector<type_index>{
            typeid(required_field_ref<int32_t&>),
            typeid(optional_field_ref<string&>),
            typeid(field_ref<enum1&>),
            typeid(required_field_ref<enum2&>),
            typeid(optional_field_ref<union1&>),
            typeid(field_ref<union2&>),
        })[i]);

    // required field always has value
    EXPECT_EQ(
        ref.has_value(), (vector{true, false, false, true, false, false})[i]);
    ++i;
  });
}

TEST(struct1, modify_field) {
  struct1 s;
  s.field0_ref() = 10;
  s.field1_ref() = "20";
  s.field2_ref() = enum1::field0;
  s.field3_ref() = enum2::field1_2;
  s.field4_ref().emplace().set_us("foo");
  s.field5_ref().emplace().set_us_2("bar");
  auto run = folly::overload(
      [](int32_t& ref) {
        EXPECT_EQ(ref, 10);
        ref = 20;
      },
      [](string& ref) {
        EXPECT_EQ(ref, "20");
        ref = "30";
      },
      [](enum1& ref) {
        EXPECT_EQ(ref, enum1::field0);
        ref = enum1::field1;
      },
      [](enum2& ref) {
        EXPECT_EQ(ref, enum2::field1_2);
        ref = enum2::field2_2;
      },
      [](union1& ref) {
        EXPECT_EQ(ref.get_us(), "foo");
        ref.set_ui(20);
      },
      [](union2& ref) {
        EXPECT_EQ(ref.get_us_2(), "bar");
        ref.set_ui_2(30);
      },
      [](auto&) { EXPECT_TRUE(false) << "type mismatch"; });
  for_each_field(s, [run](auto&, auto&& ref) {
    EXPECT_TRUE(ref.has_value());
    run(*ref);
  });
  EXPECT_EQ(s.field0_ref(), 20);
  EXPECT_EQ(s.field1_ref(), "30");
  EXPECT_EQ(s.field2_ref(), enum1::field1);
  EXPECT_EQ(s.field3_ref(), enum2::field2_2);
  EXPECT_EQ(s.field4_ref()->get_ui(), 20);
  EXPECT_EQ(s.field5_ref()->get_ui_2(), 30);
}

TEST(hasRefUnique, test_cpp_ref_unique) {
  hasRefUnique s;
  deque<string> names = {
      "aStruct",
      "aList",
      "aSet",
      "aMap",
      "aUnion",
      "anOptionalStruct",
      "anOptionalList",
      "anOptionalSet",
      "anOptionalMap",
      "anOptionalUnion",
  };
  for_each_field(s, [&, i = 0](auto& meta, auto&& ref) mutable {
    EXPECT_EQ(*meta.name_ref(), names[i++]);
    if constexpr (is_same_v<decltype(*ref), deque<string>&>) {
      if (*meta.is_optional_ref()) {
        ref.reset(new decltype(names)(names));
      }
    }
  });

  // for cpp.ref, unqualified field has value by default
  EXPECT_TRUE(s.aStruct_ref());
  EXPECT_TRUE(s.aList_ref()->empty());
  EXPECT_TRUE(s.aSet_ref()->empty());
  EXPECT_TRUE(s.aMap_ref()->empty());
  EXPECT_TRUE(s.aUnion_ref());

  EXPECT_FALSE(s.anOptionalStruct_ref());
  EXPECT_EQ(*s.anOptionalList_ref(), names);
  EXPECT_FALSE(s.anOptionalSet_ref());
  EXPECT_FALSE(s.anOptionalMap_ref());
  EXPECT_FALSE(s.anOptionalUnion_ref());
}

TEST(struct1, test_reference_type) {
  struct1 s;
  for_each_field(s, [](auto& meta, auto&& ref) {
    switch (*meta.id_ref()) {
      case 1:
        EXPECT_TRUE((is_same_v<decltype(*ref), int32_t&>));
        break;
      case 2:
        EXPECT_TRUE((is_same_v<decltype(*ref), string&>));
        break;
      case 4:
        EXPECT_TRUE((is_same_v<decltype(*ref), enum1&>));
        break;
      case 8:
        EXPECT_TRUE((is_same_v<decltype(*ref), enum2&>));
        break;
      case 16:
        EXPECT_TRUE((is_same_v<decltype(*ref), union1&>));
        break;
      case 32:
        EXPECT_TRUE((is_same_v<decltype(*ref), union2&>));
        break;
      default:
        EXPECT_TRUE(false) << *meta.name_ref() << " " << *meta.id_ref();
    }
  });
  for_each_field(move(s), [](auto& meta, auto&& ref) {
    switch (*meta.id_ref()) {
      case 1:
        EXPECT_TRUE((is_same_v<decltype(*ref), int32_t&&>));
        break;
      case 2:
        EXPECT_TRUE((is_same_v<decltype(*ref), string&&>));
        break;
      case 4:
        EXPECT_TRUE((is_same_v<decltype(*ref), enum1&&>));
        break;
      case 8:
        EXPECT_TRUE((is_same_v<decltype(*ref), enum2&&>));
        break;
      case 16:
        EXPECT_TRUE((is_same_v<decltype(*ref), union1&&>));
        break;
      case 32:
        EXPECT_TRUE((is_same_v<decltype(*ref), union2&&>));
        break;
      default:
        EXPECT_TRUE(false) << *meta.name_ref() << " " << *meta.id_ref();
    }
  });
  const struct1 t;
  for_each_field(t, [](auto& meta, auto&& ref) {
    switch (*meta.id_ref()) {
      case 1:
        EXPECT_TRUE((is_same_v<decltype(*ref), const int32_t&>));
        break;
      case 2:
        EXPECT_TRUE((is_same_v<decltype(*ref), const string&>));
        break;
      case 4:
        EXPECT_TRUE((is_same_v<decltype(*ref), const enum1&>));
        break;
      case 8:
        EXPECT_TRUE((is_same_v<decltype(*ref), const enum2&>));
        break;
      case 16:
        EXPECT_TRUE((is_same_v<decltype(*ref), const union1&>));
        break;
      case 32:
        EXPECT_TRUE((is_same_v<decltype(*ref), const union2&>));
        break;
      default:
        EXPECT_TRUE(false) << *meta.name_ref() << " " << *meta.id_ref();
    }
  });
  for_each_field(move(t), [](auto& meta, auto&& ref) {
    switch (*meta.id_ref()) {
      case 1:
        EXPECT_TRUE((is_same_v<decltype(*ref), const int32_t&&>));
        break;
      case 2:
        EXPECT_TRUE((is_same_v<decltype(*ref), const string&&>));
        break;
      case 4:
        EXPECT_TRUE((is_same_v<decltype(*ref), const enum1&&>));
        break;
      case 8:
        EXPECT_TRUE((is_same_v<decltype(*ref), const enum2&&>));
        break;
      case 16:
        EXPECT_TRUE((is_same_v<decltype(*ref), const union1&&>));
        break;
      case 32:
        EXPECT_TRUE((is_same_v<decltype(*ref), const union2&&>));
        break;
      default:
        EXPECT_TRUE(false) << *meta.name_ref() << " " << *meta.id_ref();
    }
  });
}

TEST(structA, test_two_structs_document) {
  structA thrift1;
  thrift1.a_ref() = 10;
  thrift1.b_ref() = "20";
  structA thrift2 = thrift1;

  for_each_field(
      thrift1,
      thrift2,
      [](const apache::thrift::metadata::ThriftField& meta,
         auto field_ref1,
         auto field_ref2) {
        EXPECT_EQ(field_ref1, field_ref2) << *meta.name_ref() << " mismatch";
      });
}

TEST(struct1, test_two_structs_assignment) {
  struct1 s, t;
  s.field0_ref() = 10;
  s.field1_ref() = "11";
  t.field0_ref() = 20;
  t.field1_ref() = "22";
  auto run = folly::overload(
      [](auto& meta,
         required_field_ref<int32_t&> r1,
         required_field_ref<int32_t&> r2) {
        EXPECT_EQ(r1, 10);
        EXPECT_EQ(r2, 20);
        r1 = 30;
        r2 = 40;

        EXPECT_EQ(*meta.name_ref(), "field0");
        EXPECT_EQ(*meta.is_optional_ref(), false);
      },
      [](auto& meta,
         optional_field_ref<string&> r1,
         optional_field_ref<string&> r2) {
        EXPECT_EQ(r1, "11");
        EXPECT_EQ(r2, "22");
        r1 = "33";
        r2 = "44";

        EXPECT_EQ(*meta.name_ref(), "field1");
        EXPECT_EQ(*meta.is_optional_ref(), true);
      },
      [](auto&&...) {});
  for_each_field(s, t, run);
  EXPECT_EQ(s.field0_ref(), 30);
  EXPECT_EQ(s.field1_ref(), "33");
  EXPECT_EQ(t.field0_ref(), 40);
  EXPECT_EQ(t.field1_ref(), "44");
}

} // namespace cpp_reflection
} // namespace test_cpp2

namespace test_cpp2 {
namespace cpp_reflection_no_metadata {

TEST(struct1_no_metadata, test_metadata) {
  struct1 s;
  for_each_field(s, [i = 0](auto& meta, auto&& ref) mutable {
    EXPECT_EQ(*meta.id_ref(), 0);
    EXPECT_EQ(*meta.name_ref(), "");
    EXPECT_EQ(int(meta.type_ref()->getType()), 0);
    EXPECT_EQ(*meta.is_optional_ref(), false);
    EXPECT_EQ(
        type_index(typeid(ref)),
        (vector<type_index>{
            typeid(required_field_ref<int32_t&>),
            typeid(optional_field_ref<string&>),
            typeid(field_ref<enum1&>),
            typeid(required_field_ref<enum2&>),
            typeid(optional_field_ref<union1&>),
            typeid(field_ref<union2&>),
        })[i]);

    // required field always has value
    EXPECT_EQ(
        ref.has_value(), (vector{true, false, false, true, false, false})[i]);
    ++i;
  });
}

TEST(struct1_no_metadata, modify_field) {
  struct1 s;
  s.field0_ref() = 10;
  s.field1_ref() = "20";
  s.field2_ref() = enum1::field0;
  s.field3_ref() = enum2::field1_2;
  s.field4_ref().emplace().set_us("foo");
  s.field5_ref().emplace().set_us_2("bar");
  auto run = folly::overload(
      [](int32_t& ref) {
        EXPECT_EQ(ref, 10);
        ref = 20;
      },
      [](string& ref) {
        EXPECT_EQ(ref, "20");
        ref = "30";
      },
      [](enum1& ref) {
        EXPECT_EQ(ref, enum1::field0);
        ref = enum1::field1;
      },
      [](enum2& ref) {
        EXPECT_EQ(ref, enum2::field1_2);
        ref = enum2::field2_2;
      },
      [](union1& ref) {
        EXPECT_EQ(ref.get_us(), "foo");
        ref.set_ui(20);
      },
      [](union2& ref) {
        EXPECT_EQ(ref.get_us_2(), "bar");
        ref.set_ui_2(30);
      },
      [](auto&) { EXPECT_TRUE(false) << "type mismatch"; });
  for_each_field(s, [run](auto&, auto&& ref) {
    EXPECT_TRUE(ref.has_value());
    run(*ref);
  });
  EXPECT_EQ(s.field0_ref(), 20);
  EXPECT_EQ(s.field1_ref(), "30");
  EXPECT_EQ(s.field2_ref(), enum1::field1);
  EXPECT_EQ(s.field3_ref(), enum2::field2_2);
  EXPECT_EQ(s.field4_ref()->get_ui(), 20);
  EXPECT_EQ(s.field5_ref()->get_ui_2(), 30);
}

} // namespace cpp_reflection_no_metadata
} // namespace test_cpp2
