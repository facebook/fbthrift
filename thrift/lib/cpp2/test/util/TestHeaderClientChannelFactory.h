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

#pragma once

#include <folly/io/async/AsyncTransport.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/util/TestClientChannelFactory.h>

struct TestHeaderClientChannelFactory : public TestClientChannelFactory {
 public:
  TestHeaderClientChannelFactory() : TestClientChannelFactory() {}
  ~TestHeaderClientChannelFactory() override {}

  apache::thrift::ClientChannel::Ptr create(
      folly::AsyncTransport::UniquePtr socket) override {
    std::shared_ptr<folly::AsyncTransport> sharedSocket(std::move(socket));
    auto channel = apache::thrift::HeaderClientChannel::Ptr(
        new apache::thrift::HeaderClientChannel(sharedSocket));

    channel->setProtocolId(protocol_);
    channel->setTimeout(timeout_);

    return std::move(channel);
  }
};
