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

#include <folly/Memory.h>

#include <thrift/lib/cpp2/reflection/internal/test_helpers.h>
#include <thrift/lib/cpp2/reflection/populator.h>
#include <thrift/lib/cpp2/reflection/serializer.h>
#include <thrift/test/fatal_serialization_common.h>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_types_custom_protocol.h>

namespace test_cpp2 {
namespace simple_cpp_reflection {

using namespace apache::thrift;
using namespace apache::thrift::test;
using apache::thrift::populator::populator_opts;

TYPED_TEST_CASE(MultiProtocolTest, protocol_type_pairs);

TYPED_TEST(MultiProtocolTest, test_structs_populate) {
  populator_opts opts;
  std::mt19937 rng;

  for (int i = 0; i < 100; i++) {
    struct7 a, b;
    populator::populate(a, opts, rng);
    serializer_write(a, this->writer);
    this->prep_read();
    this->debug_buffer();
    serializer_read(b, this->reader);

    ASSERT_EQ(*a.field1_ref(), *b.field1_ref());
    ASSERT_EQ(*a.field2_ref(), *b.field2_ref());
    ASSERT_EQ(*a.field3_ref(), *b.field3_ref());
    ASSERT_EQ(*a.field4_ref(), *b.field4_ref());
    ASSERT_EQ(*a.field5_ref(), *b.field5_ref());
    ASSERT_EQ(*a.field6_ref(), *b.field6_ref());
    ASSERT_EQ(*a.field7_ref(), *b.field7_ref());
    ASSERT_EQ(*a.field8_ref(), *b.field8_ref());
    ASSERT_EQ(*a.field9_ref(), *b.field9_ref());
    ASSERT_EQ(*(a.field10), *(b.field10));

    auto abuf = a.field11_ref()->coalesce();
    auto bbuf = b.field11_ref()->coalesce();

    ASSERT_EQ(abuf.size(), bbuf.size());
    ASSERT_EQ(abuf, bbuf);

    ASSERT_EQ(*a.field12_ref(), *b.field12_ref());
    if (a.field13) {
      ASSERT_EQ(*(a.field13), *(b.field13));
    } else {
      ASSERT_EQ(nullptr, b.field13);
    }
    ASSERT_EQ(*(a.field14), *(b.field14));
    ASSERT_EQ(*(a.field15), *(b.field15));
  }
}

TYPED_TEST(MultiProtocolTest, test_unions_populate) {
  std::mt19937 rng;
  populator_opts opts;

  for (int i = 0; i < 100; i++) {
    union1 a, b;
    populator::populate(a, opts, rng);
    serializer_write(a, this->writer);
    this->prep_read();
    this->debug_buffer();
    serializer_read(b, this->reader);

    ASSERT_EQ(a, b);
  }
}
} // namespace simple_cpp_reflection
} // namespace test_cpp2
