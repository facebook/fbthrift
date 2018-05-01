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

#include <thrift/test/gen-cpp2/References_types.h>
#include <thrift/test/gen-cpp2/References_types.tcc>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <gtest/gtest.h>

using namespace apache::thrift;

namespace cpp2 {

TEST(References, nonopt_ref_fields) {
  SimpleJSONProtocolWriter writer;
  folly::IOBufQueue buff;
  writer.setOutput(&buff, 1024);

  EXPECT_EQ(nullptr, buff.front());

  cpp2::RefStruct a;
  EXPECT_EQ(nullptr, a.def_field.get());
  EXPECT_EQ(nullptr, a.req_field.get());
  EXPECT_EQ(nullptr, a.opt_field.get());

  // this isn't the correct serialized size, but it's what simple json returns.
  // it is the correct length for a manually inspected, correct serializedSize
  EXPECT_EQ(120, a.serializedSize(&writer));
  EXPECT_EQ(120, a.serializedSizeZC(&writer));

  if(buff.front()) {
    EXPECT_EQ(0, buff.front()->length());
  }

  a.def_field = std::make_unique<cpp2::RefStruct>();
  a.opt_field = std::make_unique<cpp2::RefStruct>();
  EXPECT_EQ(415, a.serializedSize(&writer));
  EXPECT_EQ(415, a.serializedSizeZC(&writer));
}

TEST(References, ref_container_fields) {
  StructWithContainers a;

  // tests that we initialize ref container fields
  EXPECT_NE(nullptr, a.list_ref);
  EXPECT_NE(nullptr, a.set_ref);
  EXPECT_NE(nullptr, a.map_ref);
  EXPECT_NE(nullptr, a.list_ref_unique);
  EXPECT_NE(nullptr, a.set_ref_shared);
  EXPECT_NE(nullptr, a.list_ref_shared_const);
}

} // namespace cpp2
