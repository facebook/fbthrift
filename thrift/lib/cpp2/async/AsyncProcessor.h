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

#pragma once

#include <folly/Portability.h>

#include <folly/ExceptionWrapper.h>
#include <folly/String.h>
#include <folly/container/F14Map.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/SerializationSwitch.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache {
namespace thrift {

namespace detail {
template <typename T>
struct HandlerCallbackHelper;
}

class EventTask : public virtual concurrency::Runnable {
 public:
  EventTask(
      folly::Function<void(ResponseChannelRequest::UniquePtr)>&& taskFunc,
      ResponseChannelRequest::UniquePtr req,
      folly::EventBase* base,
      bool oneway)
      : taskFunc_(std::move(taskFunc)),
        req_(std::move(req)),
        base_(base),
        oneway_(oneway) {}

  ~EventTask() override;

  void run() override;
  void expired();

 private:
  folly::Function<void(ResponseChannelRequest::UniquePtr)> taskFunc_;
  ResponseChannelRequest::UniquePtr req_;
  folly::EventBase* base_;
  bool oneway_;
};

class PriorityEventTask : public concurrency::PriorityRunnable,
                          public EventTask {
 public:
  PriorityEventTask(
      concurrency::PriorityThreadManager::PRIORITY priority,
      folly::Function<void(ResponseChannelRequest::UniquePtr)>&& taskFunc,
      ResponseChannelRequest::UniquePtr req,
      folly::EventBase* base,
      bool oneway)
      : EventTask(std::move(taskFunc), std::move(req), base, oneway),
        priority_(priority) {}

  concurrency::PriorityThreadManager::PRIORITY getPriority() const override {
    return priority_;
  }
  using EventTask::run;

 private:
  concurrency::PriorityThreadManager::PRIORITY priority_;
};

class AsyncProcessor : public TProcessorBase {
 public:
  virtual ~AsyncProcessor() = default;

  virtual void processSerializedRequest(
      ResponseChannelRequest::UniquePtr,
      SerializedRequest&&,
      protocol::PROTOCOL_TYPES,
      Cpp2RequestContext*,
      folly::EventBase*,
      concurrency::ThreadManager*) = 0;

  virtual std::shared_ptr<folly::RequestContext> getBaseContextForRequest() {
    return nullptr;
  }

  virtual void getServiceMetadata(metadata::ThriftServiceMetadataResponse&) {}
};

class GeneratedAsyncProcessor : public AsyncProcessor {
 public:
  virtual const char* getServiceName() = 0;

  template <typename Derived>
  using ProcessFunc = void (Derived::*)(
      ResponseChannelRequest::UniquePtr,
      SerializedRequest&&,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm);
  template <typename ProcessFunc>
  using ProcessMap = folly::F14ValueMap<std::string, ProcessFunc>;

 protected:
  template <typename ProtocolIn, typename Args>
  static void deserializeRequest(
      Args& args,
      folly::StringPiece methodName,
      const SerializedRequest& serializedRequest,
      ContextStack* c);

  template <typename ProtocolOut, typename Result>
  static folly::IOBufQueue serializeResponse(
      const char* method,
      ProtocolOut* prot,
      int32_t protoSeqId,
      ContextStack* ctx,
      const Result& result);

  static bool validateRpcKind(
      ResponseChannelRequest::UniquePtr& req,
      RpcKind kind);

  template <typename ProtocolIn_, typename ProtocolOut_, typename ChildType>
  static void processInThread(
      ResponseChannelRequest::UniquePtr req,
      SerializedRequest&& serializedRequest,
      Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      concurrency::PRIORITY pri,
      RpcKind kind,
      ProcessFunc<ChildType> processFunc,
      ChildType* childClass);
};

class AsyncProcessorFactory {
 public:
  virtual std::unique_ptr<AsyncProcessor> getProcessor() = 0;
  virtual ~AsyncProcessorFactory() = default;
};

/**
 * This struct encapsulates the various thrift control information of interest
 * to request handlers; the executor on which we expect them to execute, the
 * Cpp2RequestContext of the incoming request struct, etc.
 */
class RequestParams {
 public:
  RequestParams(
      Cpp2RequestContext* requestContext,
      concurrency::ThreadManager* threadManager,
      folly::EventBase* eventBase)
      : requestContext_(requestContext),
        threadManager_(threadManager),
        eventBase_(eventBase) {}
  RequestParams() : RequestParams(nullptr, nullptr, nullptr) {}
  RequestParams(const RequestParams&) = default;
  RequestParams& operator=(const RequestParams&) = default;

