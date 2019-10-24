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

#include <algorithm>
#include <array>
#include <random>

#include <folly/container/Array.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/nimble/Decoder.h>
#include <thrift/lib/cpp2/protocol/nimble/Encoder.h>

namespace apache {
namespace thrift {
namespace detail {

TEST(EncoderDecoderTest, EndToEnd) {
  // + 123 and + 789 just to make it an unusual number.
  const int kFieldBytes = 1000 * 1000 + 123;
  const int kNumContentChunks = 1000 * 1000 + 789;
  const int kNumStrs = 100 * 1000;
  const int kMaxStrLength = 100;
  const int kJunkChar = 111;

  auto interestingValues = folly::make_array<std::uint32_t>(
      0,
      1,
      2,
      123,
      255,
      256,
      257,
      12345,
      65535,
      65536,
      65537,
      100 * 1000,
      (std::uint32_t)-1,
      (std::uint32_t)-2,
      (std::uint32_t)-100,
      123 * 456);

  std::minstd_rand gen;
  std::uniform_int_distribution<> dist(0, interestingValues.size() - 1);
  auto valueGen = [&] { return interestingValues[dist(gen)]; };

  std::uniform_int_distribution<> fieldByteLenGen(1, 3);
  std::vector<std::uint32_t> contentChunks(kNumContentChunks);

  std::generate(contentChunks.begin(), contentChunks.end(), valueGen);

  std::uniform_int_distribution<char> charDist;
  std::uniform_int_distribution<> sizeDist(0, kMaxStrLength);
  auto randString = [&] {
    std::string result(sizeDist(gen), '\0');
    std::generate(result.begin(), result.end(), [&] { return charDist(gen); });
    return result;
  };

  std::string fieldBytes(kFieldBytes, '\0');
  std::generate(
      fieldBytes.begin(), fieldBytes.end(), [&] { return charDist(gen); });

  std::vector<std::string> binaryBytes(kNumStrs);
  std::generate(binaryBytes.begin(), binaryBytes.end(), randString);

  Encoder enc;

  std::uniform_int_distribution<int> fieldLenDist(1, 3);
  unsigned nextFieldByteInd = 0;
  while (nextFieldByteInd < kFieldBytes) {
    unsigned nextLen = fieldLenDist(gen);
    if (nextFieldByteInd + nextLen >= fieldBytes.size()) {
      nextLen = fieldBytes.size() - nextFieldByteInd;
    }
    nimble::FieldBytes nextBytes;
    nextBytes.len = nextLen;
    for (unsigned i = 0; i < nextLen; ++i) {
      nextBytes.bytes[i] = fieldBytes[nextFieldByteInd];
      ++nextFieldByteInd;
    }
    enc.encodeFieldBytes(nextBytes);
  }

  for (std::uint32_t chunk : contentChunks) {
    enc.encodeContentChunk(chunk);
  }

  for (auto& str : binaryBytes) {
    enc.encodeBinary(str.data(), str.size());
  }

  auto message = enc.finalize();

  Decoder dec;
  dec.setInput(folly::io::Cursor{message.get()});

  nextFieldByteInd = 0;
  std::uniform_int_distribution<> coinflip(0, 1);
  while (nextFieldByteInd < fieldBytes.size()) {
    // Randomly select whether or not we grab 1 or 2 bytes at a time; we should
    // be correct regardless of whether or not the order of insertion matches.
    if (coinflip(gen) || nextFieldByteInd == fieldBytes.size() - 1) {
      EXPECT_EQ(fieldBytes[nextFieldByteInd], dec.nextFieldByte());
      nextFieldByteInd += 1;
    } else {
      EXPECT_EQ(
          (unsigned)fieldBytes[nextFieldByteInd] |
              ((unsigned)fieldBytes[nextFieldByteInd + 1] << 8),
          dec.nextFieldShort());
      nextFieldByteInd += 2;
    }
  }

  for (std::uint32_t chunk : contentChunks) {
    EXPECT_EQ(chunk, dec.nextContentChunk());
  }

  for (const auto& str : binaryBytes) {
    std::array<char, kMaxStrLength> buf;
    std::memset(buf.data(), kJunkChar, buf.size());
    dec.nextBinary(buf.data(), str.size());
    EXPECT_EQ(0, std::memcmp(str.data(), buf.data(), str.size()));
    for (int i = str.size(); i < kMaxStrLength; ++i) {
      EXPECT_EQ(kJunkChar, buf.data()[i]);
    }
  }
}

TEST(EncoderDecoderTest, EmptyStreams) {
  // It's not correct to have an empty field stream, but it is OK to have an
  // empty content or binary stream.
  Encoder enc;
  enc.encodeFieldBytes(nimble::stopBytes());
  auto message = enc.finalize();
  Decoder dec;
  dec.setInput(folly::io::Cursor{message.get()});
  EXPECT_EQ(0, dec.nextFieldByte());
}

} // namespace detail
} // namespace thrift
} // namespace apache
