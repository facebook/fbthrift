/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/test/util/TestClientChannelFactory.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>

struct TestHTTPClientChannelFactory : public TestClientChannelFactory {
 public:
  TestHTTPClientChannelFactory() : TestClientChannelFactory() {}
  ~TestHTTPClientChannelFactory() override {}

  apache::thrift::ClientChannel::Ptr create(
      apache::thrift::async::TAsyncTransport::UniquePtr socket) override {
    auto channel = apache::thrift::HTTPClientChannel::newHTTP2Channel(
        std::move(socket));

    channel->setHTTPHost("localhost");
    channel->setHTTPUrl("/");
    channel->setProtocolId(protocol_);
    channel->setTimeout(timeout_);

    return std::move(channel);
  }
};
