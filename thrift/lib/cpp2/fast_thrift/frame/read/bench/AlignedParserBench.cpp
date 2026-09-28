/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

/**
 * Compares both parsers on the same wire bytes through getReadBuffer and
 * consume. That is the loop a plaintext AsyncSocket runs, with recv replaced
 * by memcpy.
 *
 * The median of three @fbcode//mode/opt-clang-lto runs on 2026-09-25 was:
 *   - REQUEST_RESPONSE with metadata, 100 B: 36.38 ns for FrameLengthParser
 *     and 112.14 ns for AlignedParser;
 *   - REQUEST_RESPONSE with metadata, 1 KiB: 62.75 ns and 148.08 ns;
 *   - REQUEST_RESPONSE with metadata, 64 KiB: 2.02 us and 2.25 us;
 *   - REQUEST_RESPONSE without metadata, 100 B: 33.95 ns and 65.34 ns;
 *   - PAYLOAD, 1 KiB: 60.70 ns and 95.65 ns;
 *   - REQUEST_FNF, 100 B: 33.81 ns and 100.72 ns;
 *   - REQUEST_FNF, 64 KiB: 1.96 us for both parsers.
 *
 * AlignedParser uses separate header, metadata and data buffers for a
 * REQUEST_RESPONSE with metadata. PAYLOAD uses separate header and data
 * buffers. REQUEST_FNF uses the plain queue path.
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include <glog/logging.h>
#include <folly/Benchmark.h>
#include <folly/Range.h>
#include <folly/init/Init.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/AlignedParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/FrameLengthParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/WireFrames.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameHeaders.h>

using apache::thrift::fast_thrift::channel_pipeline::BytesPtr;
using apache::thrift::fast_thrift::channel_pipeline::Result;
using apache::thrift::fast_thrift::frame::read::AlignedParser;
using apache::thrift::fast_thrift::frame::read::FrameLengthParser;
using apache::thrift::fast_thrift::frame::read::serializeFrame;
using apache::thrift::fast_thrift::frame::write::PayloadHeader;
using apache::thrift::fast_thrift::frame::write::RequestFnfHeader;
using apache::thrift::fast_thrift::frame::write::RequestResponseHeader;

namespace {

constexpr size_t kMetadataSize = 64;
constexpr size_t kSmallDataSize = 100;
constexpr size_t kMediumDataSize = 1024;
constexpr size_t kLargeDataSize = 64 * 1024;

BytesPtr filledBuffer(size_t size, uint8_t value) {
  BytesPtr buffer = folly::IOBuf::create(size);
  std::memset(buffer->writableData(), value, size);
  buffer->append(size);
  return buffer;
}

std::vector<uint8_t> buildRequestResponseFrame(
    size_t dataSize, bool withMetadata) {
  BytesPtr metadata;
  if (withMetadata) {
    metadata = filledBuffer(kMetadataSize, 'm');
  }
  return serializeFrame(
      RequestResponseHeader{.streamId = 1},
      std::move(metadata),
      filledBuffer(dataSize, 'd'));
}

std::vector<uint8_t> buildPlainFrame(size_t dataSize) {
  return serializeFrame(
      RequestFnfHeader{.streamId = 1}, nullptr, filledBuffer(dataSize, 'd'));
}

std::vector<uint8_t> buildPayloadFrame(size_t dataSize) {
  return serializeFrame(
      PayloadHeader{.streamId = 1, .next = true},
      nullptr,
      filledBuffer(dataSize, 'd'));
}

auto countingSink(size_t& framesRead) noexcept {
  return [&framesRead](BytesPtr&& frame) noexcept {
    folly::doNotOptimizeAway(frame.get());
    ++framesRead;
    return Result::Success;
  };
}

template <typename Parser>
void consumeBytes(
    Parser& parser, folly::ByteRange remaining, size_t& framesRead) {
  auto sink = countingSink(framesRead);
  while (!remaining.empty()) {
    void* buffer = nullptr;
    size_t room = 0;
    parser.getReadBuffer(&buffer, &room);
    CHECK_GT(room, 0);

    const size_t length = std::min(room, remaining.size());
    std::memcpy(CHECK_NOTNULL(buffer), remaining.data(), length);
    remaining.advance(length);

    const Result result = parser.consume(length, sink);
    CHECK(result == Result::Success);
    folly::doNotOptimizeAway(result);
  }
}

struct BenchmarkFrames {
  std::vector<uint8_t> small;
  std::vector<uint8_t> medium;
  std::vector<uint8_t> large;
  std::vector<uint8_t> smallWithoutMetadata;
  std::vector<uint8_t> mediumPayload;
  std::vector<uint8_t> smallPlain;
  std::vector<uint8_t> largePlain;
};

const BenchmarkFrames& benchmarkFrames() {
  static const BenchmarkFrames frames{
      buildRequestResponseFrame(kSmallDataSize, true),
      buildRequestResponseFrame(kMediumDataSize, true),
      buildRequestResponseFrame(kLargeDataSize, true),
      buildRequestResponseFrame(kSmallDataSize, false),
      buildPayloadFrame(kMediumDataSize),
      buildPlainFrame(kSmallDataSize),
      buildPlainFrame(kLargeDataSize),
  };
  return frames;
}

template <typename Parser>
void runBenchmark(size_t iterations, const std::vector<uint8_t>& frame) {
  folly::BenchmarkSuspender suspender;
  Parser parser;
  std::vector<uint8_t> bytes;
  bytes.reserve(iterations * frame.size());
  for (size_t i = 0; i < iterations; ++i) {
    bytes.insert(bytes.end(), frame.begin(), frame.end());
  }
  size_t framesRead = 0;

  suspender.dismiss();
  consumeBytes(
      parser, folly::ByteRange{bytes.data(), bytes.size()}, framesRead);
  suspender.rehire();

  CHECK_EQ(framesRead, iterations);
}

} // namespace

BENCHMARK(FrameLengthParser_RequestResponseWithMetadata_Small, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().small);
}

BENCHMARK_RELATIVE(AlignedParser_RequestResponseWithMetadata_Small, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().small);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_RequestResponseWithMetadata_Medium, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().medium);
}

BENCHMARK_RELATIVE(AlignedParser_RequestResponseWithMetadata_Medium, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().medium);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_RequestResponseWithMetadata_Large, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().large);
}

BENCHMARK_RELATIVE(AlignedParser_RequestResponseWithMetadata_Large, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().large);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_RequestResponseWithoutMetadata_Small, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().smallWithoutMetadata);
}

BENCHMARK_RELATIVE(AlignedParser_RequestResponseWithoutMetadata_Small, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().smallWithoutMetadata);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_Payload_Medium, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().mediumPayload);
}

BENCHMARK_RELATIVE(AlignedParser_Payload_Medium, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().mediumPayload);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_RequestFnf_Small, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().smallPlain);
}

BENCHMARK_RELATIVE(AlignedParser_RequestFnf_Small, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().smallPlain);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(FrameLengthParser_RequestFnf_Large, n) {
  runBenchmark<FrameLengthParser>(n, benchmarkFrames().largePlain);
}

BENCHMARK_RELATIVE(AlignedParser_RequestFnf_Large, n) {
  runBenchmark<AlignedParser>(n, benchmarkFrames().largePlain);
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  // Builds the frames now, so no benchmark is timed building them.
  (void)benchmarkFrames();
  folly::runBenchmarks();
  return 0;
}
