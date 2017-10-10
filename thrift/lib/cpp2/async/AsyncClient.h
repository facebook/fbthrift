/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp2/async/HeaderChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache {
namespace thrift {

class Cpp2ConnContext;

class GeneratedAsyncClient : public TClientBase {
 public:
  using channel_ptr =
      std::unique_ptr<RequestChannel, folly::DelayedDestruction::Destructor>;

  // TODO: make it possible to create a connection context from a thrift channel
  GeneratedAsyncClient(std::shared_ptr<RequestChannel> channel);

  ~GeneratedAsyncClient() override;

  virtual char const* getServiceName() const noexcept = 0;

  RequestChannel* getChannel() const noexcept {
    return channel_.get();
  }

  HeaderChannel* getHeaderChannel() const noexcept {
    return dynamic_cast<HeaderChannel*>(channel_.get());
  }

 protected:
  std::unique_ptr<Cpp2ConnContext> connectionContext_;
  std::shared_ptr<RequestChannel> channel_;
};

} // namespace thrift
} // namespace apache