  Cpp2RequestContext* getRequestContext() const {
    return requestContext_;
  }
  concurrency::ThreadManager* getThreadManager() const {
    return threadManager_;
  }
  folly::EventBase* getEventBase() const {
    return eventBase_;
  }

 private:
  friend class ServerInterface;

  Cpp2RequestContext* requestContext_;
  concurrency::ThreadManager* threadManager_;
  folly::EventBase* eventBase_;
};

class ServerInterface : public AsyncProcessorFactory {
 public:
  Cpp2RequestContext* getConnectionContext() const {
    return requestParams_.requestContext_;
  }

  void setConnectionContext(Cpp2RequestContext* c) {
    requestParams_.requestContext_ = c;
  }

  void setThreadManager(concurrency::ThreadManager* tm) {
    requestParams_.threadManager_ = tm;
  }

  concurrency::ThreadManager* getThreadManager() {
    return requestParams_.threadManager_;
  }

  folly::Executor::KeepAlive<> getBlockingThreadManager() {
    return BlockingThreadManager::create(requestParams_.threadManager_);
  }

  static folly::Executor::KeepAlive<> getBlockingThreadManager(
      concurrency::ThreadManager* threadManager) {
    return BlockingThreadManager::create(threadManager);
  }

  void setEventBase(folly::EventBase* eb);

  folly::EventBase* getEventBase() {
    return requestParams_.eventBase_;
  }

  /**
   * Override to return a pre-initialized RequestContext.
   * Its content will be copied in the RequestContext initialized at
   * the beginning of each thrift request processing.
   */
  virtual std::shared_ptr<folly::RequestContext> getBaseContextForRequest() {
    return nullptr;
  }

  virtual concurrency::PRIORITY getRequestPriority(
      Cpp2RequestContext* ctx,
      concurrency::PRIORITY prio);
  concurrency::PRIORITY getRequestPriority(Cpp2RequestContext* ctx) {
    return getRequestPriority(ctx, concurrency::NORMAL);
  }

 private:
  class BlockingThreadManager : public folly::Executor {
   public:
    static folly::Executor::KeepAlive<> create(
        concurrency::ThreadManager* executor) {
      return makeKeepAlive(new BlockingThreadManager(executor));
    }
    void add(folly::Func f) override;

   private:
    explicit BlockingThreadManager(concurrency::ThreadManager* executor)
        : executor_(folly::getKeepAliveToken(executor)) {}

    bool keepAliveAcquire() noexcept override;
    void keepAliveRelease() noexcept override;

    static constexpr std::chrono::seconds kTimeout{30};
    std::atomic<size_t> keepAliveCount_{1};
    folly::Executor::KeepAlive<concurrency::ThreadManager> executor_;
  };

  /**
   * This variable is only used for sync calls when in a threadpool it
   * is threadlocal, because the threadpool will probably be
   * processing multiple requests simultaneously, and we don't want to
   * mix up the connection contexts.
   *
   * This threadlocal trick doesn't work for async requests, because
   * multiple async calls can be running on the same thread.  Instead,
   * use the callback->getConnectionContext() method.  This reqCtx_
   * will be NULL for async calls.
   */
  static thread_local RequestParams requestParams_;
};

/**
 * HandlerCallback class for async callbacks.
 *
 * These are constructed by the generated code, and your handler calls
 * either result(value), done(), exception(ex), or appOverloadedException() to
 * finish the async call.  Only one of these must be called, otherwise your
 * client will likely get confused with multiple response messages.
 */
class HandlerCallbackBase {
 protected:
  using exnw_ptr = void (*)(
      ResponseChannelRequest::UniquePtr,
      int32_t protoSeqId,
      ContextStack*,
      folly::exception_wrapper,
      Cpp2RequestContext*);

