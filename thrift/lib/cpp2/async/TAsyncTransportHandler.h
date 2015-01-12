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

#include <folly/wangle/channel/ChannelHandler.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

namespace apache { namespace thrift {

class TAsyncTransportHandler
  : public folly::wangle::BytesToBytesHandler,
    public async::TAsyncTransport::ReadCallback {
 public:
  explicit TAsyncTransportHandler(
      std::shared_ptr<async::TAsyncTransport> transport)
    : transport_(std::move(transport)) {}

  TAsyncTransportHandler(TAsyncTransportHandler&&) = default;

  ~TAsyncTransportHandler() {
    if (transport_) {
      detachReadCallback();
    }
  }

  void attachReadCallback() {
    transport_->setReadCallback(transport_->good() ? this : nullptr);
  }

  void detachReadCallback() {
    if (transport_->getReadCallback() == this) {
      transport_->setReadCallback(nullptr);
    }
  }

  void attachEventBase(folly::EventBase* eventBase) {
    if (eventBase && !transport_->getEventBase()) {
      transport_->attachEventBase(eventBase);
    }
  }

  void detachEventBase() {
    detachReadCallback();
    if (transport_->getEventBase()) {
      transport_->detachEventBase();
    }
  }

  void attachPipeline(Context* ctx) override {
    CHECK(!ctx_);
    ctx_ = ctx;
  }

  folly::Future<void> write(
      Context* ctx,
      std::unique_ptr<folly::IOBuf> buf) override {
    if (UNLIKELY(!buf)) {
      return folly::makeFuture();
    }

    if (!transport_->good()) {
      VLOG(5) << "transport is closed in write()";
      return folly::makeFuture<void>(
          transport::TTransportException("transport is closed in write()"));
    }

    auto cb = new WriteCallback();
    auto future = cb->promise_.getFuture();
    transport_->writeChain(cb, std::move(buf), ctx->getWriteFlags());
    return future;
  };

  folly::Future<void> close(Context* ctx) {
    if (transport_) {
      detachReadCallback();
      transport_->closeNow();
    }
    return folly::makeFuture();
  }

  // Must override to avoid warnings about hidden overloaded virtual due to
  // TAsyncTransport::ReadCallback::readEOF()
  void readEOF(Context* ctx) override {
    ctx->fireReadEOF();
  }

  void getReadBuffer(void** bufReturn, size_t* lenReturn) override {
    const auto readBufferSettings = ctx_->getReadBufferSettings();
    const auto ret = bufQueue_.preallocate(
        readBufferSettings.first,
        readBufferSettings.second);
    *bufReturn = ret.first;
    *lenReturn = ret.second;
  }

  void readDataAvailable(size_t len) noexcept override {
    bufQueue_.postallocate(len);
    ctx_->fireRead(bufQueue_);
  }

  void readEOF() noexcept override {
    ctx_->fireReadEOF();
  }

  void readError(const transport::TTransportException& ex)
    noexcept override {
    ctx_->fireReadException(
        folly::make_exception_wrapper<transport::TTransportException>(
            std::move(ex)));
  }

 private:
  class WriteCallback : private async::TAsyncTransport::WriteCallback {
    void writeSuccess() noexcept override {
      promise_.setValue();
      delete this;
    }

    void writeError(size_t bytesWritten,
                    const transport::TTransportException& ex)
      noexcept override {
      promise_.setException(std::make_exception_ptr(ex));
      delete this;
    }

   private:
    friend class TAsyncTransportHandler;
    folly::Promise<void> promise_;
  };

  Context* ctx_{nullptr};
  folly::IOBufQueue bufQueue_;
  std::shared_ptr<async::TAsyncTransport> transport_;
};

}}
