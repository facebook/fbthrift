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

#include <thrift/test/gen-cpp2/structs_terse_types.h>
#include <thrift/test/gen-cpp2/structs_types.h>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

using namespace apache::thrift::test;

class StructTest : public testing::Test {};

TEST_F(StructTest, compilation_terse_writes_refs_shared) {
  BasicRefsSharedTerseWrites a;
  (void)a;
}

TEST_F(StructTest, copy_ctor_refs_annot_cpp_noexcept_move_ctor) {
  {
    BasicRefsAnnotCppNoexceptMoveCtor a;
    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 3;

    BasicRefsAnnotCppNoexceptMoveCtor b(a);
    EXPECT_EQ(3, b.def_field->field);
  }
}

TEST_F(StructTest, copy_assign_refs_annot_cpp_noexcept_move_ctor) {
  {
    BasicRefsAnnotCppNoexceptMoveCtor a;
    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 3;

    BasicRefsAnnotCppNoexceptMoveCtor b;
    b = a;
    EXPECT_EQ(3, b.def_field->field);
  }
}

TEST_F(StructTest, equal_to) {
  std::equal_to<Basic> op;

  {
    Basic a;
    Basic b;

    a.def_field = 3;
    b.def_field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = 4;
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

    a.req_field = 3;
    b.req_field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = 4;
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

    a.def_field = "hello";
    b.def_field = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = "world";
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

    a.req_field = "hello";
    b.req_field = "hello";
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.req_field = "world";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = "world";
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

    a.def_field = nullptr;
    b.def_field = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = std::make_unique<HasInt>();
    b.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field->field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

TEST_F(StructTest, equal_to_refs_shared) {
  std::equal_to<BasicRefsShared> op;

  {
    BasicRefsShared a;
    BasicRefsShared b;

    a.def_field = nullptr;
    b.def_field = nullptr;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    a.def_field = std::make_shared<HasInt>();
    a.def_field->field = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = std::make_shared<HasInt>();
    b.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field->field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field = a.def_field;
    EXPECT_TRUE(op(a, b));
    EXPECT_TRUE(op(b, a));
  }
}

TEST_F(StructTest, less) {
  std::less<Basic> op;

  {
    Basic a;
    Basic b;

    b.def_field = 3;
    a.def_field = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field = 4;
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

    b.req_field = 3;
    a.req_field = 3;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = 4;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.req_field = 4;
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

    b.def_field = "hello";
    a.def_field = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.__isset.def_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.__isset.def_field = true;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field = "world";
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

    b.req_field = "hello";
    a.req_field = "hello";
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.req_field = "world";
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.req_field = "world";
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

    b.def_field = nullptr;
    a.def_field = nullptr;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = std::make_unique<HasInt>();
    b.def_field->field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

TEST_F(StructTest, less_refs_shared) {
  std::less<BasicRefsShared> op;

  {
    BasicRefsShared a;
    BasicRefsShared b;

    b.def_field = nullptr;
    a.def_field = nullptr;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = std::make_unique<HasInt>();
    b.def_field->field = 3;
    EXPECT_TRUE(op(a, b));
    EXPECT_FALSE(op(b, a));

    a.def_field = std::make_unique<HasInt>();
    a.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_TRUE(op(b, a));

    b.def_field->field = 4;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));

    b.def_field = a.def_field;
    EXPECT_FALSE(op(a, b));
    EXPECT_FALSE(op(b, a));
  }
}

TEST_F(StructTest, custom_indirection) {
  IOBufIndirection a;
  a.foo.raw = folly::IOBuf(folly::IOBuf::COPY_BUFFER, "test");
  a.bar.raw = "test2";
  IOBufIndirection b = a;
  EXPECT_EQ(a, b);
}

TEST_F(StructTest, small_sorted_vector) {
  using Set = SmallSortedVectorSet<int32_t>;
  using Map = SmallSortedVectorMap<int32_t, int32_t>;
  using serializer = apache::thrift::BinarySerializer;
  using Type = HasSmallSortedVector;
  EXPECT_TRUE((std::is_same<decltype(Type::set_field), Set>::value));
  EXPECT_TRUE((std::is_same<decltype(Type::map_field), Map>::value));

  Type o;
  o.set_field.insert({1, 3, 5});
  o.map_field.insert({{1, 4}, {3, 12}, {5, 20}});
  auto a = serializer::deserialize<HasSmallSortedVector>(
      serializer::serialize<std::string>(o));
  EXPECT_EQ(o, a);
  EXPECT_EQ(o.set_field, a.set_field);
  EXPECT_EQ(o.map_field, a.map_field);
}

TEST_F(StructTest, noexcept_move_annotation) {
  EXPECT_TRUE(std::is_nothrow_move_constructible<NoexceptMoveStruct>::value);
  EXPECT_TRUE(std::is_nothrow_move_assignable<NoexceptMoveStruct>::value);
  NoexceptMoveStruct a;
  a.set_string_field("hello world");
  NoexceptMoveStruct b(std::move(a));
  EXPECT_EQ(b.get_string_field(), "hello world");
  NoexceptMoveStruct c;
  c = std::move(b);
  EXPECT_EQ(c.get_string_field(), "hello world");
}
