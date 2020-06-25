/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache {
namespace thrift {

constexpr std::chrono::seconds ServerInterface::BlockingThreadManager::kTimeout;
thread_local RequestParams ServerInterface::requestParams_;

EventTask::~EventTask() {
  // req_ needs to be destructed on base_ eventBase thread
  if (!base_->isInEventBaseThread()) {
    expired();
    return;
  }
  if (!oneway_ && req_) {
    req_->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            "Failed to add task to queue, too full"),
        kQueueOverloadedErrorCode);
  }
}

void EventTask::run() {
  if (!oneway_) {
    if (req_ && !req_->isActive()) {
      // del on eventbase thread
      base_->runInEventBaseThread([req = std::move(req_)]() mutable {});
      return;
    }
  }
  taskFunc_(std::move(req_));
}

void EventTask::expired() {
  if (oneway_) {
    if (req_ != nullptr) {
      // del on eventbase thread
      base_->runInEventBaseThread([req = std::move(req_)]() mutable {});
    }
  } else {
    if (req_ != nullptr) {
      base_->runInEventBaseThread([req = std::move(req_)]() {
        req->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(
                "Failed to add task to queue, too full"),
            kQueueOverloadedErrorCode);
      });
    }
  }
}

bool GeneratedAsyncProcessor::validateRpcKind(
    apache::thrift::ResponseChannelRequest::UniquePtr& req,
    apache::thrift::RpcKind kind) {
  switch (kind) {
    case apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      switch (req->rpcKind()) {
        case apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE:
          return true;
        case apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          req->sendReply(std::unique_ptr<folly::IOBuf>());
          return true;
        default:
          break;
      }
      break;
    case apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      switch (req->rpcKind()) {
        case apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        case apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          return true;
        default:
          break;
      }
      break;
    default:
      if (kind == req->rpcKind()) {
        return true;
      }
  }
  if (req->rpcKind() != apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::UNKNOWN_METHOD,
            "Function kind mismatch"),
        kRequestTypeDoesntMatchServiceFunctionType);
  }
  return false;
}

apache::thrift::concurrency::PRIORITY ServerInterface::getRequestPriority(
    apache::thrift::Cpp2RequestContext* ctx,
    apache::thrift::concurrency::PRIORITY prio) {
  apache::thrift::concurrency::PRIORITY callPriority = ctx->getCallPriority();
  return callPriority == apache::thrift::concurrency::N_PRIORITIES
      ? prio
      : callPriority;
}

void ServerInterface::setEventBase(folly::EventBase* eb) {
  folly::RequestEventBase::set(eb);
  requestParams_.eventBase_ = eb;
}

void ServerInterface::BlockingThreadManager::add(folly::Func f) {
  std::shared_ptr<apache::thrift::concurrency::Runnable> task =
      concurrency::FunctionRunner::create(std::move(f));
  try {
    executor_->add(
        std::move(task), std::chrono::milliseconds(kTimeout).count(), 0, false);
    return;
  } catch (...) {
    LOG(FATAL) << "Failed to schedule a task within timeout: "
               << folly::exceptionStr(std::current_exception());
  }
}

bool ServerInterface::BlockingThreadManager::keepAliveAcquire() {
  auto keepAliveCount = keepAliveCount_.fetch_add(1, std::memory_order_relaxed);
  // We should never increment from 0
  DCHECK(keepAliveCount > 0);
  return true;
}

void ServerInterface::BlockingThreadManager::keepAliveRelease() {
  auto keepAliveCount = keepAliveCount_.fetch_sub(1, std::memory_order_acq_rel);
  assert(keepAliveCount >= 1);
  if (keepAliveCount == 1) {
    delete this;
  }
}

HandlerCallbackBase::~HandlerCallbackBase() {
  // req must be deleted in the eb
  if (req_) {
    if (req_->isActive() && ewp_) {
      exception(TApplicationException(
          TApplicationException::INTERNAL_ERROR,
          "apache::thrift::HandlerCallback not completed"));
      return;
    }
    assert(eb_ != nullptr);
    if (eb_->inRunningEventBaseThread()) {
      req_.reset();
    } else {
      eb_->runInEventBaseThread(
          [req = std::move(req_)]() mutable { req.reset(); });
    }
  }
}

void HandlerCallbackBase::exceptionInThread(std::exception_ptr ex) {
  getEventBase()->runInEventBaseThread([=]() {
    this->exception(ex);
    delete this;
  });
}

void HandlerCallbackBase::exceptionInThread(folly::exception_wrapper ew) {
  getEventBase()->runInEventBaseThread([=]() {
    this->exception(ew);
    delete this;
  });
}

void HandlerCallbackBase::exceptionInThread(
    std::unique_ptr<HandlerCallbackBase> thisPtr,
    std::exception_ptr ex) {
  assert(thisPtr != nullptr);
  thisPtr.release()->exceptionInThread(std::move(ex));
}

void HandlerCallbackBase::exceptionInThread(
    std::unique_ptr<HandlerCallbackBase> thisPtr,
    folly::exception_wrapper ew) {
  assert(thisPtr != nullptr);
  thisPtr.release()->exceptionInThread(std::move(ew));
}

