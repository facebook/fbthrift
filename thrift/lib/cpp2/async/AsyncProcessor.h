/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#pragma once

#include <exception>
#include <mutex>
#include <string>
#include <string_view>
#include <variant>

#include <folly/ExceptionWrapper.h>
#include <folly/Portability.h>
#include <folly/String.h>
#include <folly/Synchronized.h>
#include <folly/Unit.h>
#include <folly/concurrency/memory/PrimaryPtr.h>
#include <folly/container/F14Map.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <folly/lang/Badge.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/SerializationSwitch.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TrustedServerException.h>
#include <thrift/lib/cpp2/async/AsyncProcessorFactory.h>
#include <thrift/lib/cpp2/async/Interaction.h>
#include <thrift/lib/cpp2/async/ReplyInfo.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>
#include <thrift/lib/cpp2/async/ServerRequestData.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/async/processor/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/processor/AsyncProcessorFunc.h>
#include <thrift/lib/cpp2/async/processor/EventTask.h>
#include <thrift/lib/cpp2/async/processor/GeneratedAsyncProcessorBase.h>
#include <thrift/lib/cpp2/async/processor/RequestParams.h>
#include <thrift/lib/cpp2/async/processor/RequestTask.h>
#include <thrift/lib/cpp2/async/processor/ServerInterface.h>
#include <thrift/lib/cpp2/async/processor/ServerRequest.h>
#include <thrift/lib/cpp2/async/processor/ServerRequestHelper.h>
#include <thrift/lib/cpp2/async/processor/ServerRequestTask.h>
#include <thrift/lib/cpp2/async/processor/ServiceHandlerBase.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/schema/SchemaV1.h>
#include <thrift/lib/cpp2/server/ConcurrencyControllerInterface.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/IOWorkerContext.h>
#include <thrift/lib/cpp2/server/RequestPileInterface.h>
#include <thrift/lib/cpp2/server/ResourcePoolHandle.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/cpp2/util/IntrusiveSharedPtr.h>
#include <thrift/lib/cpp2/util/TypeErasedValue.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache::thrift {

namespace detail {
template <typename T>
struct HandlerCallbackHelper;
}

class ServerRequest;
class IResourcePoolAcceptor;

template <class T>
class HandlerCallback;

class HandlerCallbackBase;

namespace detail {
// These functions allow calling the function within generated code since
// the corresponding functions are protected in HandlerCallbackBase

#if FOLLY_HAS_COROUTINES
bool shouldProcessServiceInterceptorsOnRequest(HandlerCallbackBase&) noexcept;

folly::coro::Task<void> processServiceInterceptorsOnRequest(
    HandlerCallbackBase&,
    detail::ServiceInterceptorOnRequestArguments arguments);
#endif // FOLLY_HAS_COROUTINES
} // namespace detail

/**
 * HandlerCallback class for async callbacks.
 *
 * These are constructed by the generated code, and your handler calls
 * either result(value), done(), exception(ex), or appOverloadedException() to
 * finish the async call.  Only one of these must be called, otherwise your
 * client will likely get confused with multiple response messages.
 */
class HandlerCallbackBase {
 private:
  IOWorkerContext::ReplyQueue& getReplyQueue() {
    auto workerContext = reqCtx_->getConnectionContext()->getWorkerContext();
    DCHECK(workerContext != nullptr);
    return workerContext->getReplyQueue();
  }

 protected:
  using exnw_ptr = void (*)(
      ResponseChannelRequest::UniquePtr,
      int32_t protoSeqId,
      ContextStack*,
      folly::exception_wrapper,
      Cpp2RequestContext*);

  void maybeNotifyComplete() {
    // We do not expect this to be reentrant
    if (notifyRequestPile_) {
      notifyRequestPile_->onRequestFinished(requestData_);
      notifyRequestPile_ = nullptr;
    }
    if (notifyConcurrencyController_) {
      notifyConcurrencyController_->onRequestFinished(requestData_);
      notifyConcurrencyController_ = nullptr;
    }
  }

  util::BasicIntrusiveSharedPtrControlBlock intrusivePtrControlBlock_;

 public:
  struct IntrusiveSharedPtrAccess {
    static void acquireRef(HandlerCallbackBase& callback) noexcept {
      callback.intrusivePtrControlBlock_.acquireRef();
    }
    static util::BasicIntrusiveSharedPtrControlBlock::RefCount releaseRef(
        HandlerCallbackBase& callback) noexcept {
      return callback.intrusivePtrControlBlock_.releaseRef();
    }
  };

