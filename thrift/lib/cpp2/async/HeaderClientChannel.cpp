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

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <chrono>
#include <utility>

#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/gen/client_cpp.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/thrift/gen-cpp2/RocketUpgradeAsyncClient.h>

THRIFT_FLAG_DEFINE_bool(raw_client_rocket_upgrade_enabled, false);
THRIFT_FLAG_DEFINE_int64(raw_client_rocket_upgrade_timeout_ms, 100);

using folly::IOBuf;
using folly::IOBufQueue;
using std::make_unique;
using std::pair;
using std::unique_ptr;
using namespace std::chrono_literals;
using namespace apache::thrift::transport;
using folly::EventBase;
using folly::RequestContext;
using HResClock = std::chrono::high_resolution_clock;
using Us = std::chrono::microseconds;

namespace apache {
namespace thrift {
namespace {
class ReleasableDestructor : public folly::DelayedDestruction::Destructor {
 public:
  void operator()(folly::DelayedDestruction* dd) const {
    if (!released_) {
      dd->destroy();
    }
  }

  /**
   * Release the object managed by smart pointers. This is used when the
   * object ownership is transferred to another smart pointer or manually
   * managed by the caller. The original object must be properly deleted at
   * the end of its life cycle to avoid resource leaks.
   */
  void release() { released_ = true; }