folly::EventBase* HandlerCallbackBase::getEventBase() {
  assert(eb_ != nullptr);
  return eb_;
}

apache::thrift::concurrency::ThreadManager*
HandlerCallbackBase::getThreadManager() {
  assert(tm_ != nullptr);
  return tm_;
}

void HandlerCallbackBase::forward(const HandlerCallbackBase& other) {
  eb_ = other.eb_;
  tm_ = other.tm_;
  ewp_ = other.ewp_;
}

folly::Optional<uint32_t> HandlerCallbackBase::checksumIfNeeded(
    folly::IOBufQueue& queue) {
  folly::Optional<uint32_t> crc32c;
  if (req_->isReplyChecksumNeeded() && !queue.empty()) {
    std::unique_ptr<folly::IOBuf> iobuf(queue.move());
    if (iobuf) {
      crc32c = checksum::crc32c(*iobuf);
      queue.append(std::move(iobuf));
    }
  }
  return crc32c;
}

void HandlerCallbackBase::transform(folly::IOBufQueue& queue) {
  // Do any compression or other transforms in this thread, the same thread
  // that serialization happens on.
  queue.append(transport::THeader::transform(
      queue.move(),
      reqCtx_->getHeader()->getWriteTransforms(),
      reqCtx_->getHeader()->getMinCompressBytes()));
}

void HandlerCallbackBase::doExceptionWrapped(folly::exception_wrapper ew) {
  if (req_ == nullptr) {
    LOG(ERROR) << ew.what();
  } else {
    callExceptionInEventBaseThread(ewp_, ew);
  }
}

void HandlerCallbackBase::doAppOverloadedException(const std::string& message) {
  std::exchange(req_, {})->sendErrorWrapped(
      folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::LOADSHEDDING, message),
      kAppOverloadedErrorCode);
}

void HandlerCallbackBase::sendReply(folly::IOBufQueue queue) {
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);
  if (getEventBase()->isInEventBaseThread()) {
    std::exchange(req_, {})->sendReply(queue.move(), nullptr, crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_), queue = std::move(queue), crc32c]() mutable {
          req->sendReply(queue.move(), nullptr, crc32c);
        });
  }
}

void HandlerCallbackBase::sendReply(
    ResponseAndServerStreamFactory&& responseAndStream) {
  auto& queue = responseAndStream.response;
  auto& stream = responseAndStream.stream;
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);
  if (getEventBase()->isInEventBaseThread()) {
    std::exchange(req_, {})->sendStreamReply(
        queue.move(), std::move(stream), crc32c);
  } else {
    getEventBase()->runInEventBaseThread([req = std::move(req_),
                                          queue = std::move(queue),
                                          stream = std::move(stream),
                                          crc32c]() mutable {
      req->sendStreamReply(queue.move(), std::move(stream), crc32c);
    });
  }
}

#if FOLLY_HAS_COROUTINES
void HandlerCallbackBase::sendReply(
    std::pair<folly::IOBufQueue, apache::thrift::detail::SinkConsumerImpl>&&
        responseAndSinkConsumer) {
  auto& queue = responseAndSinkConsumer.first;
  auto& sinkConsumer = responseAndSinkConsumer.second;
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);

  if (getEventBase()->isInEventBaseThread()) {
    std::exchange(req_, {})->sendSinkReply(
        queue.move(), std::move(sinkConsumer), crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         sinkConsumer = std::move(sinkConsumer),
         crc32c]() mutable {
          req->sendSinkReply(queue.move(), std::move(sinkConsumer), crc32c);
        });
  }
}
#endif

HandlerCallback<void>::HandlerCallback(
    ResponseChannelRequest::UniquePtr req,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    cob_ptr cp,
    exnw_ptr ewp,
    int32_t protoSeqId,
    folly::EventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx)
    : HandlerCallbackBase(std::move(req), std::move(ctx), ewp, eb, tm, reqCtx),
      cp_(cp) {
  this->protoSeqId_ = protoSeqId;
}

void HandlerCallback<void>::doneInThread() {
  done();
  delete this;
}

void HandlerCallback<void>::doneInThread(
    std::unique_ptr<HandlerCallback> thisPtr) {
  assert(thisPtr != nullptr);
  thisPtr.release()->doneInThread();
}

void HandlerCallback<void>::complete(folly::Try<folly::Unit>&& r) {
  if (r.hasException()) {
    exception(std::move(r.exception()));
  } else {
    done();
  }
}

void HandlerCallback<void>::completeInThread(folly::Try<folly::Unit>&& r) {
  if (r.hasException()) {
    exceptionInThread(std::move(r.exception()));
  } else {
    doneInThread();
  }
}

void HandlerCallback<void>::completeInThread(
    std::unique_ptr<HandlerCallback> thisPtr,
    folly::Try<folly::Unit>&& r) {
  assert(thisPtr != nullptr);
  thisPtr.release()->completeInThread(std::move(r));
}

void HandlerCallback<void>::doDone() {
  assert(cp_ != nullptr);
  auto queue = cp_(this->protoSeqId_, this->ctx_.get());
  this->ctx_.reset();
  sendReply(std::move(queue));
}

} // namespace thrift
} // namespace apache