  using Ptr =
      util::IntrusiveSharedPtr<HandlerCallbackBase, IntrusiveSharedPtrAccess>;

 private:
  Ptr sharedFromThis() {
    // Constructing from raw pointer is safe in this case because
    // `this` is guaranteed to be alive while the current
    // function is executing.
    return Ptr(typename Ptr::UnsafelyFromRawPointer(), this);
  }

 public:
  struct MethodNameInfo {
    // The "leaf" or most derived service name
    std::string_view serviceName;
    // The service name where the incoming method name is defined (could be
    // different if inherited service)
    std::string_view definingServiceName;
    // Self explanatory
    std::string_view methodName;
    // This is {serviceName}.{methodName}
    std::string_view qualifiedMethodName;
  };

  HandlerCallbackBase() : eb_(nullptr), reqCtx_(nullptr), protoSeqId_(0) {}

  HandlerCallbackBase(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      exnw_ptr ewp,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx,
      TilePtr&& interaction = {})
      : req_(std::move(req)),
        ctx_(std::move(ctx)),
        interaction_(std::move(interaction)),
        methodNameInfo_(std::move(methodNameInfo)),
        ewp_(ewp),
        eb_(eb),
        executor_(
            tm ? tm->getKeepAlive(
                     reqCtx->getRequestExecutionScope(),
                     apache::thrift::concurrency::ThreadManager::Source::
                         INTERNAL)
               : nullptr),
        reqCtx_(reqCtx),
        protoSeqId_(0) {}

  HandlerCallbackBase(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      exnw_ptr ewp,
      folly::EventBase* eb,
      folly::Executor::KeepAlive<> executor,
      Cpp2RequestContext* reqCtx,
      RequestCompletionCallback* notifyRequestPile,
      RequestCompletionCallback* notifyConcurrencyController,
      ServerRequestData requestData,
      TilePtr&& interaction = {})
      : req_(std::move(req)),
        ctx_(std::move(ctx)),
        interaction_(std::move(interaction)),
        methodNameInfo_(std::move(methodNameInfo)),
        ewp_(ewp),
        eb_(eb),
        executor_(std::move(executor)),
        reqCtx_(reqCtx),
        protoSeqId_(0),
        notifyRequestPile_(notifyRequestPile),
        notifyConcurrencyController_(notifyConcurrencyController),
        requestData_(std::move(requestData)) {}

  virtual ~HandlerCallbackBase();

  static void releaseRequest(
      ResponseChannelRequest::UniquePtr request,
      folly::EventBase* eb,
      TilePtr interaction = {});

  void exception(std::exception_ptr ex) {
    class ExceptionHandler {
     public:
      explicit ExceptionHandler(std::exception_ptr&& ex) : ex_(std::move(ex)) {}

      folly::exception_wrapper exception() && {
        return folly::exception_wrapper(std::move(ex_));
      }

      static void handle(
          HandlerCallbackBase& callback, folly::exception_wrapper&& ew) {
        callback.doException(ew.to_exception_ptr());
      }

     private:
      std::exception_ptr ex_;
    };
    handleExceptionAndExecuteServiceInterceptors(
        ExceptionHandler(std::move(ex)));
  }

  void exception(folly::exception_wrapper ew) {
    class ExceptionHandler {
     public:
      explicit ExceptionHandler(folly::exception_wrapper&& ew)
          : ew_(std::move(ew)) {}

      folly::exception_wrapper exception() && { return std::move(ew_); }

      static void handle(
          HandlerCallbackBase& callback, folly::exception_wrapper&& ew) {
        callback.doExceptionWrapped(std::move(ew));
      }

     private:
      folly::exception_wrapper ew_;
    };
    handleExceptionAndExecuteServiceInterceptors(
        ExceptionHandler(std::move(ew)));
  }

  // Warning: just like "throw ex", this captures the STATIC type of ex, not
  // the dynamic type.  If you need the dynamic type, then either you should
  // be using exception_wrapper instead of a reference to a base exception
  // type, or your exception hierarchy should be equipped to throw
  // polymorphically, see //
  // http://www.parashift.com/c++-faq/throwing-polymorphically.html
  template <class Exception>
  void exception(const Exception& ex) {
    exception(folly::make_exception_wrapper<Exception>(ex));
  }

