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

#include <thrift/lib/cpp2/transport/core/ThriftClient.h>

#include <folly/Baton.h>
#include <glog/logging.h>
#include <proxygen/lib/utils/Base64.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>
#include <chrono>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::protocol::PROTOCOL_TYPES;
using apache::thrift::transport::THeader;
using folly::EventBase;
using folly::IOBuf;
using folly::RequestContext;

// TODO: Should we have a timeout for our replies?
static constexpr std::chrono::seconds kTimeOutForRPCCall{5};

namespace {
class WaitableRequestCallback final : public RequestCallback {
 public:
  WaitableRequestCallback(
      std::unique_ptr<RequestCallback> cb,
      folly::Baton<>& baton)
      : cb_(std::move(cb)), baton_(baton) {}

  void requestSent() override {
    cb_->requestSent();
  }

  void replyReceived(ClientReceiveState&& rs) override {
    cb_->replyReceived(std::move(rs));
    baton_.post();
  }

  void requestError(ClientReceiveState&& rs) override {
    assert(rs.isException());
    cb_->requestError(std::move(rs));
    baton_.post();
  }

 private:
  std::unique_ptr<RequestCallback> cb_;
  folly::Baton<>& baton_;
};
}

ThriftClient::ThriftClient(
    std::shared_ptr<ClientConnectionIf> connection,
    folly::EventBase* evb)
    : connection_(connection),
      clientEvb_(evb),
      connEvb_(connection->getEventBase()) {}

ThriftClient::ThriftClient(std::shared_ptr<ClientConnectionIf> connection)
    : ThriftClient(connection, connection->getEventBase()) {}

void ThriftClient::setProtocolId(uint16_t protocolId) {
  protocolId_ = protocolId;
}

uint32_t ThriftClient::sendRequestSync(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader> header) {
  folly::Baton<> baton;
  int result = 0;
  DCHECK(!connEvb_->inRunningEventBaseThread());

  // Execute on the IO thread for synchronous calls
  connEvb_->runInEventBaseThread([
    this,
    &baton,
    &result,
    options,
    cb = std::move(cb),
    ctx = std::move(ctx),
    buf = std::move(buf),
    header = std::move(header)
  ]() mutable {
    DCHECK(typeid(ClientSyncCallback) == typeid(*cb));
    bool oneway = static_cast<ClientSyncCallback&>(*cb).isOneway();
    auto scb = std::make_unique<WaitableRequestCallback>(std::move(cb), baton);
    if (oneway) {
      result = sendOnewayRequest(
          options,
          std::move(scb),
          std::move(ctx),
          std::move(buf),
          std::move(header));
    } else {
      result = sendRequest(
          options,
          std::move(scb),
          std::move(ctx),
          std::move(buf),
          std::move(header));
    }
  });
  CHECK(baton.timed_wait(kTimeOutForRPCCall));
  return result;
}

uint32_t ThriftClient::sendRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  return sendRequestHelper(
      rpcOptions,
      false,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
}

uint32_t ThriftClient::sendOnewayRequest(
    RpcOptions& rpcOptions,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  sendRequestHelper(
      rpcOptions,
      true,
      std::move(cb),
      std::move(ctx),
      std::move(buf),
      std::move(header));
  return ResponseChannel::ONEWAY_REQUEST_ID;
}

