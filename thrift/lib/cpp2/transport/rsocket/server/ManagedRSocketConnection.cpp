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
#include <thrift/lib/cpp2/transport/rsocket/server/ManagedRSocketConnection.h>

#include <folly/ScopeGuard.h>
#include <rsocket/RSocketException.h>
#include <rsocket/framing/Frame.h>
#include <rsocket/framing/FrameProcessor.h>
#include <rsocket/framing/FrameSerializer.h>
#include <rsocket/framing/FramedDuplexConnection.h>
#include <rsocket/transports/tcp/TcpDuplexConnection.h>
#include <wangle/acceptor/ConnectionManager.h>

namespace apache {
namespace thrift {

using namespace rsocket;

class ManagedRSocketConnection::SetupSubscriber
    : public DuplexConnection::Subscriber,
      public std::enable_shared_from_this<SetupSubscriber> {
 public:
  SetupSubscriber(
      ManagedRSocketConnection& managed,
      folly::AsyncTransportWrapper::UniquePtr sock,
      OnNewSetupFn setupFunc)
      : managed_(managed), setupFunc_(std::move(setupFunc)) {
    auto one = std::make_unique<TcpDuplexConnection>(std::move(sock));

    duplexConnection_ = std::make_unique<FramedDuplexConnection>(
        std::move(one), ProtocolVersion::Unknown);
  }

  void setInput() {
    duplexConnection_->setInput(shared_from_this());
  }

  void onSubscribe(
      std::shared_ptr<yarpl::flowable::Subscription> subscription) override {
    subscription_ = std::move(subscription);
    subscription_->request(std::numeric_limits<int64_t>::max());
  }

  void onComplete() override {
    closed_ = true;
    duplexConnection_.reset();
  }

  void onError(folly::exception_wrapper) override {
    closed_ = true;
    duplexConnection_.reset();
  }

  void onNext(std::unique_ptr<folly::IOBuf> buf) override {
    if (closed_) {
      return;
    }

    DCHECK(duplexConnection_) << "Received more than one frame";

    auto connection = std::move(duplexConnection_);
    subscription_->cancel();

    const auto serializer = FrameSerializer::createAutodetectedSerializer(*buf);
    if (!serializer) {
      VLOG(1) << "Unable to detect protocol version";
      managed_.removeConnection();
      return;
    }

    auto onErr = [&](Frame_ERROR&& error) {
      auto err = serializer->serializeOut(std::move(error));
      connection->send(std::move(err));
      managed_.removeConnection();
    };

    if (serializer->peekFrameType(*buf) != FrameType::SETUP) {
      constexpr auto msg = "Invalid frame, expected SETUP/RESUME";
      onErr(Frame_ERROR::connectionError(msg));
      return;
    }

    Frame_SETUP frame;
    if (!serializer->deserializeFrom(frame, std::move(buf))) {
      constexpr auto msg = "Cannot decode SETUP frame";
      onErr(Frame_ERROR::connectionError(msg));
      return;
    }

    VLOG(3) << "In: " << frame;

    SetupParameters params;
    frame.moveToSetupPayload(params);

    if (serializer->protocolVersion() != params.protocolVersion) {
      constexpr auto msg = "SETUP frame has invalid protocol version";
      onErr(Frame_ERROR::invalidSetup(msg));
      return;
    }

    std::shared_ptr<RSResponder> responder;
    try {
      responder = setupFunc_(params);
      DCHECK_NOTNULL(responder.get());
    } catch (const std::exception& ex) {
      onErr(Frame_ERROR::rejectedSetup(ex.what()));
      return;
    }

    managed_.onSetup(
        std::move(connection), std::move(responder), std::move(params));
  }

 private:
  ManagedRSocketConnection& managed_;
  OnNewSetupFn setupFunc_;
  std::unique_ptr<rsocket::FramedDuplexConnection> duplexConnection_;
  std::shared_ptr<yarpl::flowable::Subscription> subscription_;
  bool closed_{false};
};

ManagedRSocketConnection::ManagedRSocketConnection(
    folly::AsyncTransportWrapper::UniquePtr sock,
    OnNewSetupFn setupFunc) {
  auto setupSubscriber = std::make_shared<SetupSubscriber>(
      *this, std::move(sock), std::move(setupFunc));
  setupSubscriber_ =
      std::dynamic_pointer_cast<DuplexConnection::Subscriber>(setupSubscriber);
  setupSubscriber->setInput();
}

void ManagedRSocketConnection::removeConnection() {
  stop(folly::make_exception_wrapper<transport::TTransportException>(
      transport::TTransportException::TTransportExceptionType::INTERRUPTED,
      "remove connection"));
}

void ManagedRSocketConnection::onSetup(
    std::unique_ptr<DuplexConnection> connection,
    std::shared_ptr<RSResponder> responder,
    SetupParameters setupParams) {
  auto subscriber = std::move(setupSubscriber_);

  stateMachine_ = std::make_shared<RSocketStateMachine>(
      std::move(responder),
      nullptr,
      RSocketMode::SERVER,
      RSocketStats::noop(),
      nullptr, /* connectionEvents */
      nullptr, /* resumeManager */
      nullptr /* coldResumeHandler */);

  stateMachine_->registerCloseCallback(this);

  stateMachine_->connectServer(
      std::make_shared<FrameTransportImpl>(std::move(connection)),
      std::move(setupParams));
}

void ManagedRSocketConnection::stop(folly::exception_wrapper ew) {
  if (auto subscriber = std::exchange(setupSubscriber_, nullptr)) {
    setupSubscriber_->onError(ew);
  }
  if (auto stateMachine = std::exchange(stateMachine_, nullptr)) {
    stateMachine->registerCloseCallback(nullptr);
    stateMachine->close(
        std::move(ew), rsocket::StreamCompletionSignal::CONNECTION_END);
  }
  if (auto manager = getConnectionManager()) {
    manager->removeConnection(this);
  }

  destroy();
}

void ManagedRSocketConnection::closeWhenIdle() {
  stop(folly::make_exception_wrapper<transport::TTransportException>(
      transport::TTransportException::TTransportExceptionType::INTERRUPTED,
      "close when idle"));
}

void ManagedRSocketConnection::dropConnection() {
  stop(folly::make_exception_wrapper<transport::TTransportException>(
      transport::TTransportException::TTransportExceptionType::INTERRUPTED,
      "drop connection"));
}

void ManagedRSocketConnection::timeoutExpired() noexcept {
  // Only disconnect if there are no active requests. No need to set another
  // timeout here because it's going to be set when all the requests are
  // handled.
  if (!stateMachine_->hasStreams()) {
    stop(folly::make_exception_wrapper<transport::TTransportException>(
        transport::TTransportException::TTransportExceptionType::TIMED_OUT,
        "idle timeout"));
  }
}

bool ManagedRSocketConnection::isBusy() const {
  return stateMachine_->hasStreams();
}

} // namespace thrift
} // namespace apache
