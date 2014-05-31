/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#pragma once

#include "folly/wangle/Future.h"
#include "folly/wangle/ThreadGate.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"

namespace apache { namespace thrift {

// Gate specific to thrift - no extra storage needed
// Assumes TEventBase* pointer outlives result
template <class T>
folly::wangle::Future<T> thriftGate(
  folly::wangle::Promise<T>& p,
  apache::thrift::async::TEventBase* base) {
  folly::MoveWrapper<folly::wangle::Promise<T>> p2;
  auto f = p2->getFuture();
  p.getFuture().then([=](folly::wangle::Try<T>&& t) mutable {
      folly::MoveWrapper<folly::wangle::Try<T>> tm(std::move(t));
      std::function<void()> lambda = [=]() mutable {
        p2->fulfilTry(std::move(*tm));
      };
      base->runInEventBaseThread(lambda);
    });
  return std::move(f);
}

template <typename Result>
class FutureCallbackBase : public RequestCallback {
  public:
    explicit FutureCallbackBase(folly::wangle::Promise<Result>&& promise)
          : promise_(std::move(promise)) {}

    void requestSent() {};

    void requestError(ClientReceiveState&& state) {
      CHECK(state.exception());
      promise_.setException(state.exception());
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
      CHECK(!state.exception());
      CHECK(state.buf());

      try {
        Result result;
        processor_(result, state);
        this->promise_.setValue(std::move(result));
      } catch (...) {
        this->promise_.setException(std::current_exception());
      }
    }

    void requestError(ClientReceiveState&& state) {
      CHECK(state.exception());
      CHECK(!state.buf());
      this->promise_.setException(state.exception());
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
      CHECK(!state.exception());
      CHECK(state.buf());

      try {
        this->promise_.setValue(processor_(state));
      } catch (...) {
        this->promise_.setException(std::current_exception());
      }
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
      CHECK(!state.exception());
      CHECK(!isOneWay_);

      promise_.setValue();
    }

  private:
    bool isOneWay_;
};

template <typename T>
class FutureStreamItemCallback : public InputStreamCallback<T> {
  public:
    explicit FutureStreamItemCallback(folly::wangle::Promise<T>&& promise)
        : promise_(std::move(promise)),
          hasSetPromise_(false) {
    }

    void onReceive(T& value) noexcept {
      this->setResultIfNotSet(value);
    }

    void onFinish() noexcept {
      this->setNoResultIfNotSet();
    }

    void onException(const folly::exception_wrapper& ex) noexcept {
      this->setExceptionIfNotSet(ex);
    }

    void onError(const folly::exception_wrapper& ex) noexcept {
      this->setExceptionIfNotSet(ex);
    }

    void onClose() noexcept {
      this->setNoResultIfNotSet();
    }

  private:
    folly::wangle::Promise<T> promise_;
    bool hasSetPromise_;

    void setResultIfNotSet(T& value) {
      if (!hasSetPromise_) {
        hasSetPromise_ = true;
        promise_.setValue(std::move(value));
        this->close();
      }
    }

    void setNoResultIfNotSet() {
      std::runtime_error exception("No result received.");
      this->setExceptionIfNotSet(std::make_exception_ptr(exception));
    }

    void setExceptionIfNotSet(const folly::exception_wrapper& ex) {
      if (!hasSetPromise_) {
        hasSetPromise_ = true;
        promise_.setException(ex.getExceptionPtr());
      }
    }
};

class FutureStreamEndCallback : public StreamEndCallback {
  public:
    explicit FutureStreamEndCallback(folly::wangle::Promise<void>&& promise)
        : promise_(std::move(promise)) {
    }

    void onStreamComplete() noexcept {
      promise_.setValue();
    }

    void onStreamCancel() noexcept {
      promise_.setValue();
    }

    void onStreamError(const folly::exception_wrapper& ew) noexcept {
      promise_.setException(ew.getExceptionPtr());
    }

  private:
    folly::wangle::Promise<void> promise_;
};

}} // Namespace
