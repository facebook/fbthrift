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

#include <folly/futures/Future.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <yarpl/Flowable.h>

namespace apache {
namespace thrift {

/**
 * If an RPC function is a stream enabled one, then that function will
 * create an instance of StreamRequestCallback class instead of others.
 * ThriftClient will handle instances of this class.
 */
class StreamRequestCallback : public ClientSyncCallback {
 public:
  using SubscriberRef = std::shared_ptr<
      yarpl::flowable::Subscriber<std::unique_ptr<folly::IOBuf>>>;
  using FlowableRef = std::shared_ptr<
      yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>>;

  StreamRequestCallback(RpcKind kind)
      : ClientSyncCallback(
            &rs_,
            kind != RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE),
        kind_(kind) {}

  // Called from the compiler generated code
  void subscribeToOutput(SubscriberRef subscriber) {
    subscriber_ = std::move(subscriber);
  }

  void setInput(FlowableRef input) {
    inputFlowable_ = std::move(input);
  }

  // Called from the Channel
  void subscribeToInput(SubscriberRef subscriber) {
    inputFlowable_->subscribe(subscriber);
  }

  SubscriberRef getOutput() {
    return subscriber_;
  }

  void replyReceived(ClientReceiveState&& rs) override {
    ClientSyncCallback::replyReceived(std::move(rs));
    replyPromise_.setValue();
  }

  void requestError(ClientReceiveState&& rs) override {
    ClientSyncCallback::requestError(std::move(rs));
    replyPromise_.setValue();
  }

  folly::Future<folly::Unit> getReplyFuture() {
    // if the function type is no response, just return the empty Future
    // otherwise return a promise dependent future to wait.
    if (kind_ == RpcKind::STREAMING_REQUEST_SINGLE_RESPONSE) {
      return replyPromise_.getFuture();
    }
    return folly::unit;
  }

 public:
  // Even though it is a streaming request,
  // we have various kinds of streaming requests.
  RpcKind kind_;

 protected:
  ClientReceiveState rs_;

  // Write output to this subscriber
  SubscriberRef subscriber_;
  FlowableRef inputFlowable_;

  folly::Promise<folly::Unit> replyPromise_;
};
} // namespace thrift
} // namespace apache
