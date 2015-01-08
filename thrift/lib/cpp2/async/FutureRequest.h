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

#include <folly/wangle/futures/Future.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache { namespace thrift {

template <typename Result>
class FutureCallbackBase : public RequestCallback {
  public:
    explicit FutureCallbackBase(folly::wangle::Promise<Result>&& promise)
          : promise_(std::move(promise)) {}

    void requestSent() {};

    void requestError(ClientReceiveState&& state) {
      CHECK(state.isException());
      promise_.setException(state.moveExceptionWrapper());
    }

  protected:
    folly::wangle::Promise<Result> promise_;
};

template <typename Result, typename IsScalar = void>
class FutureCallback : public FutureCallbackBase<Result> {
  private:
    typedef void (*Processor)(Result&,ClientReceiveState&);

  public:
    FutureCallback(folly::wangle::Promise<Result>&& promise,
                   Processor processor)
          : FutureCallbackBase<Result>(std::move(promise)),
            processor_(processor) {}

    void replyReceived(ClientReceiveState&& state) {
      CHECK(!state.isException());
      CHECK(state.buf());

      this->promise_.fulfil([&]{
        Result result;
        processor_(result, state);
        return result;
      });
    }

    void requestError(ClientReceiveState&& state) {
      CHECK(state.isException());
      CHECK(!state.buf());
      this->promise_.setException(state.moveExceptionWrapper());
    }
  private:
    Processor processor_;
};

template <typename Result>
class FutureCallback<Result,
              typename std::enable_if<std::is_scalar<Result>::value>::type>
      : public FutureCallbackBase<Result> {
  private:
    typedef Result (*Processor)(ClientReceiveState&);

  public:
    FutureCallback(folly::wangle::Promise<Result>&& promise,
                   Processor processor)
          : FutureCallbackBase<Result>(std::move(promise)),
            processor_(processor) {}

    void replyReceived(ClientReceiveState&& state) {
      CHECK(!state.isException());
      CHECK(state.buf());

      this->promise_.fulfil([&]{
        return processor_(state);
      });
    }

  private:
    Processor processor_;
};

template <>
class FutureCallback<void> : public FutureCallbackBase<void> {
  public:
    FutureCallback(folly::wangle::Promise<void>&& promise, bool isOneWay)
        : FutureCallbackBase<void>(std::move(promise)),
          isOneWay_(isOneWay) {}

    void requestSent() {
      if (isOneWay_) {
        promise_.setValue();
      }
    };

    void replyReceived(ClientReceiveState&& state) {
      CHECK(!state.isException());
      CHECK(!isOneWay_);

      promise_.setValue();
    }

  private:
    bool isOneWay_;
};


}} // Namespace