 public:
  HandlerCallbackBase()
      : eb_(nullptr), tm_(nullptr), reqCtx_(nullptr), protoSeqId_(0) {}

  HandlerCallbackBase(
      ResponseChannelRequest::UniquePtr req,
      std::unique_ptr<ContextStack> ctx,
      exnw_ptr ewp,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx)
      : req_(std::move(req)),
        ctx_(std::move(ctx)),
        ewp_(ewp),
        eb_(eb),
        tm_(tm),
        reqCtx_(reqCtx),
        protoSeqId_(0) {}

  virtual ~HandlerCallbackBase();

  void exception(std::exception_ptr ex) {
    doException(ex);
  }

  void exception(folly::exception_wrapper ew) {
    doExceptionWrapped(ew);
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
    doAppOverloadedException(message);
  }

  folly::EventBase* getEventBase();

  concurrency::ThreadManager* getThreadManager();

  Cpp2RequestContext* getConnectionContext() {
    return reqCtx_;
  }

  bool isRequestActive() {
    // If req_ is nullptr probably it is not managed by this HandlerCallback
    // object and we just return true. An example can be found in task 3106731
    return !req_ || req_->isActive();
  }

  ResponseChannelRequest* getRequest() {
    return req_.get();
  }

  template <class F>
  void runFuncInQueue(F&& func, bool oneway = false);

 protected:
  // HACK(tudorb): Call this to set up forwarding to the event base and
  // thread manager of the other callback.  Use when you want to create
  // callback wrappers that forward to another callback (and do some
  // pre- / post-processing).
  void forward(const HandlerCallbackBase& other);

  folly::Optional<uint32_t> checksumIfNeeded(folly::IOBufQueue& queue);

  virtual void transform(folly::IOBufQueue& queue);

  // Can be called from IO or TM thread
  virtual void doException(std::exception_ptr ex) {
    doExceptionWrapped(folly::exception_wrapper::from_exception_ptr(ex));
  }

  virtual void doExceptionWrapped(folly::exception_wrapper ew);
  virtual void doAppOverloadedException(const std::string& message);

  template <typename F, typename T>
  void callExceptionInEventBaseThread(F&& f, T&& ex);

  void sendReply(folly::IOBufQueue queue);
  void sendReply(ResponseAndServerStreamFactory&& responseAndStream);

#if !FOLLY_HAS_COROUTINES
  [[noreturn]]
#endif
  void
  sendReply(FOLLY_MAYBE_UNUSED
                std::pair<folly::IOBufQueue, detail::SinkConsumerImpl>&&
                    responseAndSinkConsumer);

  // Required for this call
  ResponseChannelRequest::UniquePtr req_;
  std::unique_ptr<ContextStack> ctx_;

  // May be null in a oneway call
  exnw_ptr ewp_;

  // Useful pointers, so handler doesn't need to have a pointer to the server
  folly::EventBase* eb_;
  concurrency::ThreadManager* tm_;
  Cpp2RequestContext* reqCtx_;

  int32_t protoSeqId_;
};

template <typename T>
class HandlerCallback : public HandlerCallbackBase {
  using Helper = detail::HandlerCallbackHelper<T>;
  using InputType = typename Helper::InputType;
  using cob_ptr = typename Helper::CobPtr;

 public:
  using ResultType = std::decay_t<typename Helper::InputType>;

 public:
  HandlerCallback() : cp_(nullptr) {}

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      std::unique_ptr<ContextStack> ctx,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx);

  void result(InputType r) {
    doResult(std::forward<InputType>(r));
  }
  void result(std::unique_ptr<ResultType> r);

  void complete(folly::Try<T>&& r);

 protected:
  virtual void doResult(InputType r);

  cob_ptr cp_;
};

template <>
class HandlerCallback<void> : public HandlerCallbackBase {
  using cob_ptr = folly::IOBufQueue (*)(int32_t protoSeqId, ContextStack*);