 private:
  bool released_{false};
};

std::unique_ptr<folly::AsyncTransport, ReleasableDestructor> toReleasable(
    folly::AsyncTransport::UniquePtr transport) {
  return std::unique_ptr<folly::AsyncTransport, ReleasableDestructor>(
      transport.release());
}
} // namespace

template class ChannelCallbacks::TwowayCallback<HeaderClientChannel>;

HeaderClientChannel::HeaderClientChannel(
    folly::AsyncTransport::UniquePtr transport)
    : HeaderClientChannel(std::shared_ptr<Cpp2Channel>(Cpp2Channel::newChannel(
          toReleasable(std::move(transport)),
          make_unique<ClientFramingHandler>(*this)))) {
  upgradeToRocket_ = THRIFT_FLAG(raw_client_rocket_upgrade_enabled);
}

HeaderClientChannel::HeaderClientChannel(
    WithRocketUpgrade rocketUpgrade, folly::AsyncTransport::UniquePtr transport)
    : HeaderClientChannel(std::shared_ptr<Cpp2Channel>(Cpp2Channel::newChannel(
          toReleasable(std::move(transport)),
          make_unique<ClientFramingHandler>(*this)))) {
  upgradeToRocket_ = rocketUpgrade.enabled;
}

HeaderClientChannel::HeaderClientChannel(
    std::shared_ptr<Cpp2Channel> cpp2Channel)
    : sendSeqId_(0),
      closeCallback_(nullptr),
      timeout_(0),
      cpp2Channel_(cpp2Channel),
      protocolId_(apache::thrift::protocol::T_COMPACT_PROTOCOL),
      upgradeToRocket_(false),
      upgradeState_(RocketUpgradeState::INIT) {}

void HeaderClientChannel::setTimeout(uint32_t ms) {
  if (isUpgradedToRocket()) {
    rocketChannel_->setTimeout(ms);
  } else {
    getTransport()->setSendTimeout(ms);
    timeout_ = ms;
  }
}

void HeaderClientChannel::closeNow() {
  if (isUpgradedToRocket()) {
    rocketChannel_->closeNow();
  } else {
    cpp2Channel_->closeNow();
  }
}

void HeaderClientChannel::destroy() {
  closeNow();
  folly::DelayedDestruction::destroy();
}

void HeaderClientChannel::useAsHttpClient(
    const std::string& host, const std::string& uri) {
  setClientType(THRIFT_HTTP_CLIENT_TYPE);
  httpClientParser_ = std::make_shared<util::THttpClientParser>(host, uri);
  // Do not attempt transport upgrade to rocket if the channel is used as http
  // channel
  upgradeToRocket_ = false;
  upgradeState_ = RocketUpgradeState::NO_UPGRADE;
}

bool HeaderClientChannel::good() {
  auto transport = getTransport();
  return transport && transport->good();
}

void HeaderClientChannel::attachEventBase(EventBase* eventBase) {
  if (isUpgradedToRocket()) {
    rocketChannel_->attachEventBase(eventBase);
  } else {
    cpp2Channel_->attachEventBase(eventBase);
  }
}

void HeaderClientChannel::detachEventBase() {
  if (isUpgradedToRocket()) {
    rocketChannel_->detachEventBase();
  } else {
    cpp2Channel_->detachEventBase();
  }
}

bool HeaderClientChannel::isDetachable() {
  if (isUpgradedToRocket()) {
    return rocketChannel_->isDetachable();
  }
  return getTransport()->isDetachable() && recvCallbacks_.empty();
}

bool HeaderClientChannel::clientSupportHeader() {
  return getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
      getClientType() == THRIFT_HTTP_CLIENT_TYPE;
}

class HeaderClientChannel::RocketUpgradeCallback
    : public apache::thrift::RequestCallback {
 public:
  explicit RocketUpgradeCallback(
      apache::thrift::HeaderClientChannel* headerClientChannel)
      : headerClientChannel_(headerClientChannel) {}

  void requestSent() override {}

  void replyReceived(apache::thrift::ClientReceiveState&& state) override {
    auto ew = RocketUpgradeAsyncClient::recv_wrapped_upgradeToRocket(state);

    if (ew) {
      VLOG(4) << "Unable to upgrade transport from header to rocket! "
              << "Exception : " << folly::exceptionStr(ew);
    } else {
      // upgrade
      auto transportShared =
          headerClientChannel_->cpp2Channel_->getTransportShared();

      auto deleter = std::get_deleter<ReleasableDestructor>(transportShared);
      if (!deleter) {
        LOG(DFATAL) << "Rocket upgrade cannot complete. "
                    << "Underlying socket not using the special deleter.";
        return;
      }

      headerClientChannel_->cpp2Channel_->setTransport(nullptr);
      headerClientChannel_->cpp2Channel_->closeNow();
      // Note here we have one instance of the
      // std::shared_ptr<folly::AsyncTransport> in transportShared, and another
      // one in cpp2Channel_->pipeline_. Calling closeNow() on cpp2Channel_
      // inside a callback does not immediately close the OutboundLink of the
      // pipeline.
      assert(transportShared.use_count() == 2);
      // header channel give up ownership of the socket so that rocket
      // channel can own the socket from here onwards
      deleter->release();

      using apache::thrift::RocketClientChannel;
      auto rocketTransport =
          folly::AsyncTransport::UniquePtr(transportShared.get());
      headerClientChannel_->rocketChannel_ =
          headerClientChannel_->rocketRequestSetupMetadata_ != nullptr
          ? RocketClientChannel::newChannelWithMetadata(
                std::move(rocketTransport),
                std::move(*headerClientChannel_->rocketRequestSetupMetadata_))
          : RocketClientChannel::newChannel(std::move(rocketTransport));
      copyConfigurationToRocketChannel(
          *headerClientChannel_->rocketChannel_, *headerClientChannel_);
    }

    auto oldState = headerClientChannel_->upgradeState_.exchange(
        RocketUpgradeState::DONE, std::memory_order_acq_rel);
    CHECK_EQ(int(oldState), int(RocketUpgradeState::IN_PROGRESS));

    drainPendingRequests();
  }

  void requestError(apache::thrift::ClientReceiveState&& state) override {
    VLOG(4) << "Transport upgrade from header to rocket failed! "
            << "Exception : " << folly::exceptionStr(state.exception());

    auto oldState = headerClientChannel_->upgradeState_.exchange(
        RocketUpgradeState::DONE, std::memory_order_acq_rel);
    CHECK_EQ(int(oldState), int(RocketUpgradeState::IN_PROGRESS));

    drainPendingRequests();
  }

  bool isInlineSafe() const override { return true; }

 private:
  void drainPendingRequests() {
    while (!headerClientChannel_->pendingRequests_.empty()) {
      auto& req = headerClientChannel_->pendingRequests_.front();

      if (req.oneWay_) {
        headerClientChannel_->sendRequestNoResponse(
            req.rpcOptions_,
            std::move(req.methodName_),
            std::move(req.serializedRequest_),
            std::move(req.header_),
            std::move(req.callback_));
      } else {
        headerClientChannel_->sendRequestResponse(
            req.rpcOptions_,
            std::move(req.methodName_),
            std::move(req.serializedRequest_),
            std::move(req.header_),
            std::move(req.callback_));
      }
      headerClientChannel_->pendingRequests_.pop_front();
    }
  }

  static void copyConfigurationToRocketChannel(
      RocketClientChannel& rocketChannel,
      const HeaderClientChannel& headerChannel) {
    if (headerChannel.closeCallback_) {
      rocketChannel.setCloseCallback(headerChannel.closeCallback_);
    }
    rocketChannel.setProtocolId(headerChannel.protocolId_);
  }

  apache::thrift::HeaderClientChannel* headerClientChannel_;
};

void HeaderClientChannel::tryUpgradeTransportToRocket(
    std::chrono::milliseconds timeout) {
  auto state = upgradeState_.exchange(
      RocketUpgradeState::IN_PROGRESS, std::memory_order_acq_rel);
  CHECK_EQ(int(state), int(RocketUpgradeState::INIT));

  apache::thrift::RpcOptions rpcOptions;
  if (timeout <= 0ms) {
    timeout = std::chrono::milliseconds(timeout_) > 0ms
        ? std::chrono::milliseconds(timeout_)
        : std::chrono::milliseconds(
              THRIFT_FLAG(raw_client_rocket_upgrade_timeout_ms));
  }
  rpcOptions.setTimeout(timeout);

  auto callback = std::make_unique<RocketUpgradeCallback>(this);

  auto client = std::make_unique<apache::thrift::RocketUpgradeAsyncClient>(
      std::shared_ptr<HeaderClientChannel>(this, [](HeaderClientChannel*) {}));
  client->upgradeToRocket(rpcOptions, std::move(callback));
}

// Client Interface
void HeaderClientChannel::sendRequestNoResponse(
    const RpcOptions& rpcOptions,
    ManagedStringView&& methodName,
    SerializedRequest&& serializedRequest,
    std::shared_ptr<THeader> header,
    RequestClientCallback::Ptr cb) {
  preprocessHeader(header.get());
  // For raw thrift client only: before sending first request, check if we need
  // to upgrade transport to rocket
  switch (upgradeState_.load(std::memory_order_relaxed)) {
    case RocketUpgradeState::INIT:
      if (std::exchange(upgradeToRocket_, false)) {
        pendingRequests_.emplace_back(HeaderRequestContext(
            rpcOptions,
            std::move(methodName),
            std::move(serializedRequest),
            std::move(header),
            std::move(cb),
            true /* oneWay */));
        tryUpgradeTransportToRocket(rpcOptions.getTimeout());
        return;
      }
      break;
    case RocketUpgradeState::IN_PROGRESS:
      pendingRequests_.emplace_back(HeaderRequestContext(
          rpcOptions,
          std::move(methodName),
          std::move(serializedRequest),
          std::move(header),
          std::move(cb),
          true /* oneWay */));
      return;
    case RocketUpgradeState::DONE:
    case RocketUpgradeState::NO_UPGRADE:
      break;
  }
  if (rocketChannel_) {
    rocketChannel_->sendRequestNoResponse(
        rpcOptions,
        std::move(methodName),
        std::move(serializedRequest),
        std::move(header),
        std::move(cb));
  } else {
    auto buf = LegacySerializedRequest(
                   header->getProtocolId(),
                   methodName.view(),
                   std::move(serializedRequest))
                   .buffer;

    setRequestHeaderOptions(header.get(), buf->computeChainDataLength());
    addRpcOptionHeaders(header.get(), rpcOptions);
    attachMetadataOnce(header.get());

    // Both cb and buf are allowed to be null.
    uint32_t oldSeqId = sendSeqId_;
    sendSeqId_ = ResponseChannel::ONEWAY_REQUEST_ID;

    if (cb) {
      sendMessage(
          new OnewayCallback(std::move(cb)), std::move(buf), header.get());
    } else {
      sendMessage(nullptr, std::move(buf), header.get());
    }
    sendSeqId_ = oldSeqId;
  }
}

void HeaderClientChannel::setCloseCallback(CloseCallback* cb) {
  if (isUpgradedToRocket()) {
    rocketChannel_->setCloseCallback(cb);
  } else {
    closeCallback_ = cb;
    setBaseReceivedCallback();
  }
}

void HeaderClientChannel::setRequestHeaderOptions(
    THeader* header, ssize_t payloadSize) {
  header->setFlags(HEADER_FLAG_SUPPORT_OUT_OF_ORDER);
  header->setClientType(getClientType());
  header->forceClientType(true);
  if (auto compressionConfig = header->getDesiredCompressionConfig()) {
    if (auto codecRef = compressionConfig->codecConfig_ref()) {
      if (payloadSize >
          compressionConfig->compressionSizeLimit_ref().value_or(0)) {
        switch (codecRef->getType()) {
          case CodecConfig::zlibConfig:
            header->setTransform(THeader::ZLIB_TRANSFORM);
            break;
          case CodecConfig::zstdConfig:
            header->setTransform(THeader::ZSTD_TRANSFORM);
            break;
          default:
            break;
        }
      }
    }
  }
  if (getClientType() == THRIFT_HTTP_CLIENT_TYPE) {
    header->setHttpClientParser(httpClientParser_);
  }
}

void HeaderClientChannel::setConnectionAgentName(std::string_view name) {
  agentName_ = name;
}

void HeaderClientChannel::attachMetadataOnce(THeader* header) {
  if (std::exchange(firstRequest_, false)) {
    ClientMetadata md;
    if (const auto& hostMetadata = ClientChannel::getHostMetadata()) {
      md.hostname_ref().from_optional(hostMetadata->hostname);
      md.otherMetadata_ref().from_optional(hostMetadata->otherMetadata);
    }
    if (!agentName_.empty()) {
      md.agent_ref() = std::move(agentName_);
    }
    header->setClientMetadata(md);
  }
}

uint16_t HeaderClientChannel::getProtocolId() {
  if (isUpgradedToRocket()) {
    return rocketChannel_->getProtocolId();
  }
  if (getClientType() == THRIFT_HEADER_CLIENT_TYPE ||
      getClientType() == THRIFT_HTTP_CLIENT_TYPE) {
    return protocolId_;
  } else if (getClientType() == THRIFT_FRAMED_COMPACT) {
    return T_COMPACT_PROTOCOL;
  } else {
    return T_BINARY_PROTOCOL; // Assume other transports use TBinary
  }
}

void HeaderClientChannel::sendRequestResponse(
    const RpcOptions& rpcOptions,
    ManagedStringView&& methodName,
    SerializedRequest&& serializedRequest,
    std::shared_ptr<THeader> header,
    RequestClientCallback::Ptr cb) {
  preprocessHeader(header.get());
  // Raw header client might go through a transport upgrade process.
  // upgradeState_ ensures that the requests that are coming during upgrade can
  // be properly handled
  switch (upgradeState_.load(std::memory_order_relaxed)) {
    case RocketUpgradeState::INIT:
      // before sending first request, check if we
      // need to upgrade transport to rocket
      if (std::exchange(upgradeToRocket_, false)) {
        pendingRequests_.emplace_back(HeaderRequestContext(
            rpcOptions,
            std::move(methodName),
            std::move(serializedRequest),
            std::move(header),
            std::move(cb),
            false /* oneWay */));
        tryUpgradeTransportToRocket(rpcOptions.getTimeout());
        return;
      }
      break;
    case RocketUpgradeState::IN_PROGRESS:
      if (methodName.view() != "upgradeToRocket") {
        pendingRequests_.emplace_back(HeaderRequestContext(
            rpcOptions,
            std::move(methodName),
            std::move(serializedRequest),
            std::move(header),
            std::move(cb),
            false /* oneWay */));
        return;
      }
      break;
    case RocketUpgradeState::DONE:
    case RocketUpgradeState::NO_UPGRADE:
      break;
  }
  if (rocketChannel_) {
    rocketChannel_->sendRequestResponse(
        rpcOptions,
        std::move(methodName),
        std::move(serializedRequest),
        std::move(header),
        std::move(cb));
  } else {
    auto buf = LegacySerializedRequest(
                   header->getProtocolId(),
                   methodName.view(),
                   std::move(serializedRequest))
                   .buffer;

    // cb is not allowed to be null.
    DCHECK(cb);

    DestructorGuard dg(this);

    // Oneway requests use a special sequence id.
    // Make sure this non-oneway request doesn't use
    // the oneway request ID.
    if (++sendSeqId_ == ResponseChannel::ONEWAY_REQUEST_ID) {
      ++sendSeqId_;
    }

    std::chrono::milliseconds timeout(timeout_);
    if (rpcOptions.getTimeout() > std::chrono::milliseconds(0)) {
      timeout = rpcOptions.getTimeout();
    }

    auto twcb = new TwowayCallback<HeaderClientChannel>(
        this, sendSeqId_, std::move(cb), &getEventBase()->timer(), timeout);

    setRequestHeaderOptions(header.get(), buf->computeChainDataLength());
    addRpcOptionHeaders(header.get(), rpcOptions);
    attachMetadataOnce(header.get());

    if (getClientType() != THRIFT_HEADER_CLIENT_TYPE) {
      recvCallbackOrder_.push_back(sendSeqId_);
    }
    recvCallbacks_[sendSeqId_] = twcb;
    try {
      setBaseReceivedCallback(); // Cpp2Channel->setReceiveCallback can throw
    } catch (const TTransportException& ex) {
      twcb->messageSendError(
          folly::exception_wrapper(std::current_exception(), ex));
      return;
    }

    sendMessage(twcb, std::move(buf), header.get());
  }
}

// Header framing
std::unique_ptr<folly::IOBuf>
HeaderClientChannel::ClientFramingHandler::addFrame(
    unique_ptr<IOBuf> buf, THeader* header) {
  header->setSequenceNumber(channel_.sendSeqId_);
  return header->addHeader(
      std::move(buf), channel_.getPersistentWriteHeaders());
}

std::tuple<std::unique_ptr<IOBuf>, size_t, std::unique_ptr<THeader>>
HeaderClientChannel::ClientFramingHandler::removeFrame(IOBufQueue* q) {
  std::unique_ptr<THeader> header(new THeader(THeader::ALLOW_BIG_FRAMES));
  if (!q || !q->front() || q->front()->empty()) {
    return make_tuple(std::unique_ptr<IOBuf>(), 0, nullptr);
  }

  size_t remaining = 0;
  std::unique_ptr<folly::IOBuf> buf =
      header->removeHeader(q, remaining, channel_.persistentReadHeaders_);
  if (!buf) {
    return make_tuple(std::unique_ptr<folly::IOBuf>(), remaining, nullptr);
  }
  HeaderChannelTrait::checkSupportedClient(header->getClientType());
  return make_tuple(std::move(buf), 0, std::move(header));
}

// Interface from MessageChannel::RecvCallback
void HeaderClientChannel::messageReceived(
    unique_ptr<IOBuf>&& buf, unique_ptr<THeader>&& header) {
  DestructorGuard dg(this);

  if (!buf) {
    return;
  }

  uint32_t recvSeqId;

  if (header->getClientType() != THRIFT_HEADER_CLIENT_TYPE) {
    if (header->getClientType() == THRIFT_HTTP_CLIENT_TYPE &&
        buf->computeChainDataLength() == 0) {
      // HTTP/1.x Servers must send a response, even for oneway requests.
      // Ignore these responses.
      return;
    }
    // Non-header clients will always be in order.
    // Note that for non-header clients, getSequenceNumber()
    // will return garbage.
    recvSeqId = recvCallbackOrder_.front();
    recvCallbackOrder_.pop_front();
  } else {
    // The header contains the seq-id.  May be out of order.
    recvSeqId = header->getSequenceNumber();
  }

  auto cb = recvCallbacks_.find(recvSeqId);

  // TODO: On some errors, some servers will return 0 for seqid.
  // Could possibly try and deserialize the buf and throw a
  // TApplicationException.
  // BUT, we don't even know for sure what protocol to deserialize with.
  if (cb == recvCallbacks_.end()) {
    VLOG(5) << "Could not find message id in recvCallbacks "
            << "(timed out, possibly server is just now sending response?)";
    return;
  }

  auto f(cb->second);

  recvCallbacks_.erase(recvSeqId);
  // we are the last callback?
  setBaseReceivedCallback();
  f->replyReceived(std::move(buf), std::move(header));
}

void HeaderClientChannel::messageChannelEOF() {
  DestructorGuard dg(this);
  messageReceiveErrorWrapped(folly::make_exception_wrapper<TTransportException>(
      TTransportException::TTransportExceptionType::END_OF_FILE,
      "Channel got EOF. Check for server hitting connection limit, "
      "server connection idle timeout, and server crashes."));
  if (closeCallback_) {
    closeCallback_->channelClosed();
    closeCallback_ = nullptr;
  }
  setBaseReceivedCallback();
}

void HeaderClientChannel::messageReceiveErrorWrapped(
    folly::exception_wrapper&& ex) {
  DestructorGuard dg(this);

  while (!recvCallbacks_.empty()) {
    auto cb = recvCallbacks_.begin()->second;
    recvCallbacks_.erase(recvCallbacks_.begin());
    DestructorGuard dgcb(cb);
    cb->requestError(ex);
  }

  setBaseReceivedCallback();
}

void HeaderClientChannel::eraseCallback(
    uint32_t seqId, TwowayCallback<HeaderClientChannel>* cb) {
  CHECK(getEventBase()->isInEventBaseThread());
  auto it = recvCallbacks_.find(seqId);
  CHECK(it != recvCallbacks_.end());
  CHECK(it->second == cb);
  recvCallbacks_.erase(it);

  setBaseReceivedCallback(); // was this the last callback?
}

void HeaderClientChannel::setBaseReceivedCallback() {
  if (recvCallbacks_.size() != 0 || closeCallback_) {
    cpp2Channel_->setReceiveCallback(this);
  } else {
    cpp2Channel_->setReceiveCallback(nullptr);
  }
}

folly::AsyncTransport::UniquePtr HeaderClientChannel::stealTransport() {
  auto transportShared = cpp2Channel_->getTransportShared();
  cpp2Channel_->setTransport(nullptr);
  cpp2Channel_->closeNow();
  assert(transportShared.use_count() == 1);
  auto deleter = std::get_deleter<ReleasableDestructor>(transportShared);
  deleter->release();
  return folly::AsyncTransport::UniquePtr(transportShared.get());
}

void HeaderClientChannel::preprocessHeader(
    apache::thrift::transport::THeader* header) {
  if (compressionConfig_ && !header->getDesiredCompressionConfig()) {
    header->setDesiredCompressionConfig(*compressionConfig_);
  }
}

} // namespace thrift
} // namespace apache
