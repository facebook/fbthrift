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
  expired();
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
  // only expire req_ once
  if (!req_) {
    return;
  }
  failWith(
      TApplicationException{"Task expired without processing"},
      kTaskExpiredErrorCode);
}

void EventTask::failWith(folly::exception_wrapper ex, std::string exCode) {
  auto cleanUp = [oneway = oneway_,
                  req = std::move(req_),
                  ex = std::move(ex),
                  exCode = std::move(exCode)]() mutable {
    // if oneway, skip sending back anything
    if (oneway) {
      return;
    }
    req->sendErrorWrapped(std::move(ex), std::move(exCode));
  };

  if (base_->isInEventBaseThread()) {
    cleanUp();
  } else {
    base_->runInEventBaseThread(std::move(cleanUp));
  }
}

void AsyncProcessor::terminateInteraction(
    int64_t,
    Cpp2ConnContext&,
    concurrency::ThreadManager&,
    folly::EventBase&) noexcept {
  LOG(DFATAL) << "This processor doesn't support interactions";
}

void AsyncProcessor::destroyAllInteractions(
    Cpp2ConnContext&,
    concurrency::ThreadManager&,
    folly::EventBase&) noexcept {}

bool GeneratedAsyncProcessor::createInteraction(
    int64_t id,
    const std::string& name,
    Cpp2ConnContext& conn,
    concurrency::ThreadManager& tm,
    folly::EventBase& eb) {
  eb.dcheckIsInEventBaseThread();
  auto promise = std::make_unique<TilePromise>();
  auto promisePtr = promise.get();
  if (!conn.addTile(id, std::move(promise))) {
    return false;
  }

  folly::via(&tm, [=] { return createInteractionImpl(name); })
      .via(&eb)
      .thenTry([&conn, id, &tm, &eb, promisePtr](auto&& t) {
        auto promise = std::unique_ptr<TilePromise>(promisePtr);
        std::unique_ptr<Tile> tile;
        if (t.hasValue()) {
          if (*t) {
            tile = std::move(*t);
          } else {
            tile = std::make_unique<ErrorTile>(
                folly::make_exception_wrapper<std::runtime_error>(
                    "Nullptr returned from interaction constructor"));
            DLOG(FATAL) << "Nullptr returned from interaction constructor";
          }
        } else {
          tile = std::make_unique<ErrorTile>(std::move(t.exception()));
        }

        if (promise->destructionRequested_) {
          // promise not in tile map, not running continuations
          tm.add([tile = std::move(tile)] {});
        } else if (promise->terminationRequested_) {
          // promise not in tile map, continuations will free tile
          tile->terminationRequested_ = true;
          promise->fulfill<InteractionEventTask>(*tile.release(), tm, eb);
        } else {
          // promise and tile managed by pointer in tile map
          promise->fulfill<InteractionEventTask>(*tile, tm, eb);
          conn.resetTile(id, std::move(tile)).release(); // aliases promise
        }
      });
  return true;
}

std::unique_ptr<Tile> GeneratedAsyncProcessor::createInteractionImpl(
    const std::string&) {
  return std::make_unique<ErrorTile>(
      folly::make_exception_wrapper<std::runtime_error>(
          "Handler doesn't perform any interactions"));
}

void GeneratedAsyncProcessor::terminateInteraction(
    int64_t id,
    Cpp2ConnContext& conn,
    concurrency::ThreadManager& tm,
    folly::EventBase& eb) noexcept {
  eb.dcheckIsInEventBaseThread();

  auto tile = conn.removeTile(id);
  if (!tile) {
    return;
  } else if (tile->refCount_ || tile->__fbthrift_isPromise()) {
    tile->terminationRequested_ = true;
    tile.release(); // freed by last decref
  } else {
    tm.add([tile = std::move(tile)] {});
  }
}

void GeneratedAsyncProcessor::destroyAllInteractions(
    Cpp2ConnContext& conn,
    concurrency::ThreadManager& tm,
    folly::EventBase& eb) noexcept {
  eb.dcheckIsInEventBaseThread();

  if (conn.tiles_.empty()) {
    return;
  }

  auto tiles = std::move(conn.tiles_);
  for (auto& [id, tile] : tiles) {
    if (tile->refCount_) {
      if (tile->__fbthrift_isPromise()) {
        static_cast<TilePromise&>(*tile).destructionRequested_ = true;
      } else {
        tile->terminationRequested_ = true;
      }
      tile.release();
    }
  }

  tm.add([tiles = std::move(tiles)] {});
}