  void appOverloadedException(const std::string& message) {
    exception(TrustedServerException::appOverloadError(message));
  }

  folly::EventBase* getEventBase();

  [[deprecated("use getHandlerExecutor()")]] folly::Executor*
  getThreadManager();

  [[deprecated("use getHandlerExecutor()")]] concurrency::ThreadManager*
  getThreadManager_deprecated();

  bool isResourcePoolEnabled();

  folly::Executor* getHandlerExecutor();

  [[deprecated("Replaced by getRequestContext")]] Cpp2RequestContext*
  getConnectionContext() {
    return reqCtx_;
  }

  Cpp2RequestContext* getRequestContext() { return reqCtx_; }

  bool isRequestActive() {
    // If req_ is nullptr probably it is not managed by this HandlerCallback
    // object and we just return true. An example can be found in task 3106731
    return !req_ || req_->isActive();
  }

  ResponseChannelRequest* getRequest() { return req_.get(); }

  const folly::Executor::KeepAlive<>& getInternalKeepAlive();

 protected:
  folly::Optional<uint32_t> checksumIfNeeded(
      LegacySerializedResponse& response);

  folly::Optional<uint32_t> checksumIfNeeded(SerializedResponse& response);

  virtual ResponsePayload transform(ResponsePayload&& response);

#if FOLLY_HAS_COROUTINES
  template <class T>
  void startOnExecutor(folly::coro::Task<T>&& task) {
    folly::Executor::KeepAlive<> executor =
        executor_ ? executor_ : folly::getKeepAliveToken(eb_);
    if (executor.get() == eb_ && eb_->isInEventBaseThread()) {
      // Avoid rescheduling in the common case where result() is called inline
      // on the EB thread where request execution began
      co_withExecutor(std::move(executor), std::move(task)).startInlineUnsafe();
    } else {
      co_withExecutor(std::move(executor), std::move(task)).start();
    }
  }

  template <class ExceptionHandler>
  static folly::coro::Task<void>
  doInvokeServiceInterceptorsOnResponseWithException(
      Ptr callback, ExceptionHandler handler) {
    folly::exception_wrapper ew = std::move(handler).exception();
    folly::Try<void> onResponseResult = co_await folly::coro::co_awaitTry(
        callback->processServiceInterceptorsOnResponse(ew));
    // When both user code and ServiceInterceptor::onResponse have exceptions,
    // the ServiceInterceptor wins. This is because
    // ServiceInterceptor::onResponse has access to the user-thrown exception
    // and can choose to either swallow it or not.
    if (onResponseResult.hasException()) {
      ExceptionHandler::handle(
          *callback, std::move(onResponseResult).exception());
    } else {
      ExceptionHandler::handle(*callback, std::move(ew));
    }
  }
#endif // FOLLY_HAS_COROUTINES

  template <class ExceptionHandler>
  void handleExceptionAndExecuteServiceInterceptors(ExceptionHandler handler) {
#if FOLLY_HAS_COROUTINES
    if (!shouldProcessServiceInterceptorsOnResponse()) {
      ExceptionHandler::handle(*this, std::move(handler).exception());
      return;
    }
    startOnExecutor(doInvokeServiceInterceptorsOnResponseWithException(
        sharedFromThis(), std::move(handler)));
#else
    ExceptionHandler::handle(*this, std::move(handler).exception());
#endif // FOLLY_HAS_COROUTINES
  }

  // Can be called from IO or TM thread
  virtual void doException(std::exception_ptr ex) {
    doExceptionWrapped(folly::exception_wrapper(ex));
  }

  virtual void doExceptionWrapped(folly::exception_wrapper ew);

  template <typename F, typename T>
  void callExceptionInEventBaseThread(F&& f, T&& ex);

  template <typename Reply, typename... A>
  void putMessageInReplyQueue(std::in_place_type_t<Reply> tag, A&&... a);

  void sendReply(SerializedResponse response);
  void sendReply(ResponseAndServerStreamFactory&& responseAndStream);

  bool fulfillTilePromise(std::unique_ptr<Tile> ptr);
  void breakTilePromise();

#if FOLLY_HAS_COROUTINES
  bool shouldProcessServiceInterceptorsOnRequest() const noexcept;
  friend bool detail::shouldProcessServiceInterceptorsOnRequest(
      HandlerCallbackBase&) noexcept;