 public:
  using ResultType = void;

  HandlerCallback() : cp_(nullptr) {}

  HandlerCallback(
      ResponseChannelRequest::UniquePtr req,
      std::unique_ptr<ContextStack> ctx,
      cob_ptr cp,
      exnw_ptr ewp,
      int32_t protoSeqId,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* reqCtx);

  void done() {
    doDone();
  }

  void complete(folly::Try<folly::Unit>&& r);

 protected:
  virtual void doDone();

  cob_ptr cp_;
};

////
// Implemenation details
////

template <typename ProtocolIn, typename Args>
void GeneratedAsyncProcessor::deserializeRequest(
    Args& args,
    folly::StringPiece methodName,
    const SerializedRequest& serializedRequest,
    ContextStack* c) {
  ProtocolIn iprot;
  iprot.setInput(serializedRequest.buffer.get());
  c->preRead();
  SerializedMessage smsg;
  smsg.protocolType = iprot.protocolType();
  smsg.buffer = serializedRequest.buffer.get();
  smsg.methodName = methodName;
  c->onReadData(smsg);
  uint32_t bytes = detail::deserializeRequestBody(&iprot, &args);
  iprot.readMessageEnd();
  c->postRead(nullptr, bytes);
}

template <typename ProtocolOut, typename Result>
folly::IOBufQueue GeneratedAsyncProcessor::serializeResponse(
    const char* method,
    ProtocolOut* prot,
    int32_t protoSeqId,
    ContextStack* ctx,
    const Result& result) {
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  size_t bufSize = detail::serializedResponseBodySizeZC(prot, &result);
  bufSize += prot->serializedMessageSize(method);

  // Preallocate small buffer headroom for transports metadata & framing.
  constexpr size_t kHeadroomBytes = 128;
  auto buf = folly::IOBuf::create(kHeadroomBytes + bufSize);
  buf->advance(kHeadroomBytes);
  queue.append(std::move(buf));

  prot->setOutput(&queue, bufSize);
  ctx->preWrite();
  prot->writeMessageBegin(method, T_REPLY, protoSeqId);
  detail::serializeResponseBody(prot, &result);
  prot->writeMessageEnd();
  SerializedMessage smsg;
  smsg.protocolType = prot->protocolType();
  smsg.buffer = queue.front();
  ctx->onWriteData(smsg);
  DCHECK_LE(queue.chainLength(), std::numeric_limits<int>::max());
  ctx->postWrite(folly::to_narrow(queue.chainLength()));
  return queue;
}

template <typename ProtocolIn_, typename ProtocolOut_, typename ChildType>
void GeneratedAsyncProcessor::processInThread(
    ResponseChannelRequest::UniquePtr req,
    SerializedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    concurrency::PRIORITY pri,
    RpcKind kind,
    ProcessFunc<ChildType> processFunc,
    ChildType* childClass) {
  if (!validateRpcKind(req, kind)) {
    return;
  }
  tm->add(
      std::make_shared<PriorityEventTask>(
          pri,
          [=, serializedRequest = std::move(serializedRequest)](
              ResponseChannelRequest::UniquePtr rq) mutable {
            if (ctx->getTimestamps().getSamplingStatus().isEnabled()) {
              // Since this request was queued, reset the processBegin
              // time to the actual start time, and not the queue time.
              ctx->getTimestamps().processBegin =
                  std::chrono::steady_clock::now();
            }
            // Oneway request won't be canceled if expired. see
            // D1006482 for furhter details.  TODO: fix this
            if (kind != RpcKind::SINGLE_REQUEST_NO_RESPONSE) {
              if (!rq->isActive()) {
                eb->runInEventBaseThread(
                    [rq = std::move(rq)]() mutable { rq.reset(); });
                return;
              }
            }
            (childClass->*processFunc)(
                std::move(rq), std::move(serializedRequest), ctx, eb, tm);
          },
          std::move(req),
          eb,
          kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE),
      0, // timeout
      0, // expiration
      true); // upstream
}

