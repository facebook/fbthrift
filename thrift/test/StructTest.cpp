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

#include <memory>
#include <thrift/test/gen-cpp2/structs_terse_types.h>
#include <thrift/test/gen-cpp2/structs_types.h>

#include <folly/Traits.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace apache::thrift::test;

class StructTest : public testing::Test {};

TEST_F(StructTest, compilation_terse_writes_refs_shared) {
  BasicRefsSharedTerseWrites a;
  (void)a;
}

TEST_F(StructTest, serialization_terse_writes_refs_shared) {
  using apache::thrift::CompactSerializer;

  BasicRefsSharedTerseWrites a;

  a.shared_field_ref() = std::make_shared<HasInt>();
  a.shared_field_ref()->field_ref() = 3;

  a.shared_fields_ref() = std::make_shared<std::vector<HasInt>>();
  a.shared_fields_ref()->emplace_back();
  a.shared_fields_ref()->back().field_ref() = 4;
  a.shared_fields_ref()->emplace_back();
  a.shared_fields_ref()->back().field_ref() = 5;
  a.shared_fields_ref()->emplace_back();
  a.shared_fields_ref()->back().field_ref() = 6;

  a.shared_fields_const_ref() = std::make_shared<const std::vector<HasInt>>();

  const std::string serialized = CompactSerializer::serialize<std::string>(a);

  BasicRefsSharedTerseWrites b;
  CompactSerializer::deserialize(serialized, b);

  EXPECT_EQ(a, b);
}

TEST_F(StructTest, serialization_terse_writes_default_values) {
  using apache::thrift::CompactSerializer;

  BasicRefsSharedTerseWrites empty;

  BasicRefsSharedTerseWrites defaults;
  defaults.shared_field_req_ref() = std::make_shared<HasInt>();
  defaults.shared_fields_req_ref() = std::make_shared<std::vector<HasInt>>();

  // This struct has terse writes enabled, so the default values set above
  // should not be part of the serialization.
  EXPECT_EQ(
      CompactSerializer::serialize<std::string>(empty),
      CompactSerializer::serialize<std::string>(defaults));
}