  bool shouldProcessServiceInterceptorsOnResponse() const noexcept;

  folly::coro::Task<void> processServiceInterceptorsOnRequest(
      detail::ServiceInterceptorOnRequestArguments arguments);
  folly::coro::Task<void> processServiceInterceptorsOnResponse(
      detail::ServiceInterceptorOnResponseResult resultOrActiveException);

  friend folly::coro::Task<void> detail::processServiceInterceptorsOnRequest(
      HandlerCallbackBase&,
      detail::ServiceInterceptorOnRequestArguments arguments);
#endif // FOLLY_HAS_COROUTINES

#if !FOLLY_HAS_COROUTINES
  [[noreturn]]
#endif
  void sendReply(
      [[maybe_unused]] std::pair<
          apache::thrift::SerializedResponse,
          apache::thrift::detail::SinkConsumerImpl>&& responseAndSinkConsumer);

  // Required for this call
  ResponseChannelRequest::UniquePtr req_;
  ContextStack::UniquePtr ctx_;
  TilePtr interaction_;

  MethodNameInfo methodNameInfo_;

  // May be null in a oneway call
  exnw_ptr ewp_;

  // Useful pointers, so handler doesn't need to have a pointer to the server
  folly::EventBase* eb_{nullptr};
  folly::Executor::KeepAlive<> executor_{};
  Cpp2RequestContext* reqCtx_{nullptr};

  int32_t protoSeqId_;

  RequestCompletionCallback* notifyRequestPile_{nullptr};
  RequestCompletionCallback* notifyConcurrencyController_{nullptr};
  ServerRequestData requestData_;
};

class HandlerCallbackOneWay : public HandlerCallbackBase {
 public:
  using Ptr =
      util::IntrusiveSharedPtr<HandlerCallbackOneWay, IntrusiveSharedPtrAccess>;
  using HandlerCallbackBase::HandlerCallbackBase;

 private:
#if FOLLY_HAS_COROUTINES
  static folly::coro::Task<void> doInvokeServiceInterceptorsOnResponse(
      Ptr callback);
#endif // FOLLY_HAS_COROUTINES

  Ptr sharedFromThis() noexcept {
    // Constructing from raw pointer is safe in this case because
    // `this` is guaranteed to be alive while the current
    // function is executing.
    return Ptr(typename Ptr::UnsafelyFromRawPointer(), this);
  }

 public:
  void done() noexcept;
  void complete(folly::Try<folly::Unit>&& r) noexcept;

  class CompletionGuard {
   public:
    explicit CompletionGuard(Ptr&& callback) noexcept
        : callback_(std::move(callback)) {}
    CompletionGuard(CompletionGuard&& other) noexcept
        : callback_(other.release()) {}
    CompletionGuard& operator=(CompletionGuard&& other) noexcept {
      callback_ = other.release();
      return *this;
    }

    ~CompletionGuard() noexcept {
      if (callback_ == nullptr) {
        return;
      }
      if (auto ex = folly::current_exception()) {
        callback_->exception(std::move(ex));
      } else {
        callback_->done();
      }
    }

    Ptr release() noexcept { return std::exchange(callback_, nullptr); }

   private:
    Ptr callback_;
  };
};

template <class T>
using HandlerCallbackPtr = util::IntrusiveSharedPtr<
    HandlerCallback<T>,
    HandlerCallbackBase::IntrusiveSharedPtrAccess>;

template <typename T>
class HandlerCallback : public HandlerCallbackBase {
  using Helper = apache::thrift::detail::HandlerCallbackHelper<T>;
  using InnerType = typename Helper::InnerType;
  using InputType = typename Helper::InputType;
  using cob_ptr = typename Helper::CobPtr;

 public:
  using Ptr = HandlerCallbackPtr<T>;
  using ResultType = std::decay_t<typename Helper::InputType>;

 private:
  Ptr sharedFromThis() {
    // Constructing from raw pointer is safe in this case because
    // `this` is guaranteed to be alive while the current
    // function is executing.
    return Ptr(typename Ptr::UnsafelyFromRawPointer(), this);
  }

