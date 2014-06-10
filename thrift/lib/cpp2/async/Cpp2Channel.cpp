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

#include "thrift/lib/cpp2/async/Cpp2Channel.h"
#include "thrift/lib/cpp/transport/TTransportException.h"
#include "thrift/lib/cpp/concurrency/Util.h"

#include "folly/io/IOBufQueue.h"
#include "folly/io/Cursor.h"
#include "folly/String.h"

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

Cpp2Channel::Cpp2Channel(
  const std::shared_ptr<TAsyncTransport>& transport,
  std::unique_ptr<FramingChannelHandler> framingHandler)
    : transport_(transport)
    , queue_(new IOBufQueue)
    , remaining_(DEFAULT_BUFFER_SIZE)
    , recvCallback_(nullptr)
    , closing_(false)
    , eofInvoked_(false)
    , queueSends_(true)
    , protectionHandler_(folly::make_unique<ProtectionChannelHandler>())
    , framingHandler_(std::move(framingHandler)) {
}

void Cpp2Channel::closeNow() {
  // closeNow can invoke callbacks
  DestructorGuard dg(this);
  closing_ = true;
  if (transport_) {
    transport_->setReadCallback(nullptr);
    transport_->closeNow();

    processReadEOF(); // Call failure callbacks
  }
}

void Cpp2Channel::destroy() {
  closeNow();
  MessageChannel::destroy();
}

void Cpp2Channel::attachEventBase(
  TEventBase* eventBase) {
  transport_->attachEventBase(eventBase);
}

void Cpp2Channel::detachEventBase() {
  if (transport_->getReadCallback() == this) {
    transport_->setReadCallback(nullptr);
  }

  transport_->detachEventBase();
}

TEventBase* Cpp2Channel::getEventBase() {
  return transport_->getEventBase();
}

void Cpp2Channel::getReadBuffer(void** bufReturn, size_t* lenReturn) {
  // If remaining_ > DEFAULT_BUFFER_SIZE, preallocate only allocates
  // DEFAULT_BUFFER_SIZE chunks at a time.
  pair<void*, uint32_t> data = queue_->preallocate(remaining_,
                                                   DEFAULT_BUFFER_SIZE);

  *lenReturn = data.second;
  *bufReturn = data.first;
}

void Cpp2Channel::readDataAvailable(size_t len) noexcept {
  assert(recvCallback_);
  assert(len > 0);

  DestructorGuard dg(this);

  queue_->postallocate(len);

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
  // frame length (if we're in the middle of a frame) or to DEFAULT_BUFFER_SIZE
  // (if we are exactly between frames)
  while (true) {
    unique_ptr<IOBuf> unframed;

    auto ex = folly::try_and_catch<std::exception>([&]() {
      unique_ptr<IOBuf> decrypted;
      size_t rem = 0;
      std::tie(decrypted, rem) = protectionHandler_->decrypt(queue_.get());

      if (!decrypted) {
        // no full message available, remember how many more bytes we need
        // and continue to frame decoding (because frame decoder might have
        // cached messages)
        remaining = rem;
      }

      // message decrypted
      IOBufQueue q;
      q.append(std::move(decrypted));
      std::tie(unframed, rem) = framingHandler_->removeFrame(&q);

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
      remaining_ = remaining > 0 ? remaining : DEFAULT_BUFFER_SIZE;
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

void Cpp2Channel::readEOF() noexcept {
  processReadEOF();
}

void Cpp2Channel::readError(const TTransportException & ex) noexcept {
  DestructorGuard dg(this);
  VLOG(5) << "Got a read error: " << folly::exceptionStr(ex);
  if (recvCallback_) {
    recvCallback_->messageReceiveErrorWrapped(
        folly::make_exception_wrapper<TTransportException>(ex));
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
    transport_->writeChain(this, std::move(buf));
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
  transport_->writeChain(this, std::move(sends_));
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
    transport_->setReadCallback(this);
  } else {
    transport_->setReadCallback(nullptr);
  }
}

std::pair<std::unique_ptr<folly::IOBuf>, size_t>
ProtectionChannelHandler::decrypt(folly::IOBufQueue* q) {
  if (protectionState_ == ProtectionState::INVALID) {
    throw TTransportException("protection state is invalid");
  }

  if (protectionState_ != ProtectionState::VALID) {
    // not an encrypted message, so pass-through
    return std::make_pair(q->move(), 0);
  }

  assert(saslEndpoint_ != nullptr);
  size_t remaining = 0;

  if (!q->front() || q->front()->empty()) {
    return std::make_pair(unique_ptr<IOBuf>(), 0);
  }
  // decrypt
  unique_ptr<IOBuf> unwrapped = saslEndpoint_->unwrap(q, &remaining);
  assert(bool(unwrapped) ^ (remaining > 0));   // 1 and only 1 should be true

  return std::make_pair(std::move(unwrapped), remaining);
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