bool GeneratedAsyncProcessor::validateRpcKind(
    ResponseChannelRequest::UniquePtr& req,
    RpcKind kind) {
  switch (kind) {
    case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
      switch (req->rpcKind()) {
        case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
          return true;
        case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
          req->sendReply(std::unique_ptr<folly::IOBuf>());
          return true;
        default:
          break;
      }
      break;
    case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      switch (req->rpcKind()) {
        case RpcKind::SINGLE_REQUEST_NO_RESPONSE:
        case RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE:
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
  if (req->rpcKind() != RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
    req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::UNKNOWN_METHOD,
            "Function kind mismatch"),
        kRequestTypeDoesntMatchServiceFunctionType);
  }
  return false;
}

bool GeneratedAsyncProcessor::setUpRequestProcessing(
    ResponseChannelRequest::UniquePtr& req,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    RpcKind kind,
    const char* interaction) {
  if (!validateRpcKind(req, kind)) {
    return false;
  }

  bool interactionMetadataValid;
  if (ctx->getInteractionId()) {
    CHECK(tm) << "EB interactions blocked by validator";
    if (auto interactionCreate = ctx->getInteractionCreate()) {
      if (!interaction ||
          *interactionCreate->interactionName_ref() != interaction) {
        interactionMetadataValid = false;
      } else if (!createInteraction(
                     *interactionCreate->interactionId_ref(),
                     *interactionCreate->interactionName_ref(),
                     *ctx->getConnectionContext(),
                     *tm,
                     *eb)) {
        // Duplicate id is a contract violation so close the connection.
        // Terminate this interaction first so queued requests can't use it
        // (which could result in UB).
        terminateInteraction(
            *interactionCreate->interactionId_ref(),
            *ctx->getConnectionContext(),
            *tm,
            *eb);
        req->sendErrorWrapped(
            TApplicationException(
                "Attempting to create interaction with duplicate id. Failing all requests in that interaction."),
            kConnectionClosingErrorCode);
        return false;
      } else {
        interactionMetadataValid = true;
      }
    } else {
      interactionMetadataValid = !!interaction;
    }
  } else {
    interactionMetadataValid = !interaction;
  }
  if (!interactionMetadataValid) {
    req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::UNKNOWN_METHOD,
            "Interaction and method do not match"),
        kMethodUnknownErrorCode);
    return false;
  }

  return true;
}

concurrency::PRIORITY ServerInterface::getRequestPriority(
    Cpp2RequestContext* ctx,
    concurrency::PRIORITY prio) {
  concurrency::PRIORITY callPriority = ctx->getCallPriority();
  return callPriority == concurrency::N_PRIORITIES ? prio : callPriority;
}

void ServerInterface::setEventBase(folly::EventBase* eb) {
  folly::RequestEventBase::set(eb);
  requestParams_.eventBase_ = eb;
}

void ServerInterface::BlockingThreadManager::add(folly::Func f) {
  std::shared_ptr<concurrency::Runnable> task =
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

bool ServerInterface::BlockingThreadManager::keepAliveAcquire() noexcept {
  auto keepAliveCount = keepAliveCount_.fetch_add(1, std::memory_order_relaxed);
  // We should never increment from 0
  DCHECK(keepAliveCount > 0);
  return true;
}

void ServerInterface::BlockingThreadManager::keepAliveRelease() noexcept {
  auto keepAliveCount = keepAliveCount_.fetch_sub(1, std::memory_order_acq_rel);
  DCHECK(keepAliveCount >= 1);
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
      releaseInteractionInstance();
      req_.reset();
    } else {
      eb_->runInEventBaseThread(
          [req = std::move(req_),
           interaction = std::exchange(interaction_, nullptr),
           tm = getThreadManager(),
           eb = getEventBase()] { releaseInteraction(interaction, tm, eb); });
    }
  }
}

folly::EventBase* HandlerCallbackBase::getEventBase() {
  assert(eb_ != nullptr);
  return eb_;
}

concurrency::ThreadManager* HandlerCallbackBase::getThreadManager() {
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
      queue.move(), reqCtx_->getHeader()->getWriteTransforms()));
}

