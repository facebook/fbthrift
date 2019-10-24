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

#include <folly/portability/GFlags.h>

#include <thrift/lib/cpp2/protocol/nimble/BufferingNimbleEncoder.h>
#include <thrift/lib/cpp2/protocol/nimble/test/BufferingEncoderDecoderBenchmarksCommon.h>

using folly::BenchmarkSuspender;
using folly::runBenchmarks;

namespace apache {
namespace thrift {
namespace detail {

template <ChunkRepr repr>
void doRunWithData(unsigned iters, std::vector<std::uint32_t> vals) {
  const int kSize = 1000 * 1000;
  BenchmarkSuspender braces;

  std::vector<std::uint32_t> chunks;
  for (int i = 0; i < kSize; ++i) {
    chunks.push_back(vals[i % vals.size()]);
  }

  braces.dismiss();
  for (unsigned i = 0; i < iters; ++i) {
    folly::IOBufQueue controlOut;
    folly::IOBufQueue dataOut;

    BufferingNimbleEncoder<repr> encoder;
    encoder.setControlOutput(&controlOut);
    encoder.setDataOutput(&dataOut);

    for (std::uint32_t chunk : chunks) {
      encoder.encodeChunk(chunk);
    }

    encoder.finalize();
  }
}

} // namespace detail
} // namespace thrift
} // namespace apache

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  using apache::thrift::detail::ChunkRepr;
  using apache::thrift::detail::doRunWithData;
  apache::thrift::detail::addAllBenchmarks(
      __FILE__,
      &doRunWithData<ChunkRepr::kRaw>,
      &doRunWithData<ChunkRepr::kZigzag>);
  folly::runBenchmarks();
  return 0;
}
