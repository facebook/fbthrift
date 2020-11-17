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

#include <cstdio>
#include <stdexcept>
#include <vector>

#include <folly/init/Init.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/data/TestGenerator.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

using apache::thrift::conformance::getStandardProtocol;
using apache::thrift::conformance::StandardProtocol;

int main(int argc, char** argv) {
  folly::Init(&argc, &argv);
  std::vector<folly::StringPiece> protocolNames;

  auto testSuite = apache::thrift::conformance::data::createRoundTripSuite(
      {getStandardProtocol<StandardProtocol::Compact>(),
       getStandardProtocol<StandardProtocol::Binary>(),
       getStandardProtocol<StandardProtocol::SimpleJson>()});

  apache::thrift::BinaryProtocolWriter writer;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  writer.setOutput(&queue);
  testSuite.write(&writer);
  while (auto buf = queue.pop_front()) {
    ::fwrite(buf->data(), sizeof(uint8_t), buf->length(), stdout);
  }
}
