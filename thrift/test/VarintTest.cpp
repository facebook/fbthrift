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

#include <thrift/lib/cpp/util/VarintUtils.h>

#include <folly/portability/GTest.h>

using namespace apache::thrift::util;
using namespace std;
using folly::IOBuf;
using namespace folly::io;

TEST(VarintTest, Varint) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(1000));
  iobuf1->append(1000);
  RWPrivateCursor wcursor(iobuf1.get());
  Cursor rcursor(iobuf1.get());
  int64_t v = 1;
  writeVarint(wcursor, 0);
  for (int bit = 0; bit < 64; bit++, v <<= int(bit < 64)) {
    if (bit < 8) {
      writeVarint(wcursor, int8_t(v));
    }
    if (bit < 16) {
      writeVarint(wcursor, int16_t(v));
    }
    if (bit < 32) {
      writeVarint(wcursor, int32_t(v));
    }
    writeVarint(wcursor, v);
  }
  int32_t oversize = 1000000;
  writeVarint(wcursor, oversize);

  EXPECT_EQ(0, readVarint<int8_t>(rcursor));
  v = 1;
  for (int bit = 0; bit < 64; bit++, v <<= int(bit < 64)) {
    if (bit < 8) {
      EXPECT_EQ(int8_t(v), readVarint<int8_t>(rcursor));
    }
    if (bit < 16) {
      EXPECT_EQ(int16_t(v), readVarint<int16_t>(rcursor));
    }
    if (bit < 32) {
      EXPECT_EQ(int32_t(v), readVarint<int32_t>(rcursor));
    }
    EXPECT_EQ(v, readVarint<int64_t>(rcursor));
  }
  EXPECT_THROW(readVarint<uint8_t>(rcursor), out_of_range);
}
