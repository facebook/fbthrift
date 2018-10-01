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
#include <memory>

#include <folly/Try.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>

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

namespace apache {
namespace thrift {
namespace rocket {

class RocketClient;

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
      std::chrono::milliseconds timeout = std::chrono::milliseconds(250));

  folly::Try<void> sendRequestFnfSync(Payload request);

  folly::Try<SemiStream<Payload>> sendRequestStreamSync(Payload request);

 private:
  folly::ScopedEventBaseThread evbThread_;
  folly::EventBase& evb_;
  folly::fibers::FiberManager& fm_;
  std::shared_ptr<RocketClient> client_;
};

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
