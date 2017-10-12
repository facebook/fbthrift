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

using namespace apache::thrift;

void TestServiceMock::serializeSumTwoNumbers(
    int32_t x,
    int32_t y,
    bool wrongMethodName,
    folly::IOBufQueue* request,
    RequestRpcMetadata* metadata) {
  std::string methodName = "sumTwoNumbers";
  if (wrongMethodName) {
    methodName = "wrongMethodName";
  }
  TestService_sumTwoNumbers_pargs args;
  args.get<0>().value = &x;
  args.get<1>().value = &y;

  auto writer = std::make_unique<apache::thrift::CompactProtocolWriter>();
  writer->setOutput(request);
  args.write(writer.get());
  metadata->protocol = ProtocolId::COMPACT;
  metadata->__isset.protocol = true;
  if (wrongMethodName) {
    metadata->name = "wrongMethodName";
  } else {
    metadata->name = "sumTwoNumbers";
  }
  metadata->__isset.name = true;
  metadata->kind = RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  metadata->__isset.kind = true;
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
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
  auto header = getConnectionContext()->getHeader();

  // Even if the method throws or not, put the header value and check if reaches
  // to the client in any case or not
  header->setHeader("header_from_server", "1");

  auto keyValue = header->getHeaders();
  if (keyValue.find("unexpected_exception") != keyValue.end()) {
    throw std::runtime_error("unexpected exception");
  }

  if (keyValue.find("expected_exception") != keyValue.end()) {
    TestServiceException exception;
    exception.message = "expected exception";
    throw exception;
  }

  if (keyValue.find("header_from_client") == keyValue.end() ||
      keyValue.find("header_from_client")->second != "2") {
    TestServiceException exception;
    exception.message = "Expected key/value, foo:bar, is missing";
    throw exception;
  }
}

} // namespace testservice
} // namespace testutil
