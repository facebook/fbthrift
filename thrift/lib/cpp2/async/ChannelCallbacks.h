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

#ifndef THRIFT_ASYNC_CHANNELCALLBACKS_H_
#define THRIFT_ASYNC_CHANNELCALLBACKS_H_ 1

#include <folly/io/async/HHWheelTimer.h>
#include <folly/io/async/Request.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <memory>

#include <unordered_map>
#include <deque>

namespace apache {
namespace thrift {

class ChannelCallbacks {
 protected:
  /**
   * Callback to manage the lifetime of a two-way call.
   * Deletes itself when it receives both a send and recv callback.
   * Exceptions:
   * 1) If we get a messageSendError, we will never get a recv callback,
   *    so it is safe to delete.
   * 2) timeoutExpired uninstalls the recv callback, so it is safe to delete
   *    if it was already sent.
   *
   * Deletion automatically uninstalls the timeout.
   */
  template <class Channel>
  class TwowayCallback : public MessageChannel::SendCallback,
                         public folly::HHWheelTimer::Callback {
   public:
#define X_CHECK_STATE_EQ(state, expected) \
  CHECK_EQ(static_cast<int>(state), static_cast<int>(expected))
#define X_CHECK_STATE_NE(state, expected) \
  CHECK_NE(static_cast<int>(state), static_cast<int>(expected))
    // Keep separate state for send and receive.
    // Starts as queued for receive (as that's how it's created in
    // HeaderClientChannel::sendRequest).
    // We then try to send and either get messageSendError() or sendQueued().
    // If we get sendQueued(), we know to wait for either messageSendError()
    // or messageSent() before deleting.
    TwowayCallback(Channel* channel,
                   uint32_t sendSeqId,
                   uint16_t protoId,
                   std::unique_ptr<RequestCallback> cb,
                   std::unique_ptr<apache::thrift::ContextStack> ctx,
                   folly::HHWheelTimer* timer,
                   std::chrono::milliseconds timeout,
                   std::chrono::milliseconds chunkTimeout)
        : channel_(channel),
          sendSeqId_(sendSeqId),
          protoId_(protoId),
          cb_(std::move(cb)),
          ctx_(std::move(ctx)),
          sendState_(QState::INIT),
          recvState_(QState::QUEUED),
          cbCalled_(false),
          chunkTimeoutCallback_(this, timer, chunkTimeout) {
      if (timeout > std::chrono::milliseconds(0)) {
        timer->scheduleTimeout(this, timeout);
      }
    }
    ~TwowayCallback() override {
      X_CHECK_STATE_EQ(sendState_, QState::DONE);
      X_CHECK_STATE_EQ(recvState_, QState::DONE);
      CHECK(cbCalled_);
    }
    void sendQueued() override {
      X_CHECK_STATE_EQ(sendState_, QState::INIT);
      sendState_ = QState::QUEUED;
    }
    void messageSent() override {
      X_CHECK_STATE_EQ(sendState_, QState::QUEUED);
      if (!cbCalled_) {
        CHECK(cb_);
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestSent();
        folly::RequestContext::setContext(old_ctx);
      }
      sendState_ = QState::DONE;
      maybeDeleteThis();
    }
    void messageSendError(folly::exception_wrapper&& ex) override {
      X_CHECK_STATE_NE(sendState_, QState::DONE);
      sendState_ = QState::DONE;
      if (recvState_ == QState::QUEUED) {
        recvState_ = QState::DONE;
        channel_->eraseCallback(sendSeqId_, this);
        cancelTimeout();
      }
      if (!cbCalled_) {
        CHECK(cb_);
        cbCalled_ = true;
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(
            std::move(ex), std::move(ctx_), channel_->isSecurityActive()));
        folly::RequestContext::setContext(old_ctx);
        cb_.reset();
      }
      delete this;
    }
    void replyReceived(
        std::unique_ptr<folly::IOBuf> buf,
        std::unique_ptr<apache::thrift::transport::THeader> header) {
      X_CHECK_STATE_NE(sendState_, QState::INIT);
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      recvState_ = QState::DONE;
      cancelTimeout();

      CHECK(!cbCalled_);
      CHECK(cb_);
      cbCalled_ = true;

      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->replyReceived(ClientReceiveState(protoId_,
                                            std::move(buf),
                                            std::move(header),
                                            std::move(ctx_),
                                            channel_->isSecurityActive(),
                                            true));
      cb_.reset();

      folly::RequestContext::setContext(old_ctx);
      maybeDeleteThis();
    }
    void partialReplyReceived(
        std::unique_ptr<folly::IOBuf> buf,
        std::unique_ptr<apache::thrift::transport::THeader> header) {
      X_CHECK_STATE_NE(sendState_, QState::INIT);
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      chunkTimeoutCallback_.resetTimeout();

      CHECK(cb_);

      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->replyReceived(ClientReceiveState(protoId_,
                                            std::move(buf),
                                            std::move(header),
                                            ctx_,
                                            channel_->isSecurityActive()));

      folly::RequestContext::setContext(old_ctx);
    }
    void requestError(folly::exception_wrapper ex) {
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      recvState_ = QState::DONE;
      cancelTimeout();
      CHECK(cb_);
      if (!cbCalled_) {
        cbCalled_ = true;
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(
            std::move(ex), std::move(ctx_), channel_->isSecurityActive()));
        folly::RequestContext::setContext(old_ctx);
        cb_.reset();
      }

