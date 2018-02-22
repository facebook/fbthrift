/*
 * Copyright 2016-present Facebook, Inc.
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

#include <thrift/test/gen-cpp2/simple_reflection_types.h>
#include <thrift/test/gen-cpp2/simple_reflection_types_custom_protocol.h>
#include <thrift/test/gen-cpp2/simple_reflection_fatal_types.h>

#include <thrift/lib/cpp2/fatal/internal/test_helpers.h>
#include <thrift/lib/cpp2/fatal/populator.h>
#include <thrift/lib/cpp2/fatal/serializer.h>

#include <thrift/test/fatal_serialization_common.h>

#include <folly/Memory.h>

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

    ASSERT_EQ(a.field1, b.field1);
    ASSERT_EQ(a.field2, b.field2);
    ASSERT_EQ(a.field3, b.field3);
    ASSERT_EQ(a.field4, b.field4);
    ASSERT_EQ(a.field5, b.field5);
    ASSERT_EQ(a.field6, b.field6);
    ASSERT_EQ(a.field7, b.field7);
    ASSERT_EQ(a.field8, b.field8);
    ASSERT_EQ(a.field9, b.field9);
    ASSERT_EQ(*(a.field10), *(b.field10));

    auto abuf = a.field11.coalesce();
    auto bbuf = b.field11.coalesce();

    ASSERT_EQ(abuf.size(), bbuf.size());
    ASSERT_EQ(abuf, bbuf);

    ASSERT_EQ(a.field12, b.field12);
    if(a.field13) {
      ASSERT_EQ(*(a.field13), *(b.field13));
    }
    else {
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
}
} /* namespace cpp_reflection::test_cpp2 */
