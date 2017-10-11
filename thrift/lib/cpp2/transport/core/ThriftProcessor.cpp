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

void ThriftProcessor::onThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::shared_ptr<ThriftChannelIf> channel) noexcept {
  DCHECK(metadata);
  DCHECK(payload);
  DCHECK(channel);
  DCHECK(tm_);
  DCHECK(metadata->__isset.protocol);
  DCHECK(metadata->__isset.name);
  DCHECK(metadata->__isset.seqId);
  DCHECK(metadata->__isset.kind);

  std::unique_ptr<ThriftRequest> request =
      std::make_unique<ThriftRequest>(channel, std::move(metadata));
  auto protoId = request->getProtoId();
  auto reqContext = request->getRequestContext();
  auto* evb = channel->getEventBase();
  cpp2Processor_->process(
      std::move(request), std::move(payload), protoId, reqContext, evb, tm_);
}

void ThriftProcessor::cancel(
    int32_t /*seqId*/,
    ThriftChannelIf* /*channel*/) noexcept {
  // Processor currently ignores cancellations.
}

} // namespace thrift
} // namespace apache
