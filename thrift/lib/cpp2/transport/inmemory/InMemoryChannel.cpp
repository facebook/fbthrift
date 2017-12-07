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

#include <thrift/lib/cpp2/transport/inmemory/InMemoryChannel.h>

#include <glog/logging.h>
#include <thrift/lib/cpp2/transport/core/EnvelopeUtil.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

namespace apache {
namespace thrift {

using folly::EventBase;
using folly::IOBuf;
using std::map;
using std::string;

InMemoryChannel::InMemoryChannel(ThriftProcessor* processor, EventBase* evb)
    : processor_(processor), evb_(evb) {}

void InMemoryChannel::sendThriftResponse(
    std::unique_ptr<ResponseRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload) noexcept {
  CHECK(evb_->isInEventBaseThread());
  CHECK(metadata);
  CHECK(callback_);
  auto evb = callback_->getEventBase();
  evb->runInEventBaseThread([evbCallback = std::move(callback_),
                             evbMetadata = std::move(metadata),
                             evbPayload = std::move(payload)]() mutable {
    evbCallback->onThriftResponse(
        std::move(evbMetadata), std::move(evbPayload));
  });
}

void InMemoryChannel::sendThriftRequest(
    std::unique_ptr<RequestRpcMetadata> metadata,
    std::unique_ptr<IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  CHECK(evb_->isInEventBaseThread());
  CHECK(metadata);
  CHECK(payload);
  if (!EnvelopeUtil::stripEnvelope(metadata.get(), payload)) {
    LOG(ERROR) << "Unexpected problem stripping envelope";
    auto evb = callback->getEventBase();
    evb->runInEventBaseThread([cb = std::move(callback)]() mutable {
      cb->onError(folly::exception_wrapper(
          std::runtime_error("Unexpected problem stripping envelope")));
    });
    return;
  }
  CHECK(metadata->__isset.kind);
  if (metadata->kind == RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE ||
      metadata->kind == RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE) {
    CHECK(callback);
    callback_ = std::move(callback);
  }
  metadata->seqId = 0;
  metadata->__isset.seqId = true;
  processor_->onThriftRequest(
      std::move(metadata), std::move(payload), shared_from_this());
}

folly::EventBase* InMemoryChannel::getEventBase() noexcept {
  return evb_;
}

} // namespace thrift
} // namespace apache
