/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/core/testutil/TestServiceMock.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <chrono>
#include <string>
#include <thread>

namespace testutil {
namespace testservice {

folly::IOBufQueue TestServiceMock::serializeSumTwoNumbers(
    int32_t x,
    int32_t y,
    bool wrongMethodName) {
  std::string methodName = "sumTwoNumbers";
  if (wrongMethodName) {
    methodName = "wrongMethodName";
  }
  TestService_sumTwoNumbers_pargs args;
  args.get<0>().value = &x;
  args.get<1>().value = &y;

  auto writer = std::make_unique<apache::thrift::CompactProtocolWriter>();
  size_t bufSize = args.serializedSizeZC(writer.get());
  bufSize += writer->serializedMessageSize(methodName);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  writer->setOutput(&queue, bufSize);
  writer->writeMessageBegin(methodName, apache::thrift::T_CALL, 0);
  args.write(writer.get());
  writer->writeMessageEnd();
  return queue;
}

int32_t TestServiceMock::deserializeSumTwoNumbers(folly::IOBuf* buf) {
  auto reader = std::make_unique<apache::thrift::CompactProtocolReader>();
  int32_t result;
  std::string fname;
  apache::thrift::MessageType mtype;
  int32_t protoSeqId = 0;

  reader->setInput(buf);
  reader->readMessageBegin(fname, mtype, protoSeqId);
  TestService_sumTwoNumbers_presult args;
  args.get<0>().value = &result;
  args.read(reader.get());
  reader->readMessageEnd();

  return result;
}

void TestServiceMock::throwExpectedException(int32_t) {
  TestServiceException exception;
  exception.message = "mock_service_method_exception";
  throw exception;
}

void TestServiceMock::throwUnexpectedException(int32_t) {
  throw std::runtime_error("mock_runtime_error");
}

void TestServiceMock::sleep(int32_t timeMs) {
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::milliseconds(timeMs));
}

void TestServiceMock::headers() {
  // TODO: implement
}

} // namespace testservice
} // namespace testutil
