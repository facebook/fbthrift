/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/server/ThriftRocketServerHandler.h>

#include <memory>
#include <utility>

#include <fmt/core.h>
#include <folly/ExceptionString.h>
#include <folly/ExceptionWrapper.h>
#include <folly/GLog.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <rsocket/RSocketParameters.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>

#include <thrift/lib/cpp2/transport/rsocket/gen-cpp2/Config_types.h>
#include <thrift/lib/cpp2/util/Checksum.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
bool deserializeMetadata(
    const folly::IOBuf& buffer,
    RequestRpcMetadata& metadata) {
  try {
    CompactProtocolReader reader;
    reader.setInput(&buffer);
    metadata.read(&reader);
    return true;
  } catch (const std::exception& ex) {
    FB_LOG_EVERY_MS(ERROR, 10000)
        << "Exception on deserializing metadata: " << folly::exceptionStr(ex);
    return false;
  }
}

bool isMetadataValid(const RequestRpcMetadata& metadata) {
  return metadata.protocol_ref() && metadata.name_ref() &&
      metadata.kind_ref() && metadata.seqId_ref();
}
} // namespace

ThriftRocketServerHandler::ThriftRocketServerHandler(
    std::shared_ptr<Cpp2Worker> worker,
    const folly::SocketAddress& clientAddress,
    const folly::AsyncTransportWrapper* transport)
    : worker_(std::move(worker)),
      cpp2Processor_(worker_->getServer()->getCpp2Processor()),
      threadManager_(worker_->getServer()->getThreadManager()),
      serverConfigs_(*worker_->getServer()),
      clientAddress_(clientAddress),
      connContext_(std::make_shared<Cpp2ConnContext>(
          &clientAddress_,
          transport,
          nullptr, /* eventBaseManager */
          nullptr, /* duplexChannel */
          nullptr, /* x509PeerCert */
          worker_->getServer()->getClientIdentityHook())) {}

void ThriftRocketServerHandler::handleSetupFrame(
    SetupFrame&& frame,
    RocketServerFrameContext&& context) {
  auto& connection = context.connection();
  auto* metadata = frame.payload().metadata().get();
  if (!metadata) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Missing required metadata in SETUP frame"));
  }

  folly::io::Cursor cursor(frame.payload().metadata().get());

  // Validate Thrift major/minor version
  int16_t majorVersion;
  int16_t minorVersion;
  const bool success = cursor.tryReadBE<int16_t>(majorVersion) &&
      cursor.tryReadBE<int16_t>(minorVersion);
  if (!success || majorVersion != 0 || minorVersion != 1) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP, "Incompatible Thrift version"));
  }

  try {
    CompactProtocolReader reader;
    reader.setInput(cursor);
    RSocketSetupParameters thriftSetupParams;
    // Throws on read error
    thriftSetupParams.read(&reader);
  } catch (const std::exception& e) {
    return connection.close(folly::make_exception_wrapper<RocketException>(
        ErrorCode::INVALID_SETUP,
        fmt::format(
            "Error deserializing SETUP payload: {}",
            folly::exceptionStr(e).toStdString())));
  }
}

void ThriftRocketServerHandler::handleRequestResponseFrame(
    RequestResponseFrame&& frame,
    RocketServerFrameContext&& context) {
  auto makeRequestResponse = [&](RequestRpcMetadata&& md) {
    return std::make_unique<ThriftServerRequestResponse>(
        *worker_->getEventBase(),
        serverConfigs_,
        std::move(md),
        connContext_,
        std::move(context));
  };

  handleRequestCommon(
      std::move(frame.payload()), std::move(makeRequestResponse));
}

void ThriftRocketServerHandler::handleRequestFnfFrame(
    RequestFnfFrame&& frame,
    RocketServerFrameContext&& context) {
  auto makeRequestFnf = [&](RequestRpcMetadata&& md) {
    return std::make_unique<ThriftServerRequestFnf>(
        *worker_->getEventBase(),
        serverConfigs_,
        std::move(md),
        connContext_,
        std::move(context),
        [keepAlive = cpp2Processor_] {});
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestFnf));
}

void ThriftRocketServerHandler::handleRequestStreamFrame(
    RequestStreamFrame&& frame,
    std::shared_ptr<RocketServerStreamSubscriber> subscriber) {
  auto makeRequestStream = [&](RequestRpcMetadata&& md) {
    return std::make_unique<ThriftServerRequestStream>(
        *worker_->getEventBase(),
        serverConfigs_,
        std::move(md),
        connContext_,
        std::move(subscriber),
        cpp2Processor_);
  };

  handleRequestCommon(std::move(frame.payload()), std::move(makeRequestStream));
}

template <class F>
void ThriftRocketServerHandler::handleRequestCommon(
    Payload&& payload,
    F&& makeRequest) {
  RequestRpcMetadata metadata;
  const bool parseOk = deserializeMetadata(*payload.metadata(), metadata);

  auto data = std::move(payload).data();
  const bool validMetadata = parseOk && isMetadataValid(metadata);
  const bool badChecksum = validMetadata && metadata.crc32c_ref() &&
      (*metadata.crc32c_ref() != checksum::crc32c(*data));

  if (validMetadata && !badChecksum) {
    if (UNLIKELY(serverConfigs_.isOverloaded(
            metadata.otherMetadata_ref() ? &*metadata.otherMetadata_ref()
                                         : nullptr,
            &*metadata.name_ref()))) {
      handleRequestOverloadedServer(makeRequest(std::move(metadata)));
      return;
    }

    auto request = makeRequest(std::move(metadata));
    const auto protocolId = request->getProtoId();
    auto* const cpp2ReqCtx = request->getRequestContext();
    cpp2Processor_->process(
        std::move(request),
        std::move(data),
        protocolId,
        cpp2ReqCtx,
        worker_->getEventBase(),
        threadManager_.get());
  } else if (!validMetadata) {
    handleRequestWithBadMetadata(makeRequest(std::move(metadata)));
  } else {
    handleRequestWithBadChecksum(makeRequest(std::move(metadata)));
  }
}

void ThriftRocketServerHandler::handleRequestWithBadMetadata(
    std::unique_ptr<ThriftRequestCore> request) {
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::UNSUPPORTED_CLIENT_TYPE,
          "Invalid metadata object"),
      "Corrupted metadata in rsocket request");
}

void ThriftRocketServerHandler::handleRequestWithBadChecksum(
    std::unique_ptr<ThriftRequestCore> request) {
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::CHECKSUM_MISMATCH, "Checksum mismatch"),
      "Corrupted request");
}

void ThriftRocketServerHandler::handleRequestOverloadedServer(
    std::unique_ptr<ThriftRequestCore> request) {
  request->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, "Loadshedding request"),
      serverConfigs_.getOverloadedErrorCode());
}

} // namespace rocket
} // namespace thrift
} // namespace apache
