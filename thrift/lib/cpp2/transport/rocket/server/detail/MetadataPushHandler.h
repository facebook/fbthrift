/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#pragma once

#include <folly/ExceptionWrapper.h>
#include <thrift/lib/cpp2/async/StreamPayload.h>
#include <thrift/lib/cpp2/server/LoggingEventTransportMetadata.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::rocket {

template <typename ConnectionT, template <typename> class ConnectionAdapter>
class MetadataPushHandler {
  using Connection = ConnectionAdapter<ConnectionT>;

 public:
  MetadataPushHandler(const MetadataPushHandler&) = delete;
  MetadataPushHandler(MetadataPushHandler&&) = delete;
  MetadataPushHandler& operator=(const MetadataPushHandler&) = delete;
  MetadataPushHandler& operator=(MetadataPushHandler&&) = delete;
  MetadataPushHandler(Connection& connection) noexcept
      : connection_(&connection) {}

  void handle(MetadataPushFrame&& frame) noexcept;

 private:
  Connection* connection_;
};

// Implementation needs to be in header for template instantiation
template <typename ConnectionT, template <typename> class ConnectionAdapter>
void MetadataPushHandler<ConnectionT, ConnectionAdapter>::handle(
    MetadataPushFrame&& frame) noexcept {
  MetadataPushFrame metadataFrame(std::move(frame));
  ClientPushMetadata clientMeta;
  try {
    connection_->getWrappedConnection()->getPayloadSerializer()->unpack(
        clientMeta, metadataFrame.metadata(), false);
  } catch (...) {
    connection_->close(
        folly::make_exception_wrapper<RocketException>(
            ErrorCode::INVALID, "Failed to deserialize metadata push frame"));
    return;
  }

  switch (clientMeta.getType()) {
    case ClientPushMetadata::Type::interactionTerminate: {
      connection_->getFrameHandler()->terminateInteraction(
          *clientMeta.interactionTerminate()->interactionId());
      break;
    }
    case ClientPushMetadata::Type::streamHeadersPush: {
      StreamId sid(clientMeta.streamHeadersPush()->streamId().value_or(0));
      auto it = connection_->getWrappedConnection()->findStream(sid);
      if (it != connection_->getWrappedConnection()->streamsEnd()) {
        it->second->handleStreamHeadersPush(HeadersPayload(
            clientMeta.streamHeadersPush()->headersPayloadContent().value_or(
                {})));
      }
      break;
    }
    case ClientPushMetadata::Type::transportMetadataPush: {
      if (auto context = connection_->getFrameHandler()->getCpp2ConnContext()) {
        auto md = clientMeta.transportMetadataPush()->transportMetadata();
        std::optional<folly::F14NodeMap<std::string, std::string>> metadata;
        if (md) {
          metadata = std::move(*md);
        }
        logTransportMetadata(*context, std::move(metadata));
      }
      break;
    }
    default:
      break;
  }
}

} // namespace apache::thrift::rocket
