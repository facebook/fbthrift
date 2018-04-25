/*
 * Copyright 2014-present Facebook, Inc.
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

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/frozen/Frozen.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Binary_layouts.h>

using namespace apache::thrift::frozen;
using namespace apache::thrift::test;
using namespace folly;

namespace {
byte test[]{0xDE, 0xAD, 0x00, 0xBE, 0xEF};
ByteRange testRange(test, sizeof(test));
StringPiece testString(testRange);

byte test2[]{0xFA, 0xCE, 0xB0, 0x0C};
ByteRange test2Range(test2, sizeof(test2));
}

TEST(FrozenIOBuf, Thrift2) {
  Binaries b2;
  b2.normal = testString.str();
  b2.iobuf = IOBuf::copyBuffer(testRange.data(), testRange.size());

  auto fb2 = freeze(b2);
  EXPECT_EQ(testString, fb2.normal());
  EXPECT_EQ(testRange, fb2.iobuf());
}

TEST(FrozenIOBuf, IOBufChain) {
  Binaries b2;
  auto buf1 = IOBuf::copyBuffer(testRange.data(), testRange.size());
  auto buf2 = IOBuf::copyBuffer(test2Range.data(), test2Range.size());
  buf1->appendChain(std::move(buf2));
  b2.iobuf = std::move(buf1);

  auto fb2 = freeze(b2);
  EXPECT_EQ(0, fb2.normal().size());
  EXPECT_EQ(9, fb2.iobuf().size());
  auto combined = fb2.iobuf();
  EXPECT_TRUE(combined.startsWith(testRange));
  EXPECT_TRUE(combined.endsWith(test2Range));
}
