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

#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

#include <glog/logging.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using apache::thrift::concurrency::ThreadManager;
using folly::IOBuf;

void ThriftProcessor::onThriftRequest(
    std::unique_ptr<FunctionInfo> functionInfo,
    std::unique_ptr<map<string, string>> headers,
    std::unique_ptr<IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  DCHECK(functionInfo->kind == SINGLE_REQUEST_SINGLE_RESPONSE);
  DCHECK(headers);
  DCHECK(payload);
  DCHECK(channel);

  auto* evb = channel->getEventBase();

  // TODO: Move this loop into channel and do not call ThriftProcessor
  // if payload is empty.
  while (payload->length() == 0) {
    if (payload->next() != payload.get()) {
      payload = payload->pop();
    } else {
      LOG(FATAL) << "Empty payload.";
    }
  }

  auto header = std::make_unique<transport::THeader>();
  header->setHeaders(std::move(*headers));

  // TODO: Looks like this code can be placed after call to
  // deserializeMessageBegin().  Looks like we are repeating
  // some work here.
  auto protByte = payload->data()[0];
  switch (protByte) {
    case 0x80:
      header->setProtocolId(protocol::T_BINARY_PROTOCOL);
      break;
    case 0x82:
      header->setProtocolId(protocol::T_COMPACT_PROTOCOL);
      break;
    // TODO: Add Frozen2 case.
    default:
      LOG(FATAL) << "Bad protocol";
  }
  auto protoId = static_cast<apache::thrift::protocol::PROTOCOL_TYPES>(
      header->getProtocolId());

  auto connContext = std::make_unique<Cpp2ConnContext>(
      nullptr, nullptr, nullptr, nullptr, nullptr);
  auto context =
      std::make_unique<Cpp2RequestContext>(connContext.get(), header.get());
  auto rawContext = context.get();
  std::unique_ptr<ResponseChannel::Request> request =
      std::make_unique<ThriftRequest>(
          channel,
          std::move(header),
          std::move(context),
          std::move(connContext),
          functionInfo->seqId);

  apache::thrift::detail::ap::deserializeMessageBegin(
      protoId, request, payload.get(), rawContext, evb);
  cpp2Processor_->process(
      std::move(request), std::move(payload), protoId, rawContext, evb, tm_);
}

void ThriftProcessor::cancel(
    uint32_t /*seqId*/,
    ThriftChannelIf* /*channel*/) noexcept {
  // Processor currently ignores cancellations.
}

} // namespace thrift
} // namespace apache