      maybeDeleteThis();
    }
    void timeoutExpired() noexcept override {
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      channel_->eraseCallback(sendSeqId_, this);
      recvState_ = QState::DONE;

      if (!cbCalled_) {
        using apache::thrift::transport::TTransportException;

        cbCalled_ = true;
        TTransportException ex(TTransportException::TIMED_OUT, "Timed Out");
        ex.setOptions(TTransportException::CHANNEL_IS_VALID); // framing okay
        auto old_ctx = folly::RequestContext::setContext(cb_->context_);
        cb_->requestError(ClientReceiveState(
            folly::make_exception_wrapper<TTransportException>(std::move(ex)),
            std::move(ctx_),
            channel_->isSecurityActive()));
        folly::RequestContext::setContext(old_ctx);
        cb_.reset();
      }
      maybeDeleteThis();
    }
    void expire() {
      X_CHECK_STATE_EQ(recvState_, QState::QUEUED);
      channel_->eraseCallback(sendSeqId_, this);
      recvState_ = QState::DONE;
      cbCalled_ = true;
      cb_.reset();

      maybeDeleteThis();
    }

   private:
    enum class QState { INIT, QUEUED, DONE };
    void maybeDeleteThis() {
      if (sendState_ == QState::DONE && recvState_ == QState::DONE) {
        delete this;
      }
    }
    Channel* channel_;
    uint32_t sendSeqId_;
    uint16_t protoId_;
    std::unique_ptr<RequestCallback> cb_;
    std::shared_ptr<apache::thrift::ContextStack> ctx_;
    QState sendState_;
    QState recvState_;
    bool cbCalled_; // invariant: (cb_ == nullptr) == cbCalled_
    class TimerCallback : public folly::HHWheelTimer::Callback {
     public:
      TimerCallback(TwowayCallback* cb,
                    folly::HHWheelTimer* timer,
                    std::chrono::milliseconds chunkTimeout)
          : cb_(cb), timer_(timer), chunkTimeout_(chunkTimeout) {
        resetTimeout();
      }
      void timeoutExpired() noexcept override { cb_->timeoutExpired(); }
      void resetTimeout() {
        cancelTimeout();
        if (chunkTimeout_.count() > 0) {
          timer_->scheduleTimeout(this, chunkTimeout_);
        }
      }

     private:
      TwowayCallback* cb_;
      folly::HHWheelTimer* timer_;
      std::chrono::milliseconds chunkTimeout_;
    } chunkTimeoutCallback_;
#undef X_CHECK_STATE_NE
#undef X_CHECK_STATE_EQ
  };

  class OnewayCallback : public MessageChannel::SendCallback {
   public:
    OnewayCallback(std::unique_ptr<RequestCallback> cb,
                   std::unique_ptr<apache::thrift::ContextStack> ctx,
                   bool isSecurityActive)
        : cb_(std::move(cb)),
          ctx_(std::move(ctx)),
          isSecurityActive_(isSecurityActive) {}
    void sendQueued() override {}
    void messageSent() override {
      CHECK(cb_);
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestSent();
      folly::RequestContext::setContext(old_ctx);
      delete this;
    }
    void messageSendError(folly::exception_wrapper&& ex) override {
      CHECK(cb_);
      auto old_ctx = folly::RequestContext::setContext(cb_->context_);
      cb_->requestError(
          ClientReceiveState(ex, std::move(ctx_), isSecurityActive_));
      folly::RequestContext::setContext(old_ctx);
      delete this;
    }

   private:
    std::unique_ptr<RequestCallback> cb_;
    std::unique_ptr<apache::thrift::ContextStack> ctx_;
    bool isSecurityActive_;
  };
};
}
} // apache::thrift

#endif // THRIFT_ASYNC_CHANNELCALLBACKS_H_
