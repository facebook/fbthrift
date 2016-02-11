/*
 * Copyright 2015 Facebook, Inc.
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

#include <proxygen/lib/http/codec/HTTP2Codec.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp2/test/util/TestClientChannelFactory.h>
#include <thrift/lib/cpp2/async/HTTPClientChannel.h>

struct TestHTTPClientChannelFactory : public TestClientChannelFactory {
 public:
  TestHTTPClientChannelFactory() : TestClientChannelFactory() {}
  ~TestHTTPClientChannelFactory() {}

  apache::thrift::ClientChannel::Ptr create(
      apache::thrift::async::TAsyncTransport::UniquePtr socket) {

    std::unique_ptr<proxygen::HTTPCodec> codec;
    switch (codec_) {
      case HTTP2:
        codec = folly::make_unique<proxygen::HTTP2Codec>(
            proxygen::TransportDirection::UPSTREAM);
    }

    auto channel = apache::thrift::HTTPClientChannel::newChannel(
        std::move(socket), "localhost", "/", std::move(codec));

    channel->setProtocolId(protocol_);
    channel->setTimeout(timeout_);

    return std::move(channel);
  }

  enum Codec {
    HTTP2,
  };

  TestHTTPClientChannelFactory& setCodec(Codec codec) {
    codec_ = codec;
    return *this;
  }

 protected:
  Codec codec_{HTTP2};
};
