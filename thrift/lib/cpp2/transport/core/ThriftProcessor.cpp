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
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <string>

namespace apache {
namespace thrift {

using apache::thrift::concurrency::ThreadManager;
using folly::IOBuf;

ThriftProcessor::ThriftProcessor(
    std::unique_ptr<AsyncProcessor> cpp2Processor,
    const apache::thrift::server::ServerConfigs& serverConfigs)
    : cpp2Processor_(std::move(cpp2Processor)), serverConfigs_(serverConfigs) {}

void ThriftProcessor::onThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel,
    std::unique_ptr<Cpp2ConnContext> connContext) noexcept {
  DCHECK(metadata);
  DCHECK(payload);
  DCHECK(channel);
  DCHECK(tm_);

  bool invalidMetadata =
      !(metadata->__isset.protocol && metadata->__isset.name &&
        metadata->__isset.kind && metadata->__isset.seqId);

  auto request = std::make_unique<ThriftRequest>(
      serverConfigs_, channel, std::move(metadata), std::move(connContext));

  auto* evb = channel->getEventBase();
  if (UNLIKELY(invalidMetadata)) {
    LOG(ERROR) << "Invalid metadata object";
    evb->runInEventBaseThread([request = std::move(request)]() {
      request->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::UNSUPPORTED_CLIENT_TYPE,
              "invalid metadata object"),
          "corrupted metadata");
    });
    return;
  }

  auto protoId = request->getProtoId();
  auto reqContext = request->getRequestContext();
  cpp2Processor_->process(
      std::move(request), std::move(payload), protoId, reqContext, evb, tm_);
}
} // namespace thrift
} // namespace apache
