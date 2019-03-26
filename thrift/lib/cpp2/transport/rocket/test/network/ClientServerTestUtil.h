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

#pragma once

#include <chrono>
#include <future>
#include <memory>

#include <folly/Try.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

namespace folly {
class EventBase;
class IOBuf;
class SocketAddress;

namespace fibers {
class FiberManager;
} // namespace fibers
} // namespace folly

namespace rsocket {
class RSocketServer;
} // namespace rsocket

namespace wangle {
class Acceptor;
} // namespace wangle

namespace apache {
namespace thrift {
namespace rocket {

class RocketClient;
class RocketClientWriteCallback;

namespace test {

class RsocketTestServer {
 public:
  RsocketTestServer();
  ~RsocketTestServer();

  uint16_t getListeningPort() const;
  void shutdown();

 private:
  std::unique_ptr<rsocket::RSocketServer> rsocketServer_;
};

class RocketTestClient {
 public:
  explicit RocketTestClient(const folly::SocketAddress& serverAddr);
  ~RocketTestClient();

  folly::Try<Payload> sendRequestResponseSync(
      Payload request,
      std::chrono::milliseconds timeout = std::chrono::milliseconds(250),
      RocketClientWriteCallback* writeCallback = nullptr);

  folly::Try<void> sendRequestFnfSync(
      Payload request,
      RocketClientWriteCallback* writeCallback = nullptr);

  folly::Try<SemiStream<Payload>> sendRequestStreamSync(Payload request);

  rocket::SetupFrame makeTestSetupFrame();

  void reconnect();

 private:
  folly::ScopedEventBaseThread evbThread_;
  folly::EventBase& evb_;
  folly::fibers::FiberManager& fm_;
  std::shared_ptr<RocketClient> client_;
  const folly::SocketAddress serverAddr_;

  void connect();
  void disconnect();
};

class RocketTestServer {
 public:
  RocketTestServer();
  ~RocketTestServer();

  uint16_t getListeningPort() const;

 private:
  folly::ScopedEventBaseThread ioThread_;
  folly::EventBase& evb_;
  folly::AsyncServerSocket::UniquePtr listeningSocket_;
  std::unique_ptr<wangle::Acceptor> acceptor_;
  std::future<void> shutdownFuture_;

  void start();
  void stop();
};

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
