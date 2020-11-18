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

#include <thrift/test/testset/Testset.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/cpp2/ThriftTypes.h>

namespace apache::thrift::test::testset {
namespace {
using namespace apache::thrift::conformance::type;

template <typename T1, typename T2>
struct SameType;

template <typename T>
struct SameType<T, T> {};

TEST(TestsetTest, StructWith) {
  SameType<struct_i64, struct_with<i64_t>>();
  SameType<
      struct_optional_float,
      struct_with<float_t, FieldModifier::Optional>>();
  SameType<
      struct_optional_string_cpp_ref,
      struct_with<
          string_t,
          FieldModifier::Optional,
          FieldModifier::Reference>>();
  // Order of field modifiers doesn't matter.
  SameType<
      struct_optional_string_cpp_ref,
      struct_with<
          string_t,
          FieldModifier::Reference,
          FieldModifier::Optional>>();
}

TEST(TestsetTest, ExceptionWith) {
  SameType<exception_i64, exception_with<i64_t>>();
  SameType<
      exception_optional_float,
      exception_with<float_t, FieldModifier::Optional>>();
  SameType<
      exception_optional_string_cpp_ref,
      exception_with<
          string_t,
          FieldModifier::Optional,
          FieldModifier::Reference>>();
  // Order of field modifiers doesn't matter.
  SameType<
      exception_optional_string_cpp_ref,
      exception_with<
          string_t,
          FieldModifier::Reference,
          FieldModifier::Optional>>();
}

TEST(TestsetTest, UnionWith) {
  SameType<union_set_binary, union_with<set<binary_t>>>();
  SameType<
      union_map_string_binary_cpp_ref,
      union_with<map<string_t, binary_t>, FieldModifier::Reference>>();
}

} // namespace
} // namespace apache::thrift::test::testset
