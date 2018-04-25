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

#include <thrift/lib/cpp2/transport/core/testutil/CoreTestFixture.h>

#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <string>

namespace apache {
namespace thrift {

using namespace testutil::testservice;

CoreTestFixture::CoreTestFixture()
    : threadManager_(std::make_shared<FakeThreadManager>()),
      processor_(serverConfigs_) {
  threadManager_->start();
  processor_.setThreadManager(threadManager_.get());
  processor_.setCpp2Processor(service_.getProcessor());
  channel_ = std::make_shared<FakeChannel>(&eventBase_);
}

CoreTestFixture::~CoreTestFixture() {
  threadManager_->join();
}

void CoreTestFixture::runInEventBaseThread(folly::Function<void()> test) {
  eventBase_.runInEventBaseThread(std::move(test));
  eventBase_.loop();
}

void CoreTestFixture::serializeSumTwoNumbers(
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

int32_t CoreTestFixture::deserializeSumTwoNumbers(folly::IOBuf* buf) {
  auto reader = std::make_unique<apache::thrift::CompactProtocolReader>();
  int32_t result;
  std::string fname;
  apache::thrift::MessageType mtype;
  int32_t protoSeqId;
  reader->setInput(buf);
  reader->readMessageBegin(fname, mtype, protoSeqId);
  TestService_sumTwoNumbers_presult args;
  args.get<0>().value = &result;
  args.read(reader.get());
  reader->readMessageEnd();
  return result;
}

std::unique_ptr<RequestRpcMetadata>
CoreTestFixture::makeMetadata(std::string name, int32_t seqId, RpcKind kind) {
  auto metadata = std::make_unique<RequestRpcMetadata>();
  metadata->protocol = ProtocolId::COMPACT;
  metadata->__isset.protocol = true;
  metadata->name = name;
  metadata->__isset.name = true;
  metadata->seqId = seqId;
  metadata->__isset.seqId = true;
  metadata->kind = kind;
  metadata->__isset.kind = true;
  return metadata;
}

bool CoreTestFixture::deserializeException(
    folly::IOBuf* buf,
    TApplicationException* tae) {
  try {
    auto reader = std::make_unique<apache::thrift::CompactProtocolReader>();
    std::string fname;
    apache::thrift::MessageType mtype;
    int32_t protoSeqId;
    reader->setInput(buf);
    reader->readMessageBegin(fname, mtype, protoSeqId);
    EXPECT_EQ(T_EXCEPTION, mtype);
    tae->read(reader.get());
    return true;
  } catch (...) {
    return false;
  }
}

} // namespace thrift
} // namespace apache
