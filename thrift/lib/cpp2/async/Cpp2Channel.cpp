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

#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <folly/io/IOBufQueue.h>
#include <folly/io/Cursor.h>
#include <folly/String.h>

#include <glog/logging.h>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using namespace folly::io;
using namespace apache::thrift::transport;
using apache::thrift::async::TEventBase;
using namespace apache::thrift::concurrency;
using apache::thrift::async::TAsyncTransport;

namespace apache { namespace thrift {

const uint32_t Cpp2Channel::DEFAULT_BUFFER_SIZE;

Cpp2Channel::Cpp2Channel(
  const std::shared_ptr<TAsyncTransport>& transport,
  std::unique_ptr<FramingChannelHandler> framingHandler,
  std::unique_ptr<ProtectionChannelHandler> protectionHandler)
    : transport_(transport)
    , queue_(new IOBufQueue(IOBufQueue::cacheChainLength()))
    , readBufferSize_(DEFAULT_BUFFER_SIZE)
    , remaining_(readBufferSize_)
    , recvCallback_(nullptr)
    , closing_(false)
    , eofInvoked_(false)
    , queueSends_(true)
    , protectionHandler_(std::move(protectionHandler))
    , framingHandler_(std::move(framingHandler))
    , pipeline_(new Pipeline(transport, this)) {
  transportHandler_ = pipeline_->getHandler<TAsyncTransportHandler>(0);

  if (!protectionHandler_) {
    protectionHandler_.reset(new ProtectionChannelHandler);
  }
}

folly::Future<void> Cpp2Channel::close(Context* ctx) {
  DestructorGuard dg(this);
  closing_ = true;
  processReadEOF();
  return ctx->fireClose();
}

void Cpp2Channel::closeNow() {
  // closeNow can invoke callbacks
  DestructorGuard dg(this);

  // If there are sends queued, cancel them
  if (isLoopCallbackScheduled()) {
    cancelLoopCallback();
  }

  closing_ = true;
  if (pipeline_) {
    if (transport_) {
      pipeline_->close();
    }
    // We must remove the circular reference to this, or descrution order
    // issues ensue
    pipeline_->getHandler<folly::wangle::ChannelHandlerPtr<Cpp2Channel, false>>(1)->setHandler(nullptr);
    pipeline_.reset();
  }
}

void Cpp2Channel::destroy() {
  closeNow();
  MessageChannel::destroy();
}

void Cpp2Channel::attachEventBase(
  TEventBase* eventBase) {
  transportHandler_->attachEventBase(eventBase);
}

void Cpp2Channel::detachEventBase() {
  transportHandler_->detachEventBase();
}

TEventBase* Cpp2Channel::getEventBase() {
  return transport_->getEventBase();
}

void Cpp2Channel::read(Context* ctx, folly::IOBufQueue& q) {
  assert(recvCallback_);

  DestructorGuard dg(this);

  if (recvCallback_ && recvCallback_->shouldSample() && !sample_) {
    sample_.reset(new RecvCallback::sample);
    sample_->readBegin = Util::currentTimeUsec();
  }

  // Remaining for this packet.  Will update the class member
  // variable below for the next call to getReadBuffer
  size_t remaining = 0;

  // Loop as long as there are complete (decrypted and deframed) frames.
  // Partial frames are stored inside the handlers between calls to
  // readDataAvailable.
  // On the last iteration, remaining_ is updated to the anticipated remaining
  // frame length (if we're in the middle of a frame) or to readBufferSize_
  // (if we are exactly between frames)
  while (true) {
    unique_ptr<IOBuf> unframed;

    auto ex = folly::try_and_catch<std::exception>([&]() {
      IOBufQueue* decrypted;
      size_t rem = 0;
      std::tie(decrypted, rem) = protectionHandler_->decrypt(&q);

      if (!decrypted) {
        // no full message available, remember how many more bytes we need
        // and continue to frame decoding (because frame decoder might have
        // cached messages)
        remaining = rem;
      }

      // message decrypted
      std::tie(unframed, rem) = framingHandler_->removeFrame(decrypted);

      if (!unframed && remaining == 0) {
        // no full message available, update remaining but only if previous
        // handler (encryption) hasn't already provided a value
        remaining = rem;
      }
    });
    if (ex) {
      if (recvCallback_) {
        VLOG(5) << "Failed to read a message header";
        recvCallback_->messageReceiveErrorWrapped(std::move(ex));
      } else {
        LOG(ERROR) << "Failed to read a message header";
      }
      closeNow();
      return;
    }

    if (!unframed) {
      // no more data
      remaining_ = remaining > 0 ? remaining : readBufferSize_;
      pipeline_->setReadBufferSettings(readBufferSize_, remaining_);
      return;
    }

    if (!recvCallback_) {
      LOG(ERROR) << "Received a message, but no recvCallback_ installed!";
      continue;
    }

    if (sample_) {
      sample_->readEnd = Util::currentTimeUsec();
    }
    recvCallback_->messageReceived(std::move(unframed), std::move(sample_));
    if (closing_) {
      return; // don't call more callbacks if we are going to be destroyed
    }
  }
}

void Cpp2Channel::readEOF(Context* ctx) {
  processReadEOF();
}

void Cpp2Channel::readException(Context* ctx, folly::exception_wrapper e)  {
  DestructorGuard dg(this);
  VLOG(5) << "Got a read error: " << folly::exceptionStr(e);
  if (recvCallback_) {
    recvCallback_->messageReceiveErrorWrapped(std::move(e));
  }
  processReadEOF();
}

void Cpp2Channel::writeSuccess() noexcept {
  assert(sendCallbacks_.size() > 0);

  DestructorGuard dg(this);
  for (auto& cb : sendCallbacks_.front()) {
    cb->messageSent();
  }
  sendCallbacks_.pop_front();
}

void Cpp2Channel::writeError(size_t bytesWritten,
                               const TTransportException& ex) noexcept {
  assert(sendCallbacks_.size() > 0);

  // Pop last write request, call error callback

  DestructorGuard dg(this);
  VLOG(5) << "Got a write error: " << folly::exceptionStr(ex);
  for (auto& cb : sendCallbacks_.front()) {
    cb->messageSendError(
        folly::make_exception_wrapper<TTransportException>(ex));
  }
  sendCallbacks_.pop_front();
}

void Cpp2Channel::processReadEOF() noexcept {
  transport_->setReadCallback(nullptr);

  VLOG(5) << "Got an EOF on channel";
  if (recvCallback_ && !eofInvoked_) {
    eofInvoked_ = true;
    recvCallback_->messageChannelEOF();
  }
}

// Low level interface
void Cpp2Channel::sendMessage(SendCallback* callback,
                                std::unique_ptr<folly::IOBuf>&& buf) {
  // Callback may be null.
  assert(buf);

  if (!transport_->good()) {
    VLOG(5) << "Channel is !good() in sendMessage";
    // Callback must be last thing in sendMessage, or use guard
    if (callback) {
      callback->messageSendError(
          folly::make_exception_wrapper<TTransportException>(
            "Channel is !good()"));
    }
    return;
  }

  buf = framingHandler_->addFrame(std::move(buf));
  buf = protectionHandler_->encrypt(std::move(buf));

  if (!queueSends_) {
    // Send immediately.
    std::vector<SendCallback*> cbs;
    if (callback) {
      cbs.push_back(callback);
    }
    sendCallbacks_.push_back(std::move(cbs));

    auto future = pipeline_->write(std::move(buf));
    future.then([=](folly::Try<void>&& t) {
      try {
        t.throwIfFailed();
        writeSuccess();
      } catch (const TTransportException& ex) {
        VLOG(5) << "Got a write error: " <<
          folly::exceptionStr(ex);
        writeError(0, ex);
      } catch (const std::exception& ex) {
        VLOG(5) << "Got a write error: " <<
          folly::exceptionStr(ex);
        writeError(0, TTransportException(ex.what()));
      }
    });
  } else {
    // Delay sends to optimize for fewer syscalls
    if (!sends_) {
      DCHECK(!isLoopCallbackScheduled());
      // Buffer all the sends, and call writev once per event loop.
      sends_ = std::move(buf);
      getEventBase()->runInLoop(this);
      std::vector<SendCallback*> cbs;
      if (callback) {
        cbs.push_back(callback);
      }
      sendCallbacks_.push_back(std::move(cbs));
    } else {
      DCHECK(isLoopCallbackScheduled());
      sends_->prependChain(std::move(buf));
      if (callback) {
        sendCallbacks_.back().push_back(callback);
      }
    }
    if (callback) {
      callback->sendQueued();
    }
  }
}

void Cpp2Channel::runLoopCallback() noexcept {
  assert(sends_);

  auto future = pipeline_->write(std::move(sends_));
  future.then([=](folly::Try<void>&& t) {
    if (t.withException<TTransportException>([&](const TTransportException& ex) {
          writeError(0, ex);
        }) ||
      t.withException<std::exception>([&](const std::exception& ex) {
          writeError(0, TTransportException(ex.what()));
        })) {
      return;
    } else {
      writeSuccess();
    }
  });
}

void Cpp2Channel::setReceiveCallback(RecvCallback* callback) {
  if (recvCallback_ == callback) {
    return;
  }

  // Still want to set recvCallback_ for outstanding EOFs
  recvCallback_ = callback;

  if (!transport_->good()) {
    transport_->setReadCallback(nullptr);
    return;
  }

  if (callback) {
    transportHandler_->attachReadCallback();
  } else {
    transportHandler_->detachReadCallback();
  }
}

std::pair<folly::IOBufQueue*, size_t>
ProtectionChannelHandler::decrypt(folly::IOBufQueue* q) {
  if (protectionState_ == ProtectionState::INVALID) {
    throw TTransportException("protection state is invalid");
  }

  if (protectionState_ != ProtectionState::VALID) {
    // not an encrypted message, so pass-through
    return std::make_pair(q, 0);
  }

  assert(saslEndpoint_ != nullptr);
  size_t remaining = 0;

  if (!q->front() || q->front()->empty()) {
    return std::make_pair(nullptr, 0);
  }
  // decrypt
  unique_ptr<IOBuf> unwrapped = saslEndpoint_->unwrap(q, &remaining);
  assert(bool(unwrapped) ^ (remaining > 0));   // 1 and only 1 should be true
  if (unwrapped) {
    queue_.append(std::move(unwrapped));

    return std::make_pair(&queue_, remaining);
  } else {
    return std::make_pair(nullptr, remaining);
  }
}

std::unique_ptr<folly::IOBuf>
ProtectionChannelHandler::encrypt(std::unique_ptr<folly::IOBuf> buf) {
  if (protectionState_ == ProtectionState::VALID) {
    assert(saslEndpoint_);
    return saslEndpoint_->wrap(std::move(buf));
  }
  return std::move(buf);
}

}} // apache::thrift