 public:
  HandlerCallback() : cp_(nullptr) {}

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx,
      TilePtr&& interaction = {});

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      folly::Executor::KeepAlive<> executor,
      Cpp2RequestContext* reqCtx,
      RequestCompletionCallback* notifyRequestPile,
      RequestCompletionCallback* notifyConcurrencyController,
      ServerRequestData requestData,
      TilePtr&& interaction = {});

#if FOLLY_HAS_COROUTINES
  static folly::coro::Task<void> doInvokeServiceInterceptorsOnResponse(
      Ptr callback, std::decay_t<InputType> result) {
    folly::Try<void> onResponseResult = co_await folly::coro::co_awaitTry(
        callback->processServiceInterceptorsOnResponse(
            apache::thrift::util::TypeErasedRef::of<InnerType>(result)));
    if (onResponseResult.hasException()) {
      callback->doException(onResponseResult.exception().to_exception_ptr());
    } else {
      callback->doResult(std::move(result));
    }
  }
#endif

  void result(InnerType r) {
#if FOLLY_HAS_COROUTINES
    if (!shouldProcessServiceInterceptorsOnResponse()) {
      // Some service code (especially unit tests) assume that doResult() is
      // called synchronously within a result() call. This check exists simply
      // for backwards compatibility with those services. As an added bonus, we
      // get to avoid allocating a coroutine frame + Future core in the case
      // where they will be unused.
      doResult(std::forward<InputType>(r));
    } else {
      startOnExecutor(doInvokeServiceInterceptorsOnResponse(
          sharedFromThis(), std::decay_t<InputType>(std::move(r))));
    }
#else
    doResult(std::forward<InputType>(r));
#endif // FOLLY_HAS_COROUTINES
  }
  [[deprecated("Pass the inner value directly to result()")]] void result(
      std::unique_ptr<ResultType> r);

  void complete(folly::Try<T>&& r);

 protected:
  virtual void doResult(InputType r);

  cob_ptr cp_;
};

template <>
class HandlerCallback<void> : public HandlerCallbackBase {
  using cob_ptr = SerializedResponse (*)(ContextStack*);

 public:
  using Ptr = HandlerCallbackPtr<void>;
  using ResultType = void;

 private:
  Ptr sharedFromThis() {
    // Constructing from raw pointer is safe in this case because
    // `this` is guaranteed to be alive while the current
    // function is executing.
    return Ptr(typename Ptr::UnsafelyFromRawPointer(), this);
  }

 public:
  HandlerCallback() : cp_(nullptr) {}

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx,
      TilePtr&& interaction = {});

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      ContextStack::UniquePtr ctx,
      MethodNameInfo methodNameInfo,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      folly::Executor::KeepAlive<> executor,
      Cpp2RequestContext* reqCtx,
      RequestCompletionCallback* notifyRequestPile,
      RequestCompletionCallback* notifyConcurrencyController,
      ServerRequestData requestData,
      TilePtr&& interaction = {});

#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> doInvokeServiceInterceptorsOnResponse(Ptr callback) {
    folly::Try<void> onResponseResult = co_await folly::coro::co_awaitTry(
        callback->processServiceInterceptorsOnResponse(
            apache::thrift::util::TypeErasedRef::of<folly::Unit>(folly::unit)));
    if (onResponseResult.hasException()) {
      callback->doException(onResponseResult.exception().to_exception_ptr());
    } else {
      callback->doDone();
    }
  }
#endif // FOLLY_HAS_COROUTINES

  void done() {
#if FOLLY_HAS_COROUTINES
    if (!shouldProcessServiceInterceptorsOnResponse()) {
      // Some service code (especially unit tests) assume that doResult() is
      // called synchronously within a result() call. This check exists simply
      // for backwards compatibility with those services. As an added bonus, we
      // get to avoid allocating a coroutine frame + Future core in the case
      // where they will be unused.
      doDone();
    } else {
      startOnExecutor(doInvokeServiceInterceptorsOnResponse(sharedFromThis()));
    }
#else
    doDone();
#endif // FOLLY_HAS_COROUTINES
  }

  void complete(folly::Try<folly::Unit>&& r);

 protected:
  virtual void doDone();

  cob_ptr cp_;
};

template <typename InteractionIf, typename Response>
struct TileAndResponse {
  std::unique_ptr<InteractionIf> tile;
  Response response;
};
template <typename InteractionIf>
struct TileAndResponse<InteractionIf, void> {
  std::unique_ptr<InteractionIf> tile;
};

