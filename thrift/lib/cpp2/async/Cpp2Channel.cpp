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

#include <folly/io/async/EventBaseManager.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/Cursor.h>
#include <folly/String.h>

#include <glog/logging.h>

using std::unique_ptr;
using std::pair;
using folly::IOBuf;
using folly::IOBufQueue;
using namespace folly::io;
using namespace folly::wangle;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::async::TEventBase;
using namespace apache::thrift::concurrency;
using apache::thrift::async::TAsyncTransport;

namespace apache { namespace thrift {

const uint32_t Cpp2Channel::DEFAULT_BUFFER_SIZE;

Cpp2Channel::Cpp2Channel(
  const std::shared_ptr<TAsyncTransport>& transport,
  std::unique_ptr<FramingHandler> framingHandler,
  std::unique_ptr<ProtectionHandler> protectionHandler)
    : queue_(new IOBufQueue)
    , readBufferSize_(DEFAULT_BUFFER_SIZE)
    , remaining_(readBufferSize_)
    , recvCallback_(nullptr)
    , closing_(false)
    , eofInvoked_(false)
    , queueSends_(true)
    , protectionHandler_(std::move(protectionHandler))
    , pipeline_(new Pipeline(
          transport,
          OutputBufferingHandler{},
          protectionHandler_,
          std::move(framingHandler),
          this)) {
  transportHandler_ = pipeline_->getHandler<TAsyncTransportHandler>(0);
  if (!protectionHandler_) {
    protectionHandler_.reset(new ProtectionHandler());
    pipeline_->getHandler<ChannelHandlerPtr<ProtectionHandler>>(2)->setHandler(
        protectionHandler_);
  }
  pipeline_->attachTransport(transport);
}

void Cpp2Channel::closeNow() {
  // closeNow can invoke callbacks
  DestructorGuard dg(this);
  if (pipeline_) {
    pipeline_->close();
    // We must remove the circular reference to this, or descrution order
    // issues ensue
    pipeline_->getHandler<ChannelHandlerPtr<Cpp2Channel, false>>(4)->setHandler(nullptr);
    pipeline_->getHandler<ChannelHandlerPtr<FramingHandler>>(3)->getHandler()->cancel();
    pipeline_.reset();
  }
}

void Cpp2Channel::destroy() {
  closeNow();
  MessageChannel::destroy();
}

void Cpp2Channel::attachEventBase(TEventBase* eventBase) {
  transportHandler_->attachEventBase(eventBase);
}

void Cpp2Channel::detachEventBase() {
  transportHandler_->detachEventBase();
}

TEventBase* Cpp2Channel::getEventBase() {
  return pipeline_->getTransport()->getEventBase();
}

void Cpp2Channel::sendMessage(SendCallback* callback,
                                std::unique_ptr<folly::IOBuf>&& buf) {
  DestructorGuard dg(this);
  // Callback may be null.
  assert(buf);

  auto future = pipeline_->write(std::move(buf));
  if (callback) {
    callback->sendQueued();
    future.then([=](folly::Try<void>&& t) {
      try {
        t.throwIfFailed();
        callback->messageSent();
      } catch (TTransportException& ex) {
        VLOG(5) << "Got a write error: " <<
          folly::exceptionStr(ex);
        callback->messageSendError(
          folly::make_exception_wrapper<TTransportException>(ex));
      } catch (folly::BrokenPromise& ex) {
        VLOG(5) << "Channel closed: " <<
          folly::exceptionStr(ex);
        callback->messageSendError(
          folly::make_exception_wrapper<folly::BrokenPromise>(ex));
      } catch (const std::exception& ex) {
        LOG(ERROR) << "Caught non-TTransport exception while writing: " <<
                      ex.what();
      }
    });
  }
}

void Cpp2Channel::setReceiveCallback(RecvCallback* callback) {
  if (recvCallback_ == callback) {
    return;
  }

  recvCallback_ = callback;
  if (callback) {
    transportHandler_->attachReadCallback();
  } else {
    transportHandler_->detachReadCallback();
  }
}

void Cpp2Channel::read(Context* ctx, folly::IOBufQueue& q) {
  DestructorGuard dg(this);

  if (!recvCallback_) {
    LOG(ERROR) << "Received a message, but no recvCallback_ installed!";
    return;
  }

  if (recvCallback_ && recvCallback_->shouldSample() && !sample_) {
    sample_.reset(new RecvCallback::sample);
    sample_->readBegin = Util::currentTimeUsec();
  }

  if (sample_) {
    sample_->readEnd = Util::currentTimeUsec();
  }
  recvCallback_->messageReceived(q.move(), std::move(sample_));
}

void Cpp2Channel::readEOF(Context* ctx) {
  DestructorGuard dg(this);
  processReadEOF();
}

void Cpp2Channel::readException(Context* ctx, folly::exception_wrapper e) {
  DestructorGuard dg(this);
  VLOG(5) << "Got a read error: " << e.what();
  if (recvCallback_) {
    recvCallback_->messageReceiveErrorWrapped(std::move(e));
  }
  // We don't want to call closeNow, because the callback may still
  // want to do things with the handler, like call getEventBase()
  if (pipeline_) {
    pipeline_->close();
  }
  processReadEOF();
}

folly::Future<void> Cpp2Channel::close(Context* ctx) {
  DestructorGuard dg(this);
  closing_ = true;
  processReadEOF();
  return ctx->fireClose();
}

void Cpp2Channel::processReadEOF() noexcept {
  VLOG(5) << "Got an EOF on channel";
  if (recvCallback_ && !eofInvoked_) {
    eofInvoked_ = true;
    recvCallback_->messageChannelEOF();
  }
}

}} // apache::thrift