uint32_t ThriftClient::sendRequestHelper(
    RpcOptions& rpcOptions,
    bool oneway,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<IOBuf> buf,
    std::shared_ptr<THeader> header) {
  // TODO: Copied from HTTPClientChannel, but have not copied code to
  // handle timeouts and various error cases.  Will deal with that
  // as a followup change.

  DCHECK(
      clientEvb_->isInEventBaseThread() ||
      (clientEvb_ != connEvb_ && connEvb_->isInEventBaseThread()));
  DestructorGuard dg(this);

  std::shared_ptr<ThriftChannelIf> channel = connection_->getChannel();

  std::unique_ptr<FunctionInfo> finfo = std::make_unique<FunctionInfo>();
  finfo->name = ""; // TODO: add function name (we don't use it right now)
  if (oneway) {
    finfo->kind = SINGLE_REQUEST_NO_RESPONSE;
  } else {
    finfo->kind = SINGLE_REQUEST_SINGLE_RESPONSE;
  }
  finfo->seqId = 0; // not used.
  finfo->protocol = static_cast<PROTOCOL_TYPES>(protocolId_);

  // TODO: Details regarding building of headers to be dealt with later.
  // For now just copied from HTTPClientChannel.
  setRequestHeaderOptions(header.get());
  addRpcOptionHeaders(header.get(), rpcOptions);
  auto headerMap = buildHeaderMap(header.get());

  std::unique_ptr<ThriftClientCallback> callback;
  if (!oneway) {
    callback = std::make_unique<ThriftClientCallback>(
        clientEvb_,
        std::move(cb),
        std::move(ctx),
        isSecurityActive(),
        protocolId_);
  }
  channel->sendThriftRequest(
      std::move(finfo),
      std::move(headerMap),
      std::move(buf),
      std::move(callback));
  return 0;
}

// TODO: Not clear why this is required - for now just copied from
// HTTPClientChannel.
void ThriftClient::setRequestHeaderOptions(THeader* header) {
  header->setClientType(THRIFT_HTTP_CLIENT_TYPE);
  header->forceClientType(THRIFT_HTTP_CLIENT_TYPE);
}

// TODO: Details to be ironed out later - for now just copied from
// HTTPClientChannel.  Seems to be some Proxygen dependency here.
void ThriftClient::setHeaders(
    map<string, string>& dstHeaders,
    const transport::THeader::StringToStringMap& srcHeaders) {
  for (const auto& header : srcHeaders) {
    if (header.first.find(":") != std::string::npos) {
      auto name = proxygen::Base64::urlEncode(folly::StringPiece(header.first));
      auto value =
          proxygen::Base64::urlEncode(folly::StringPiece(header.second));
      dstHeaders[folly::to<std::string>("encode_", name)] =
          folly::to<std::string>(name, "_", value);
    } else {
      dstHeaders[header.first] = header.second;
    }
  }
}

// TODO: Details to be ironed out later - for now just copied from
// HTTPClientChannel.
std::unique_ptr<map<string, string>> ThriftClient::buildHeaderMap(
    THeader* header) {
  std::unique_ptr<map<string, string>> headers;

  {
    auto pwh = getPersistentWriteHeaders();
    setHeaders(*headers, pwh);
    // We do not clear the persistent write headers, since http does not
    // distinguish persistent/per request headers
    // pwh.clear();
  }

  {
    auto wh = header->releaseWriteHeaders();
    setHeaders(*headers, wh);
  }

  {
    auto eh = header->getExtraWriteHeaders();
    if (eh) {
      setHeaders(*headers, *eh);
    }
  }

  return headers;
}

EventBase* ThriftClient::getEventBase() const {
  return clientEvb_;
}

uint16_t ThriftClient::getProtocolId() {
  return protocolId_;
}

void ThriftClient::setCloseCallback(CloseCallback* /*cb*/) {
  // TBD - handle through cancel().
}

TAsyncTransport* ThriftClient::getTransport() {
  return connection_->getTransport();
}

bool ThriftClient::good() {
  return connection_->good();
}

ClientChannel::SaturationStatus ThriftClient::getSaturationStatus() {
  return connection_->getSaturationStatus();
}

void ThriftClient::attachEventBase(folly::EventBase* eventBase) {
  connection_->attachEventBase(eventBase);
}

void ThriftClient::detachEventBase() {
  connection_->detachEventBase();
}

bool ThriftClient::isDetachable() {
  return connection_->isDetachable();
}

bool ThriftClient::isSecurityActive() {
  return connection_->isSecurityActive();
}

uint32_t ThriftClient::getTimeout() {
  return connection_->getTimeout();
}

void ThriftClient::setTimeout(uint32_t ms) {
  return connection_->setTimeout(ms);
}

void ThriftClient::closeNow() {
  connection_->closeNow();
}

CLIENT_TYPE ThriftClient::getClientType() {
  return connection_->getClientType();
}

} // namespace thrift
} // namespace apache