template <typename InteractionIf, typename Response>
class HandlerCallback<TileAndResponse<InteractionIf, Response>> final
    : public HandlerCallback<Response> {
 public:
  using Ptr = HandlerCallbackPtr<TileAndResponse<InteractionIf, Response>>;

  void result(TileAndResponse<InteractionIf, Response>&& r) {
    if (this->fulfillTilePromise(std::move(r.tile))) {
      if constexpr (!std::is_void_v<Response>) {
        HandlerCallback<Response>::result(std::move(r.response));
      } else {
        this->done();
      }
    }
  }
  void complete(folly::Try<TileAndResponse<InteractionIf, Response>>&& r) {
    if (r.hasException()) {
      this->exception(std::move(r.exception()));
    } else {
      this->result(std::move(r.value()));
    }
  }

  using HandlerCallback<Response>::HandlerCallback;

  ~HandlerCallback() override {
    if (this->interaction_) {
      this->breakTilePromise();
    }
  }
};

////
// Implementation details
////

template <typename F, typename T>
void HandlerCallbackBase::callExceptionInEventBaseThread(F&& f, T&& ex) {
  if (!f) {
    return;
  }
  if (!getEventBase() || getEventBase()->inRunningEventBaseThread()) {
    f(std::exchange(req_, {}), protoSeqId_, ctx_.get(), ex, reqCtx_);
    ctx_.reset();
  } else {
    getEventBase()->runInEventBaseThread([f = std::forward<F>(f),
                                          req = std::move(req_),
                                          protoSeqId = protoSeqId_,
                                          ctx = std::move(ctx_),
                                          ex = std::forward<T>(ex),
                                          reqCtx = reqCtx_,
                                          eb = getEventBase()]() mutable {
      f(std::move(req), protoSeqId, ctx.get(), ex, reqCtx);
    });
  }
}

template <typename Reply, typename... A>
void HandlerCallbackBase::putMessageInReplyQueue(
    std::in_place_type_t<Reply> tag, A&&... a) {
  auto eb = getEventBase();
  if (!eb) {
    Reply(std::forward<A>(a)...)();
    return;
  }
  if constexpr (folly::kIsWindows) {
    // TODO(T88449658): We are seeing performance regression on Windows if we
    // use the reply queue. The exact cause is under investigation. Before it is
    // fixed, we can use the default EventBase queue on Windows for now.
    eb->runInEventBaseThread(
        [reply = Reply(static_cast<A&&>(a)...)]() mutable { reply(); });
  } else {
    getReplyQueue().putMessage(tag, static_cast<A&&>(a)...);
  }
}

template <typename T>
HandlerCallback<T>::HandlerCallback(
    ResponseChannelRequest::UniquePtr req,
    ContextStack::UniquePtr ctx,
    MethodNameInfo methodNameInfo,
    cob_ptr cp,
    exnw_ptr ewp,
    int32_t protoSeqId,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx,
    TilePtr&& interaction)
    : HandlerCallbackBase(
          std::move(req),
          std::move(ctx),
          std::move(methodNameInfo),
          ewp,
          eb,
          tm,
          reqCtx,
          std::move(interaction)),
      cp_(cp) {
  this->protoSeqId_ = protoSeqId;
}

template <typename T>
HandlerCallback<T>::HandlerCallback(
    ResponseChannelRequest::UniquePtr req,
    ContextStack::UniquePtr ctx,
    MethodNameInfo methodNameInfo,
    cob_ptr cp,
    exnw_ptr ewp,
    int32_t protoSeqId,
    folly::EventBase* eb,
    folly::Executor::KeepAlive<> executor,
    Cpp2RequestContext* reqCtx,
    RequestCompletionCallback* notifyRequestPile,
    RequestCompletionCallback* notifyConcurrencyController,
    ServerRequestData requestData,
    TilePtr&& interaction)
    : HandlerCallbackBase(
          std::move(req),
          std::move(ctx),
          std::move(methodNameInfo),
          ewp,
          eb,
          std::move(executor),
          reqCtx,
          notifyRequestPile,
          notifyConcurrencyController,
          std::move(requestData),
          std::move(interaction)),
      cp_(cp) {
  this->protoSeqId_ = protoSeqId;
}