template <class F>
void HandlerCallbackBase::runFuncInQueue(F&& func, bool) {
  assert(tm_ != nullptr);
  assert(getEventBase()->isInEventBaseThread());
  tm_->add(
      concurrency::FunctionRunner::create(std::forward<F>(func)),
      0, // timeout
      0, // expiration
      true); // upstream
}

template <typename F, typename T>
void HandlerCallbackBase::callExceptionInEventBaseThread(F&& f, T&& ex) {
  if (!f) {
    return;
  }
  if (getEventBase()->isInEventBaseThread()) {
    f(std::exchange(req_, {}), protoSeqId_, ctx_.get(), ex, reqCtx_);
    ctx_.reset();
  } else {
    getEventBase()->runInEventBaseThread([f = std::forward<F>(f),
                                          req = std::move(req_),
                                          protoSeqId = protoSeqId_,
                                          ctx = std::move(ctx_),
                                          ex = std::forward<T>(ex),
                                          reqCtx = reqCtx_]() mutable {
      f(std::move(req), protoSeqId, ctx.get(), ex, reqCtx);
    });
  }
}

template <typename T>
HandlerCallback<T>::HandlerCallback(
    ResponseChannelRequest::UniquePtr req,
    std::unique_ptr<ContextStack> ctx,
    cob_ptr cp,
    exnw_ptr ewp,
    int32_t protoSeqId,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx)
    : HandlerCallbackBase(std::move(req), std::move(ctx), ewp, eb, tm, reqCtx),
      cp_(cp) {
  this->protoSeqId_ = protoSeqId;
}

template <typename T>
void HandlerCallback<T>::result(std::unique_ptr<ResultType> r) {
  r ? doResult(std::move(*r))
    : exception(TApplicationException(
          TApplicationException::MISSING_RESULT,
          "nullptr yielded from handler"));
}

template <typename T>
void HandlerCallback<T>::complete(folly::Try<T>&& r) {
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
      this->protoSeqId_,
      this->ctx_.get(),
      this->tm_,
      std::forward<InputType>(r));
  this->ctx_.reset();
  sendReply(std::move(reply));
}

namespace detail {

// template that typedefs type to its argument, unless the argument is a
// unique_ptr<S>, in which case it typedefs type to S.
template <class S>
struct inner_type {
  typedef S type;
};
template <class S>
struct inner_type<std::unique_ptr<S>> {
  typedef S type;
};

template <typename T>
struct HandlerCallbackHelper {
  using InputType = const typename detail::inner_type<T>::type&;
  using CobPtr =
      folly::IOBufQueue (*)(int32_t protoSeqId, ContextStack*, InputType);
  static folly::IOBufQueue call(
      CobPtr cob,
      int32_t protoSeqId,
      ContextStack* ctx,
      concurrency::ThreadManager*,
      InputType input) {
    return cob(protoSeqId, ctx, std::move(input));
  }
};

template <typename StreamInputType>
struct HandlerCallbackHelperServerStream {
  using InputType = StreamInputType;
  using CobPtr = ResponseAndServerStreamFactory (*)(
      int32_t protoSeqId,
      ContextStack*,
      folly::Executor::KeepAlive<>,
      InputType);
  static ResponseAndServerStreamFactory call(
      CobPtr cob,
      int32_t protoSeqId,
      ContextStack* ctx,
      concurrency::ThreadManager* tm,
      InputType input) {
    return cob(
        protoSeqId,
        ctx,
        ServerInterface::getBlockingThreadManager(tm),
        std::move(input));
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
  using InputType = SinkInputType;
  using CobPtr = std::pair<folly::IOBufQueue, SinkConsumerImpl> (*)(
      ContextStack*,
      InputType&&,
      folly::Executor::KeepAlive<>);
  static std::pair<folly::IOBufQueue, SinkConsumerImpl> call(
      CobPtr cob,
      int32_t,
      ContextStack* ctx,
      concurrency::ThreadManager* tm,
      InputType input) {
    return cob(
        ctx, std::move(input), ServerInterface::getBlockingThreadManager(tm));
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

} // namespace thrift
} // namespace apache
