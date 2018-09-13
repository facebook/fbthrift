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

#include <thrift/lib/cpp2/transport/rocket/test/network/ClientServerTestUtil.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>

#include <folly/Conv.h>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <folly/fibers/FiberManager.h>
#include <folly/fibers/FiberManagerMap.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>

#include <rsocket/RSocket.h>
#include <rsocket/transports/tcp/TcpConnectionAcceptor.h>
#include <yarpl/Flowable.h>
#include <yarpl/Single.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RocketClient.h>

using namespace rsocket;
using namespace yarpl::flowable;
using namespace yarpl::single;

namespace apache {
namespace thrift {
namespace rocket {
namespace test {

namespace {
class RsocketTestServerResponder : public rsocket::RSocketResponder {
 public:
  std::shared_ptr<Single<rsocket::Payload>> handleRequestResponse(
      rsocket::Payload request,
      uint32_t) final;
};

std::shared_ptr<Single<rsocket::Payload>>
RsocketTestServerResponder::handleRequestResponse(
    rsocket::Payload request,
    uint32_t /* streamId */) {
  DCHECK(request.data);
  auto data = folly::StringPiece(request.data->coalesce());

  if (data.removePrefix("error:application")) {
    return Single<rsocket::Payload>::create([](auto&& subscriber) mutable {
      subscriber->onSubscribe(SingleSubscriptions::empty());
      subscriber->onError(folly::make_exception_wrapper<std::runtime_error>(
          "Application error occurred"));
    });
  }

  constexpr folly::StringPiece kMetadataEchoPrefix{"metadata_echo:"};
  constexpr folly::StringPiece kDataEchoPrefix{"data_echo:"};

  folly::Optional<rsocket::Payload> responsePayload;
  if (data.removePrefix("sleep_ms:")) {
    // Sleep, then echo back request.
    std::chrono::milliseconds sleepFor(folly::to<uint32_t>(data));
    std::this_thread::sleep_for(sleepFor); // sleep override
  } else if (data.removePrefix("error:")) {
    // Reply with a specific kind of error.
  } else if (data.startsWith(kMetadataEchoPrefix)) {
    // Reply with echoed metadata in the response payload.
    auto responseMetadata = request.data->clone();
    responseMetadata->trimStart(kMetadataEchoPrefix.size());
    responsePayload.emplace(
        rsocket::Payload(std::move(request.data), std::move(responseMetadata)));
  } else if (data.startsWith(kDataEchoPrefix)) {
    // Reply with echoed data in the response payload.
    auto responseData = request.data->clone();
    responseData->trimStart(kDataEchoPrefix.size());
    responsePayload.emplace(
        rsocket::Payload(std::move(responseData), std::move(request.metadata)));
  }

  // If response payload is not set at this point, simply echo back what client
  // sent.
  if (!responsePayload) {
    responsePayload.emplace(std::move(request));
  }

  return Single<rsocket::Payload>::create(
      [responsePayload =
           std::move(*responsePayload)](auto&& subscriber) mutable {
        subscriber->onSubscribe(SingleSubscriptions::empty());
        subscriber->onSuccess(std::move(responsePayload));
      });
}
} // namespace

RsocketTestServer::RsocketTestServer() {
  TcpConnectionAcceptor::Options opts;
  opts.address = folly::SocketAddress("::1", 0 /* bind to any port */);
  opts.threads = 2;

  rsocketServer_ = RSocket::createServer(
      std::make_unique<TcpConnectionAcceptor>(std::move(opts)));
  // Start accepting connections
  rsocketServer_->start([](const rsocket::SetupParameters&) {
    return std::make_shared<RsocketTestServerResponder>();
  });
}

RsocketTestServer::~RsocketTestServer() {
  shutdown();
}

uint16_t RsocketTestServer::getListeningPort() const {
  auto oport = rsocketServer_->listeningPort();
  DCHECK(oport);
  return *oport;
}

void RsocketTestServer::shutdown() {
  rsocketServer_.reset();
}

RocketTestClient::RocketTestClient(const folly::SocketAddress& serverAddr)
    : evb_(*evbThread_.getEventBase()),
      fm_(folly::fibers::getFiberManager(evb_)) {
  evb_.runInEventBaseThread([this, serverAddr] {
    folly::AsyncSocket::UniquePtr socket(
        new folly::AsyncSocket(&evb_, serverAddr));
    client_ = RocketClient::create(evb_, std::move(socket));
  });
}

RocketTestClient::~RocketTestClient() {
  evb_.runInEventBaseThreadAndWait([this] { client_.reset(); });
}

folly::Try<Payload> RocketTestClient::sendRequestResponseSync(
    Payload request,
    std::chrono::milliseconds timeout) {
  folly::Try<Payload> response;
  folly::fibers::Baton baton;

  evb_.runInEventBaseThread([&] {
    fm_.addTaskFinally(
        [&] {
          return client_->sendRequestResponseSync(std::move(request), timeout);
        },
        [&](folly::Try<Payload>&& r) {
          response = std::move(r);
          baton.post();
        });
  });

  baton.wait();
  return response;
}

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
