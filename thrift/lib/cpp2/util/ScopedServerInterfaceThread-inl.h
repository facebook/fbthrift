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

#include <folly/executors/GlobalExecutor.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>

namespace apache {
namespace thrift {
namespace detail {
class FaultInjectionChannel : public RequestChannel {
 public:
  FaultInjectionChannel(
      RequestChannel::Ptr client,
      ScopedServerInterfaceThread::FaultInjectionFunc injectFault)
      : client_(std::move(client)), injectFault_(std::move(injectFault)) {}

  void sendRequestResponse(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& req,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr clientCallback) override {
    if (auto ex = injectFault_(methodName.view())) {
      clientCallback.release()->onResponseError(std::move(ex));
      return;
    }
    client_->sendRequestResponse(
        rpcOptions,
        std::move(methodName),
        std::move(req),
        std::move(header),
        std::move(clientCallback));
  }
  void sendRequestNoResponse(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& req,
      std::shared_ptr<transport::THeader> header,
      RequestClientCallback::Ptr clientCallback) override {
    if (auto ex = injectFault_(methodName.view())) {
      clientCallback.release()->onResponseError(std::move(ex));
      return;
    }
    client_->sendRequestNoResponse(
        rpcOptions,
        std::move(methodName),
        std::move(req),
        std::move(header),
        std::move(clientCallback));
  }
  void sendRequestStream(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& req,
      std::shared_ptr<transport::THeader> header,
      StreamClientCallback* clientCallback) override {
    if (auto ex = injectFault_(methodName.view())) {
      clientCallback->onFirstResponseError(std::move(ex));
      return;
    }
    client_->sendRequestStream(
        rpcOptions,
        std::move(methodName),
        std::move(req),
        std::move(header),
        clientCallback);
  }
  void sendRequestSink(
      const RpcOptions& rpcOptions,
      ManagedStringView&& methodName,
      SerializedRequest&& req,
      std::shared_ptr<transport::THeader> header,
      SinkClientCallback* clientCallback) override {
    if (auto ex = injectFault_(methodName.view())) {
      clientCallback->onFirstResponseError(std::move(ex));
      return;
    }
    client_->sendRequestSink(
        rpcOptions,
        std::move(methodName),
        std::move(req),
        std::move(header),
        clientCallback);
  }

  void setCloseCallback(CloseCallback* cb) override {
    client_->setCloseCallback(cb);
  }

  folly::EventBase* getEventBase() const override {
    return client_->getEventBase();
  }

  uint16_t getProtocolId() override {
    return client_->getProtocolId();
  }

 private:
  RequestChannel::Ptr client_;
  ScopedServerInterfaceThread::FaultInjectionFunc injectFault_;
};

template <class AsyncClientT>
struct TestClientRunner {
  ScopedServerInterfaceThread runner;
  std::unique_ptr<AsyncClientT> client;

  explicit TestClientRunner(std::shared_ptr<AsyncProcessorFactory> apf)
      : runner(std::move(apf)) {}
};
} // namespace detail

template <class AsyncClientT>
std::unique_ptr<AsyncClientT> ScopedServerInterfaceThread::newStickyClient(
    folly::Executor* callbackExecutor,
    ScopedServerInterfaceThread::MakeChannelFunc makeChannel) const {
  auto io = folly::getIOExecutor();
  auto sp = std::shared_ptr<folly::EventBase>(io, io->getEventBase());
  return std::make_unique<AsyncClientT>(PooledRequestChannel::newChannel(
      callbackExecutor,
      std::move(sp),
      [makeChannel = std::move(makeChannel),
       address = getAddress()](folly::EventBase& eb) mutable {
        return makeChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&eb, address)));
      }));
}

template <class AsyncClientT>
std::unique_ptr<AsyncClientT> ScopedServerInterfaceThread::newClient(
    folly::Executor* callbackExecutor,
    ScopedServerInterfaceThread::MakeChannelFunc makeChannel) const {
  return std::make_unique<AsyncClientT>(PooledRequestChannel::newChannel(
      callbackExecutor,
      folly::getIOExecutor(),
      [makeChannel = std::move(makeChannel),
       address = getAddress()](folly::EventBase& eb) mutable {
        return makeChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&eb, address)));
      }));
}

template <class AsyncClientT>
std::unique_ptr<AsyncClientT>
ScopedServerInterfaceThread::newClientWithFaultInjection(
    ScopedServerInterfaceThread::FaultInjectionFunc injectFault,
    folly::Executor* callbackExecutor,
    ScopedServerInterfaceThread::MakeChannelFunc makeChannel) const {
  return std::make_unique<AsyncClientT>(
      RequestChannel::Ptr(new apache::thrift::detail::FaultInjectionChannel(
          PooledRequestChannel::newChannel(
              callbackExecutor,
              folly::getIOExecutor(),
              [makeChannel = std::move(makeChannel),
               address = getAddress()](folly::EventBase& eb) mutable {
                return makeChannel(folly::AsyncSocket::UniquePtr(
                    new folly::AsyncSocket(&eb, address)));
              }),
          std::move(injectFault))));
}

template <class AsyncClientT>
std::shared_ptr<AsyncClientT> makeTestClient(
    std::shared_ptr<AsyncProcessorFactory> apf,
    ScopedServerInterfaceThread::FaultInjectionFunc injectFault) {
  auto runner =
      std::make_shared<detail::TestClientRunner<AsyncClientT>>(std::move(apf));
  runner->client = injectFault
      ? runner->runner.template newClientWithFaultInjection<AsyncClientT>(
            std::move(injectFault), nullptr, RocketClientChannel::newChannel)
      : runner->runner.template newClient<AsyncClientT>(
            nullptr, RocketClientChannel::newChannel);
  auto* client = runner->client.get();
  return {std::move(runner), client};
}

} // namespace thrift
} // namespace apache
