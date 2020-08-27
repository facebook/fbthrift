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

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/test/metadata/gen-cpp2/nested_structs_test_metadata.h> // @manual=:nested_structs_test_thrift-cpp2-services

namespace apache::thrift::detail::md {

TEST(Foo, base) {
  metadata::ThriftMetadata md;
  const metadata::ThriftStruct& ts =
      StructMetadata<::metadata::test::nested_structs::Foo>::gen(md);
  EXPECT_EQ(md.structs_ref()->size(), 1);
  EXPECT_EQ(&ts, &md.structs_ref()->at("nested_structs_test.Foo"));
}

TEST(City, base) {
  metadata::ThriftMetadata md;
  const metadata::ThriftStruct& ts =
      StructMetadata<::metadata::test::nested_structs::City>::gen(md);
  EXPECT_EQ(md.structs_ref()->size(), 2);
  EXPECT_EQ(&ts, &md.structs_ref()->at("nested_structs_test.City"));
}

} // namespace apache::thrift::detail::md
