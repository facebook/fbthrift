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

#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>

#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>

namespace apache {
namespace thrift {

namespace {
class ChannelKeepAlive : public RequestClientCallback {
 public:
  ChannelKeepAlive(
      ReconnectingRequestChannel::ImplPtr impl,
      RequestClientCallback::Ptr cob,
      bool oneWay)
      : keepAlive_(std::move(impl)), cob_(std::move(cob)), oneWay_(oneWay) {}

  void onRequestSent() noexcept override {
    if (!oneWay_) {
      cob_->onRequestSent();
    } else {
      cob_.release()->onRequestSent();
      delete this;
    }
  }

  void onResponse(ClientReceiveState&& state) noexcept override {
    cob_.release()->onResponse(std::move(state));
    delete this;
  }

  void onResponseError(folly::exception_wrapper ex) noexcept override {
    cob_.release()->onResponseError(std::move(ex));
    delete this;
  }

 private:
  ReconnectingRequestChannel::ImplPtr keepAlive_;
  RequestClientCallback::Ptr cob_;
  const bool oneWay_;
};

class ChannelKeepAliveStream : public StreamClientCallback {
 public:
  ChannelKeepAliveStream(
      ReconnectingRequestChannel::ImplPtr impl,
      StreamClientCallback& clientCallback)
      : keepAlive_(std::move(impl)), clientCallback_(clientCallback) {}

  bool onFirstResponse(
      FirstResponsePayload&& firstResponsePayload,
      folly::EventBase* evb,
      StreamServerCallback* serverCallback) override {
    SCOPE_EXIT {
      delete this;
    };
    serverCallback->resetClientCallback(clientCallback_);
    return clientCallback_.onFirstResponse(
        std::move(firstResponsePayload), evb, serverCallback);
  }
  void onFirstResponseError(folly::exception_wrapper ew) override {
    SCOPE_EXIT {
      delete this;
    };
    return clientCallback_.onFirstResponseError(std::move(ew));
  }

  virtual bool onStreamNext(StreamPayload&&) override {
    std::terminate();
  }
  virtual void onStreamError(folly::exception_wrapper) override {
    std::terminate();
  }
  virtual void onStreamComplete() override {
    std::terminate();
  }
  void resetServerCallback(StreamServerCallback&) override {
    std::terminate();
  }

 private:
  ReconnectingRequestChannel::ImplPtr keepAlive_;
  StreamClientCallback& clientCallback_;
};
} // namespace

void ReconnectingRequestChannel::sendRequestResponse(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  reconnectIfNeeded();
  cob = RequestClientCallback::Ptr(
      new ChannelKeepAlive(impl_, std::move(cob), false));

  return impl_->sendRequestResponse(
      options,
      methodName,
      std::move(request),
      std::move(header),
      std::move(cob));
}

void ReconnectingRequestChannel::sendRequestNoResponse(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    RequestClientCallback::Ptr cob) {
  reconnectIfNeeded();
  cob = RequestClientCallback::Ptr(
      new ChannelKeepAlive(impl_, std::move(cob), true));

  return impl_->sendRequestNoResponse(
      options,
      methodName,
      std::move(request),
      std::move(header),
      std::move(cob));
}

void ReconnectingRequestChannel::sendRequestStream(
    const RpcOptions& options,
    folly::StringPiece methodName,
    SerializedRequest&& request,
    std::shared_ptr<transport::THeader> header,
    StreamClientCallback* cob) {
  reconnectIfNeeded();
  cob = new ChannelKeepAliveStream(impl_, *cob);

  return impl_->sendRequestStream(
      options, methodName, std::move(request), std::move(header), cob);
}

void ReconnectingRequestChannel::reconnectIfNeeded() {
  if (!impl_ || !impl_->good()) {
    impl_ = implCreator_(evb_);
  }
}

} // namespace thrift
} // namespace apache
