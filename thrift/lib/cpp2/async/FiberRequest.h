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

#include <folly/experimental/fibers/Promise.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache { namespace thrift {

/**
 * Heavily inspired by apache::thrift::FutureCallback.
 */

template <typename Result>
class FiberRequestCallbackBase : public apache::thrift::RequestCallback {
 public:
  explicit FiberRequestCallbackBase(folly::fibers::Promise<Result>&& promise)
      : promise_(std::move(promise)) {}

  void requestSent(){};

  void requestError(apache::thrift::ClientReceiveState&& state) {
    CHECK(state.isException());
    promise_.setException(state.moveExceptionWrapper());
  }

 protected:
  folly::fibers::Promise<Result> promise_;
};

template <typename Result>
class FiberRequestCallback : public FiberRequestCallbackBase<Result> {
 private:
  typedef folly::exception_wrapper (*Processor)(
      Result&, apache::thrift::ClientReceiveState&);

 public:
  FiberRequestCallback(folly::fibers::Promise<Result>&& promise,
                       Processor processor)
      : FiberRequestCallbackBase<Result>(std::move(promise)),
        processor_(processor) {}

  void replyReceived(apache::thrift::ClientReceiveState&& state) {
    CHECK(!state.isException());
    CHECK(state.buf());

    Result result;
    auto ew = processor_(result, state);
    if (ew) {
      this->promise_.setException(ew);
    } else {
      this->promise_.setValue(std::move(result));
    }
  }

  void requestError(apache::thrift::ClientReceiveState&& state) {
    CHECK(state.isException());
    CHECK(!state.buf());
    this->promise_.setException(state.moveExceptionWrapper());
  }

 private:
  Processor processor_;
};

template <>
class FiberRequestCallback<void> : public FiberRequestCallbackBase<void> {
 public:
  FiberRequestCallback(folly::fibers::Promise<void>&& promise, bool isOneWay)
      : FiberRequestCallbackBase<void>(std::move(promise)),
        isOneWay_(isOneWay) {}

  void requestSent() {
    if (isOneWay_) {
      promise_.setValue();
    }
  };

  void replyReceived(apache::thrift::ClientReceiveState&& state) {
    CHECK(!state.isException());
    CHECK(!isOneWay_);

    promise_.setValue();
  }

 private:
  bool isOneWay_;
};

}} // Namespace
