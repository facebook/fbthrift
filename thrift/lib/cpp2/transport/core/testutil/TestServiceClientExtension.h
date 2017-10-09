/*
 * Copyright 2004-present Facebook, Inc.
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

#include <folly/Synchronized.h>
#include <yarpl/Flowable.h>
#include <vector>

#include <thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h>
#include "thrift/lib/cpp2/transport/core/ThriftClient.h"

// Extension functions, classes for client side to enable calling 'helloChannel'
// function from the client side.

namespace testutil {
namespace testservice {

class TestServiceAsyncClientExtended : public TestServiceAsyncClient {
 public:
  explicit TestServiceAsyncClientExtended(
      std::shared_ptr<apache::thrift::ThriftClient> channel)
      : TestServiceAsyncClient(channel), client_(channel) {
    evb_ = client_->getEventBase();
  }

  // Say hello to the given names
  virtual yarpl::Reference<yarpl::flowable::Flowable<std::string>> helloChannel(
      yarpl::Reference<yarpl::flowable::Flowable<std::string>> input);
  virtual yarpl::Reference<yarpl::flowable::Flowable<std::string>> helloChannel(
      apache::thrift::RpcOptions rpcOptions,
      yarpl::Reference<yarpl::flowable::Flowable<std::string>> input);

  // HelloChannel
 private:
  virtual void helloChannel(
      std::unique_ptr<apache::thrift::RequestCallback> callback);
  virtual void helloChannel(
      apache::thrift::RpcOptions& rpcOptions,
      std::unique_ptr<apache::thrift::RequestCallback> callback);

  virtual void helloChannelImpl(
      bool useSync,
      apache::thrift::RpcOptions& rpcOptions,
      std::unique_ptr<apache::thrift::RequestCallback> callback);

  virtual void helloChannel(
      folly::Function<void(::apache::thrift::ClientReceiveState&&)> callback);
  template <typename Protocol_>
  void helloChannelT(
      Protocol_* prot,
      bool useSync,
      apache::thrift::RpcOptions& rpcOptions,
      std::unique_ptr<apache::thrift::RequestCallback> callback);

 private:
  // Keep another reference in here instead of casting again and again,
  // we will already have one kind of RequestChannel soon.
  std::shared_ptr<apache::thrift::ThriftClient> client_;
  folly::EventBase* evb_;
};
} // namespace testservice
} // namespace testutil
