/*
 * Copyright 2014 Facebook, Inc.
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

namespace apache { namespace thrift {

template <typename Result>
class FutureCallbackBase : public RequestCallback {
  public:
    explicit FutureCallbackBase(folly::Promise<Result>&& promise)
          : promise_(std::move(promise)) {}

    void requestSent() override{};

    void requestError(ClientReceiveState&& state) override {
      CHECK(state.isException());
      promise_.setException(state.moveExceptionWrapper());
    }

  protected:
    folly::Promise<Result> promise_;
};

template <typename Result>
class FutureCallback : public FutureCallbackBase<Result> {
  private:
    typedef folly::exception_wrapper (*Processor)(Result&,ClientReceiveState&);

  public:
    FutureCallback(folly::Promise<Result>&& promise,
                   Processor processor)
          : FutureCallbackBase<Result>(std::move(promise)),
            processor_(processor) {}

    void replyReceived(ClientReceiveState&& state) {
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
  private:
    Processor processor_;
};

class OneWayFutureCallback : public FutureCallbackBase<void> {
  public:
    explicit OneWayFutureCallback(folly::Promise<void>&& promise)
        : FutureCallbackBase<void>(std::move(promise)) {}

    void requestSent() override {
      promise_.setValue();
    };

    void replyReceived(ClientReceiveState&& state) override {
      CHECK(false);
    }
};

template <>
class FutureCallback<void> : public FutureCallbackBase<void> {
  private:
    typedef folly::exception_wrapper (*Processor)(ClientReceiveState&);
  public:
    FutureCallback(folly::Promise<void>&& promise,
                   Processor processor)
        : FutureCallbackBase<void>(std::move(promise)),
          processor_(processor) {}

    void replyReceived(ClientReceiveState&& state) override {
      CHECK(!state.isException());
      CHECK(state.buf());

      auto ew = processor_(state);
      if (ew) {
        promise_.setException(ew);
      } else {
        promise_.setValue();
      }
    }

  private:
    Processor processor_;
};



}} // Namespace