template <typename T>
void HandlerCallback<T>::result(std::unique_ptr<ResultType> r) {
  r ? result(std::move(*r))
    : exception(TApplicationException(
          TApplicationException::MISSING_RESULT,
          "nullptr yielded from handler"));
}

template <typename T>
void HandlerCallback<T>::complete(folly::Try<T>&& r) {
  maybeNotifyComplete();
  if (r.hasException()) {
    exception(std::move(r.exception()));
  } else {
    result(std::move(r.value()));
  }
}

template <typename T>
void HandlerCallback<T>::doResult(InputType r) {
  assert(cp_ != nullptr);
  auto reply = Helper::call(
      cp_,
      this->ctx_.get(),
      executor_ ? executor_.get() : eb_,
      std::forward<InputType>(r));
  sendReply(std::move(reply));
}

namespace detail {

// template that typedefs type to its argument, unless the argument is a
// unique_ptr<S>, in which case it typedefs type to S.
template <class S>
struct inner_type {
  using type = S;
};
template <class S>
struct inner_type<std::unique_ptr<S>> {
  using type = S;
};

template <typename T>
struct HandlerCallbackHelper {
  using InnerType = typename apache::thrift::detail::inner_type<T>::type;
  using InputType = const InnerType&;
  using CobPtr =
      apache::thrift::SerializedResponse (*)(ContextStack*, InputType);
  static apache::thrift::SerializedResponse call(
      CobPtr cob, ContextStack* ctx, folly::Executor*, InputType input) {
    return cob(ctx, input);
  }
};

template <typename StreamInputType>
struct HandlerCallbackHelperServerStream {
  using InnerType = StreamInputType&&;
  using InputType = StreamInputType&&;
  using CobPtr = ResponseAndServerStreamFactory (*)(
      ContextStack*, folly::Executor::KeepAlive<>, InputType);
  static ResponseAndServerStreamFactory call(
      CobPtr cob, ContextStack* ctx, folly::Executor* ex, InputType input) {
    return cob(ctx, ex, std::move(input));
  }
};

template <typename Response, typename StreamItem>
struct HandlerCallbackHelper<ResponseAndServerStream<Response, StreamItem>>
    : public HandlerCallbackHelperServerStream<
          ResponseAndServerStream<Response, StreamItem>> {};

template <typename StreamItem>
struct HandlerCallbackHelper<ServerStream<StreamItem>>
    : public HandlerCallbackHelperServerStream<ServerStream<StreamItem>> {};

template <typename SinkInputType>
struct HandlerCallbackHelperSink {
  using InnerType = SinkInputType&&;
  using InputType = SinkInputType&&;
  using CobPtr =
      std::pair<apache::thrift::SerializedResponse, SinkConsumerImpl> (*)(
          ContextStack*, InputType, folly::Executor::KeepAlive<>);
  static std::pair<apache::thrift::SerializedResponse, SinkConsumerImpl> call(
      CobPtr cob, ContextStack* ctx, folly::Executor* ex, InputType input) {
    return cob(ctx, std::move(input), ex);
  }
};

template <typename Response, typename SinkElement, typename FinalResponse>
struct HandlerCallbackHelper<
    ResponseAndSinkConsumer<Response, SinkElement, FinalResponse>>
    : public HandlerCallbackHelperSink<
          ResponseAndSinkConsumer<Response, SinkElement, FinalResponse>> {};

template <typename SinkElement, typename FinalResponse>
struct HandlerCallbackHelper<SinkConsumer<SinkElement, FinalResponse>>
    : public HandlerCallbackHelperSink<
          SinkConsumer<SinkElement, FinalResponse>> {};

} // namespace detail

template <typename ChildType>
void RequestTask<ChildType>::run() {
  // Since this request was queued, reset the processBegin
  // time to the actual start time, and not the queue time.
  req_.requestContext()->getTimestamps().processBegin =
      std::chrono::steady_clock::now();
  if (!oneway_ && !req_.request()->getShouldStartProcessing()) {
    apache::thrift::HandlerCallbackBase::releaseRequest(
        apache::thrift::detail::ServerRequestHelper::request(std::move(req_)),
        apache::thrift::detail::ServerRequestHelper::eventBase(req_));
    return;
  }
  (childClass_->*executeFunc_)(std::move(req_));
}

} // namespace apache::thrift