TEST_F(StructTest, equal_to) {
  std::equal_to<Basic> op;

  {
    Basic a;
    Basic b;

    *a.def_field_ref() = 3;
    *b.def_field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field_ref().ensure();
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field_ref().ensure();
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *a.def_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.def_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    *a.req_field_ref() = 3;
    *b.req_field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *a.req_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.req_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    a.req_field_ref() = 3;
    b.req_field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

// currently, binary fields are handled specially, so they get their own tests
TEST_F(StructTest, equal_to_binary) {
  std::equal_to<BasicBinaries> op;

  {
    BasicBinaries a;
    BasicBinaries b;

    *a.def_field_ref() = "hello";
    *b.def_field_ref() = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field_ref().ensure();
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field_ref().ensure();
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *a.def_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.def_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field_ref() = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.opt_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    *a.req_field_ref() = "hello";
    *b.req_field_ref() = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *a.req_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.req_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    a.req_field_ref() = "hello";
    b.req_field_ref() = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

// current equal-to codegen implements def/opt/req ref fields with a single
// code path, so no need to test the different cases separately
TEST_F(StructTest, equal_to_refs) {
  std::equal_to<BasicRefs> op;

  {
    BasicRefs a;
    BasicRefs b;

    a.def_field_ref() = nullptr;
    b.def_field_ref() = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field_ref() = std::make_unique<HasInt>();
    *a.def_field_ref()->field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_unique<HasInt>();
    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.def_field_ref()->field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
  {
    BasicRefs a;
    BasicRefs b;

    a.def_field_ref() = nullptr;
    b.def_field_ref() = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field_ref() = std::make_unique<HasInt>();
    *a.def_field_ref()->field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_unique<HasInt>();
    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.def_field_ref()->field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

TEST_F(StructTest, equal_to_refs_shared) {
  std::equal_to<BasicRefsShared> op;

  {
    BasicRefsShared a;
    BasicRefsShared b;

    a.def_field_ref() = nullptr;
    b.def_field_ref() = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field_ref() = std::make_shared<HasInt>();
    *a.def_field_ref()->field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_shared<HasInt>();
    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.def_field_ref()->field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field_ref() = a.def_field_ref();
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

TEST_F(StructTest, less) {
  std::less<Basic> op;

  {
    Basic a;
    Basic b;

    *b.def_field_ref() = 3;
    *a.def_field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref().ensure();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field_ref().ensure();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.def_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.def_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.opt_field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.opt_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    *b.req_field_ref() = 3;
    *a.req_field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.req_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.req_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    Basic a;
    Basic b;

    b.req_field_ref() = 3;
    a.req_field_ref() = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field_ref() = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.req_field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

// currently, binary fields are handled specially, so they get their own tests
TEST_F(StructTest, less_binary) {
  std::less<BasicBinaries> op;

  {
    BasicBinaries a;
    BasicBinaries b;

    *b.def_field_ref() = "hello";
    *a.def_field_ref() = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref().ensure();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field_ref().ensure();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.def_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.def_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.opt_field_ref() = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.opt_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.opt_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    *b.req_field_ref() = "hello";
    *a.req_field_ref() = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *b.req_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    *a.req_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    BasicBinaries a;
    BasicBinaries b;

    b.req_field_ref() = "hello";
    a.req_field_ref() = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field_ref() = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.req_field_ref() = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

// current less codegen implements def/opt/req ref fields with a single
// code path, so no need to test the different cases separately
TEST_F(StructTest, less_refs) {
  std::less<BasicRefs> op;

  {
    BasicRefs a;
    BasicRefs b;

    b.def_field_ref() = nullptr;
    a.def_field_ref() = nullptr;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_unique<HasInt>();
    *b.def_field_ref()->field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field_ref() = std::make_unique<HasInt>();
    *a.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

TEST_F(StructTest, less_refs_shared) {
  std::less<BasicRefsShared> op;

  {
    BasicRefsShared a;
    BasicRefsShared b;

    b.def_field_ref() = nullptr;
    a.def_field_ref() = nullptr;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_unique<HasInt>();
    *b.def_field_ref()->field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field_ref() = std::make_unique<HasInt>();
    *a.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = a.def_field_ref();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
  {
    BasicRefsShared a;
    BasicRefsShared b;

    b.def_field_ref() = nullptr;
    a.def_field_ref() = nullptr;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = std::make_unique<HasInt>();
    *b.def_field_ref()->field_ref() = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field_ref() = std::make_unique<HasInt>();
    *a.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_TRUE(op(b, a));

    *b.def_field_ref()->field_ref() = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field_ref() = a.def_field_ref();
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

TEST_F(StructTest, custom_indirection) {
  IOBufIndirection a;
  a.foo_ref()->raw = folly::IOBuf(folly::IOBuf::COPY_BUFFER, "test");
  a.bar_ref()->raw = "test2";
  IOBufIndirection b = a;
  EXPECT_EQ(a, b);
}

TEST_F(StructTest, small_sorted_vector) {
  using Set = SmallSortedVectorSet<int32_t>;
  using Map = SmallSortedVectorMap<int32_t, int32_t>;
  using serializer = apache::thrift::BinarySerializer;
  using Type = HasSmallSortedVector;
  EXPECT_TRUE((std::is_same<
               folly::remove_cvref_t<decltype(*Type().set_field_ref())>,
               Set>::value));
  EXPECT_TRUE((std::is_same<
               folly::remove_cvref_t<decltype(*Type().map_field_ref())>,
               Map>::value));

  Type o;
  o.set_field_ref()->insert({1, 3, 5});
  o.map_field_ref()->insert({{1, 4}, {3, 12}, {5, 20}});
  auto a = serializer::deserialize<HasSmallSortedVector>(
      serializer::serialize<std::string>(o));
  EXPECT_EQ(o, a);
  EXPECT_EQ(*o.set_field_ref(), *a.set_field_ref());
  EXPECT_EQ(*o.map_field_ref(), *a.map_field_ref());
}

TEST_F(StructTest, noexcept_move_annotation) {
  EXPECT_TRUE(std::is_nothrow_move_constructible<NoexceptMoveStruct>::value);
  EXPECT_TRUE(std::is_nothrow_move_assignable<NoexceptMoveStruct>::value);
  NoexceptMoveStruct a;
  a.string_field_ref() = "hello world";
  NoexceptMoveStruct b(std::move(a));
  EXPECT_EQ(b.get_string_field(), "hello world");
  NoexceptMoveStruct c;
  c = std::move(b);
  EXPECT_EQ(c.get_string_field(), "hello world");
}

TEST_F(StructTest, clear) {
  Basic obj;
  obj.def_field_ref() = 7;
  obj.req_field_ref() = 8;
  obj.opt_field_ref() = 9;
  apache::thrift::clear(obj);
  EXPECT_FALSE(obj.def_field_ref().is_set());
  EXPECT_EQ(0, *obj.def_field_ref());
  EXPECT_EQ(0, *obj.req_field_ref());
  EXPECT_FALSE(obj.opt_field_ref());
}

TEST_F(StructTest, BasicIndirection) {
  BasicIndirection obj;
  obj.raw.def_field_ref() = 7;
  obj.raw.req_field_ref() = 8;
  obj.raw.opt_field_ref() = 9;
  apache::thrift::CompactSerializer ser;
  auto obj2 =
      ser.deserialize<BasicIndirection>(ser.serialize<std::string>(obj));
  EXPECT_EQ(obj.raw, obj2.raw);
}

TEST_F(StructTest, CppDataMethod) {
  CppDataMethod obj;
  obj.foo_ref() = 10;
  EXPECT_EQ(obj._data().foo_ref(), 10);
  obj._data().foo_ref() = 20;
  EXPECT_EQ(obj._data().foo_ref(), 20);
  EXPECT_EQ(std::as_const(obj)._data().foo_ref(), 20);
  EXPECT_EQ(std::move(obj)._data().foo_ref(), 20);
}
