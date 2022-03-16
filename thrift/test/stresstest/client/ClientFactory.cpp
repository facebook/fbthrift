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

#include <thrift/test/stresstest/client/ClientFactory.h>

#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace apache {
namespace thrift {
namespace stress {

namespace {

folly::AsyncTransport::UniquePtr createSocket(
    const folly::SocketAddress& addr, folly::EventBase* evb) {
  return folly::AsyncTransport::UniquePtr(new folly::AsyncSocket(evb, addr));
}

ClientChannel::Ptr createChannel(folly::AsyncTransport::UniquePtr sock) {
  return RocketClientChannel::newChannel(std::move(sock));
}

} // namespace

std::unique_ptr<StressTestAsyncClient> ClientFactory::createClient(
    const folly::SocketAddress& addr, folly::EventBase* evb) {
  auto sock = createSocket(addr, evb);
  auto chan = createChannel(std::move(sock));
  return std::make_unique<StressTestAsyncClient>(std::move(chan));
}

} // namespace stress
} // namespace thrift
} // namespace apache
