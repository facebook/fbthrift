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

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/test/gen-cpp2/DebugProtoTest_types.h>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::test;

// Globals so that the read test can read the write test data.
OneOfEach ooe;
unique_ptr<IOBuf> buf;

template <typename TBufferType_>
void runTestWrite(int iters) {
  TBufferType_ prot;
  size_t bufSize = ooe.serializedSizeZC(&prot);
  folly::IOBufQueue queue;

  for (int i = 0; i < iters; i++) {
    queue.clear();
    prot.setOutput(&queue, bufSize);
    ooe.write(&prot);
  }

  buf = queue.move();
}

BENCHMARK(runTestWrite_BinaryProtocolWriter, iters) {
  runTestWrite<BinaryProtocolWriter>(iters);
}

template <typename TBufferType_>
void runTestRead(int iters) {
  OneOfEach ooe2;
  TBufferType_ prot;
  for (int i = 0; i < iters; i++) {
    prot.setInput(buf.get());
    ooe2.read(&prot);
  }
}

BENCHMARK(runTestRead_BinaryProtocolReader, iters) {
  runTestRead<BinaryProtocolReader>(iters);
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);

  *ooe.im_true_ref() = true;
  *ooe.im_false_ref() = false;
  *ooe.a_bite_ref() = 0xd6;
  *ooe.integer16_ref() = 27000;
  *ooe.integer32_ref() = 1 << 24;
  *ooe.integer64_ref() = (uint64_t)6000 * 1000 * 1000;
  *ooe.double_precision_ref() = M_PI;
  *ooe.some_characters_ref() = "JSON THIS! \"\1";
  *ooe.zomg_unicode_ref() = "\xd7\n\a\t";
  *ooe.base64_ref() = "\1\2\3\255";
  ooe.string_string_map_ref()["one"] = "two";
  ooe.string_string_hash_map_ref()["three"] = "four";
  *ooe.float_precision_ref() = (float)12.345;
  ooe.rank_map_ref()[567419810] = (float)0.211184;
  ooe.rank_map_ref()[507959914] = (float)0.080382;

  folly::runBenchmarks();

  return 0;
}
