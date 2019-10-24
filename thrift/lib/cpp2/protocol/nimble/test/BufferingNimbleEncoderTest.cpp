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

#include <array>
#include <random>

#include <folly/container/Array.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleDecoder.h>
#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleEncoder.h>

using namespace apache::thrift::detail;

TEST(BufferingNimbleEncoder, EndToEnd) {
  const int kIters = 10 * 1000 * 1000;

  auto interestingValues = folly::make_array<std::uint32_t>(
      0,
      1,
      2,
      255,
      256,
      257,
      65535,
      65536,
      65537,
      100 * 1000,
      (std::uint32_t)-1,
      123 * 456);

  folly::IOBufQueue control;
  folly::IOBufQueue data;
  BufferingNimbleEncoder<ChunkRepr::kRaw> enc;
  enc.setControlOutput(&control);
  enc.setDataOutput(&data);

  std::minstd_rand gen0;
  std::uniform_int_distribution<> dist0(0, interestingValues.size() - 1);
  for (int i = 0; i < kIters; ++i) {
    enc.encodeChunk(interestingValues[dist0(gen0)]);
  }
  enc.finalize();

  BufferingNimbleDecoder<ChunkRepr::kRaw> dec;
  dec.setControlInput(folly::io::Cursor{control.front()});
  dec.setDataInput(folly::io::Cursor{data.front()});

  std::minstd_rand gen1;
  std::uniform_int_distribution<> dist1(0, interestingValues.size() - 1);
  for (int i = 0; i < kIters; ++i) {
    EXPECT_EQ(interestingValues[dist1(gen1)], dec.nextChunk());
  }
}