void HandlerCallbackBase::doExceptionWrapped(folly::exception_wrapper ew) {
  if (req_ == nullptr) {
    LOG(ERROR) << ew.what();
  } else {
    callExceptionInEventBaseThread(ewp_, ew);
  }
}

void HandlerCallbackBase::doAppOverloadedException(const std::string& message) {
  if (eb_->inRunningEventBaseThread()) {
    releaseInteractionInstance();
    std::exchange(req_, {})->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::LOADSHEDDING, message),
        kAppOverloadedErrorCode);
  } else {
    eb_->runInEventBaseThread(
        [message,
         interaction = std::exchange(interaction_, nullptr),
         req = std::move(req_),
         tm = getThreadManager(),
         eb = getEventBase()]() mutable {
          releaseInteraction(interaction, tm, eb);
          req->sendErrorWrapped(
              folly::make_exception_wrapper<TApplicationException>(
                  TApplicationException::LOADSHEDDING, std::move(message)),
              kAppOverloadedErrorCode);
        });
  }
}

void HandlerCallbackBase::sendReply(folly::IOBufQueue queue) {
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);
  if (getEventBase()->isInEventBaseThread()) {
    releaseInteractionInstance();
    std::exchange(req_, {})->sendReply(queue.move(), nullptr, crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         crc32c,
         interaction = std::exchange(interaction_, nullptr),
         tm = getThreadManager(),
         eb = getEventBase()]() mutable {
          releaseInteraction(interaction, tm, eb);
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
    releaseInteractionInstance();
    std::exchange(req_, {})->sendStreamReply(
        queue.move(), std::move(stream), crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         stream = std::move(stream),
         crc32c,
         interaction = std::exchange(interaction_, nullptr),
         tm = getThreadManager(),
         eb = getEventBase()]() mutable {
          releaseInteraction(interaction, tm, eb);
          req->sendStreamReply(queue.move(), std::move(stream), crc32c);
        });
  }
}

void HandlerCallbackBase::sendReply(
    FOLLY_MAYBE_UNUSED
        std::pair<folly::IOBufQueue, apache::thrift::detail::SinkConsumerImpl>&&
            responseAndSinkConsumer) {
#if FOLLY_HAS_COROUTINES
  auto& queue = responseAndSinkConsumer.first;
  auto& sinkConsumer = responseAndSinkConsumer.second;
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);

  if (getEventBase()->isInEventBaseThread()) {
    releaseInteractionInstance();
    std::exchange(req_, {})->sendSinkReply(
        queue.move(), std::move(sinkConsumer), crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         sinkConsumer = std::move(sinkConsumer),
         crc32c,
         interaction = std::exchange(interaction_, nullptr),
         tm = getThreadManager(),
         eb = getEventBase()]() mutable {
          releaseInteraction(interaction, tm, eb);
          req->sendSinkReply(queue.move(), std::move(sinkConsumer), crc32c);
        });
  }
#else
  std::terminate();
#endif
}

void HandlerCallbackBase::releaseInteraction(
    Tile* interaction,
    concurrency::ThreadManager* tm,
    folly::EventBase* eb) {
  if (interaction) {
    interaction->__fbthrift_releaseRef(*tm, *eb);
  }
}
void HandlerCallbackBase::releaseInteractionInstance() {
  releaseInteraction(
      std::exchange(interaction_, nullptr), getThreadManager(), getEventBase());
}

HandlerCallback<void>::HandlerCallback(
    ResponseChannelRequest::UniquePtr req,
    std::unique_ptr<ContextStack> ctx,
    cob_ptr cp,
    exnw_ptr ewp,
    int32_t protoSeqId,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx,
    Tile* interaction)
    : HandlerCallbackBase(
          std::move(req),
          std::move(ctx),
          ewp,
          eb,
          tm,
          reqCtx,
          interaction),
      cp_(cp) {
  this->protoSeqId_ = protoSeqId;
}

void HandlerCallback<void>::complete(folly::Try<folly::Unit>&& r) {
  if (r.hasException()) {
    exception(std::move(r.exception()));
  } else {
    done();
  }
}

void HandlerCallback<void>::doDone() {
  assert(cp_ != nullptr);
  auto queue = cp_(this->protoSeqId_, this->ctx_.get());
  this->ctx_.reset();
  sendReply(std::move(queue));
}

} // namespace thrift
} // namespace apache
