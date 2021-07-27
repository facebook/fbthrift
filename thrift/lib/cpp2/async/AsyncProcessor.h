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

#include <string_view>
#include <variant>

#include <folly/ExceptionWrapper.h>
#include <folly/Portability.h>
#include <folly/String.h>
#include <folly/Unit.h>
#include <folly/container/F14Map.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/SerializationSwitch.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/Interaction.h>
#include <thrift/lib/cpp2/async/ReplyInfo.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/server/IOWorkerContext.h>
#include <thrift/lib/cpp2/util/Checksum.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache {
namespace thrift {

namespace detail {
template <typename T>
struct HandlerCallbackHelper;
}

class EventTask : public concurrency::Runnable, public InteractionTask {
 public:
  EventTask(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* ctx,
      bool oneway)
      : req_(std::move(req)),
        serializedRequest_(std::move(serializedRequest)),
        ctx_(ctx),
        eb_(eb),
        tm_(tm),
        oneway_(oneway) {}

  ~EventTask() override;

  void expired();
  void failWith(folly::exception_wrapper ex, std::string exCode) override;

  void setTile(TilePtr&& tile) override;

 protected:
  ResponseChannelRequest::UniquePtr req_;
  SerializedCompressedRequest serializedRequest_;
  Cpp2RequestContext* ctx_;
  folly::EventBase* eb_;
  concurrency::ThreadManager* tm_;
  bool oneway_;
};

class AsyncProcessor;
class ServiceHandler;

class AsyncProcessorFactory {
 public:
  virtual std::unique_ptr<AsyncProcessor> getProcessor() = 0;
  virtual std::vector<ServiceHandler*> getServiceHandlers() = 0;

  struct MethodMetadata {
    virtual ~MethodMetadata() = default;
  };
  /**
   * A map of method names to some loosely typed metadata that will be
   * passed to AsyncProcessor::processSerializedRequest. The concrete type of
   * the entries in the map is a contract between the AsyncProcessorFactory and
   * the AsyncProcessor returned by getProcessor.
   */
  using MethodMetadataMap =
      folly::F14FastMap<std::string, std::shared_ptr<const MethodMetadata>>;
  /**
   * A marker struct indicating that the AsyncProcessor supports any method, or
   * a list of methods that is not enumerable. This applies to AsyncProcessor
   * implementations such as proxies to external services.
   * The implementation may optionally enumerate a subset of known methods.
   */
  struct WildcardMethodMetadataMap {
    MethodMetadataMap knownMethods;
  };
  /**
   * The concrete metadata type that will be passed if createMethodMetadata
   * returns WildcardMethodMetadataMap and the current method is not in its
   * knownMethods.
   */
  struct WildcardMethodMetadata final : public MethodMetadata {};

  /**
   * The API is not implemented (legacy).
   */
  using MetadataNotImplemented = std::monostate;

  using CreateMethodMetadataResult = std::variant<
      MetadataNotImplemented,
      MethodMetadataMap,
      WildcardMethodMetadataMap>;
  /**
   * This function enumerates the list of methods supported by the
   * AsyncProcessor returned by getProcessor(), if possible. The return value
   * represents one of the following states:
   *   1. This API is not supported / implemented. (Will be removed soon)
   *   2. This API is supported and there is a static list of known methods.
   *      This applies to all generated AsyncProcessors.
   *   3. This API is supported but the complete set of methods is not known or
   *      is not enumerable (e.g. all method names supported). This applies, for
   *      example, to AsyncProcessors that proxy to external services.
   *
   * If returning (1), AsyncProcessor::processSerializedCompressedRequest MUST
   * be implemented instead of
   * AsyncProcessor::processSerializedCompressedRequestWithMetadata. Override
   * the latter only if returning (2) or (3). The metadata API is effectively
   * bypassed as a result.
   *
   * If returning (2), Thrift server will lookup the method metadata in the map.
   * If the method name is not found, a not-found error will be sent and
   * getProcessor will not be called. Any metadata passed to the processor will
   * always be a reference from the map.
   *
   * If returning (3), Thrift server will lookup the method metadata in the map.
   * If the method name is not found, Thrift will pass a WildcardMethodMetadata
   * object instead. Any metadata passed to the processor will always be a
   * reference from the map (or be WildcardMethodMetadata).
   */
  virtual CreateMethodMetadataResult createMethodMetadata() { return {}; }

  /**
   * Override to return a pre-initialized RequestContext.
   * Its content will be copied in the RequestContext initialized at
   * the beginning of each thrift request processing.
   *
   * The method metadata (per createMethodMetadata's contract) is also passed
   * in. If createMethodMetadata is unimplemented or the method is not found,
   * then this function will not be called.
   */
  virtual std::shared_ptr<folly::RequestContext> getBaseContextForRequest(
      const MethodMetadata&) {
    return nullptr;
  }

  virtual ~AsyncProcessorFactory() = default;
};

class AsyncProcessor : public TProcessorBase {
 public:
  virtual ~AsyncProcessor() = default;

  using MethodMetadata = AsyncProcessorFactory::MethodMetadata;

  virtual void processSerializedRequest(
      ResponseChannelRequest::UniquePtr,
      SerializedRequest&&,
      protocol::PROTOCOL_TYPES,
      Cpp2RequestContext*,
      folly::EventBase*,
      concurrency::ThreadManager*) = 0;

  virtual void processSerializedCompressedRequest(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      protocol::PROTOCOL_TYPES prot_type,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm);

  virtual void processSerializedCompressedRequestWithMetadata(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      const MethodMetadata& methodMetadata,
      protocol::PROTOCOL_TYPES prot_type,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm);

  virtual void getServiceMetadata(metadata::ThriftServiceMetadataResponse&) {}

  virtual void terminateInteraction(
      int64_t id, Cpp2ConnContext& conn, folly::EventBase&) noexcept;
  virtual void destroyAllInteractions(
      Cpp2ConnContext& conn, folly::EventBase&) noexcept;
};

class ServerInterface;

class GeneratedAsyncProcessor : public AsyncProcessor {
 public:
  virtual const char* getServiceName() = 0;

  template <typename Derived>
  using ProcessFunc = void (Derived::*)(
      ResponseChannelRequest::UniquePtr,
      SerializedCompressedRequest&&,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm);

  template <typename Derived>
  struct ProcessFuncs {
    ProcessFunc<Derived> compact;
    ProcessFunc<Derived> binary;
  };

  template <typename ProcessFuncs>
  using ProcessMap = folly::F14ValueMap<std::string, ProcessFuncs>;

  template <typename Derived>
  using InteractionConstructor = std::unique_ptr<Tile> (Derived::*)();
  template <typename InteractionConstructor>
  using InteractionConstructorMap =
      folly::F14ValueMap<std::string, InteractionConstructor>;

  void processSerializedRequest(
      ResponseChannelRequest::UniquePtr req,
      SerializedRequest&& serializedRequest,
      protocol::PROTOCOL_TYPES prot_type,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm) final {
    processSerializedCompressedRequest(
        std::move(req),
        SerializedCompressedRequest(std::move(serializedRequest)),
        prot_type,
        context,
        eb,
        tm);
  }

  // TODO(praihan): This function will be removed once we enforce that
  // createMethodMetadata is always implemented correctly.
  void processSerializedCompressedRequest(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      protocol::PROTOCOL_TYPES prot_type,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm) override = 0;

 protected:
  template <typename ProtocolIn, typename Args>
  static void deserializeRequest(
      Args& args,
      folly::StringPiece methodName,
      const SerializedRequest& serializedRequest,
      ContextStack* c);

  template <typename ProtocolOut, typename Result>
  static LegacySerializedResponse serializeLegacyResponse(
      const char* method,
      ProtocolOut* prot,
      int32_t protoSeqId,
      ContextStack* ctx,
      const Result& result);

  // Sends an error response if validation fails.
  static bool validateRpcKind(
      ResponseChannelRequest::UniquePtr& req, RpcKind kind);

  // Returns true if setup succeeded and sends an error response otherwise.
  // Always runs in eb thread.
  // tm is null if the method is annotated with thread='eb'.
  bool setUpRequestProcessing(
      ResponseChannelRequest::UniquePtr& req,
      Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      RpcKind kind,
      ServerInterface* si,
      folly::StringPiece interaction = "");

  template <typename ChildType>
  static void processInThread(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      RpcKind kind,
      ProcessFunc<ChildType> processFunc,
      ChildType* childClass);

 private:
  template <typename ChildType>
  static std::unique_ptr<concurrency::Runnable> makeEventTaskForRequest(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      RpcKind kind,
      ProcessFunc<ChildType> processFunc,
      ChildType* childClass,
      Tile* tile);

  // Returns false if interaction id is duplicated.
  bool createInteraction(
      ResponseChannelRequest::UniquePtr& req,
      int64_t id,
      std::string&& name,
      Cpp2RequestContext& ctx,
      concurrency::ThreadManager* tm,
      folly::EventBase& eb,
      ServerInterface* si);

 protected:
  virtual std::unique_ptr<Tile> createInteractionImpl(const std::string& name);

 public:
  void terminateInteraction(
      int64_t id, Cpp2ConnContext& conn, folly::EventBase&) noexcept final;
  void destroyAllInteractions(
      Cpp2ConnContext& conn, folly::EventBase&) noexcept final;
};

template <typename ChildType>
class RequestTask final : public EventTask {
 public:
  RequestTask(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm,
      Cpp2RequestContext* ctx,
      bool oneway,
      ChildType* childClass,
      GeneratedAsyncProcessor::ProcessFunc<ChildType> processFunc)
      : EventTask(
            std::move(req), std::move(serializedRequest), eb, tm, ctx, oneway),
        childClass_(childClass),
        processFunc_(processFunc) {}

  void run() override {
    if (ctx_->getTimestamps().getSamplingStatus().isEnabled()) {
      // Since this request was queued, reset the processBegin
      // time to the actual start time, and not the queue time.
      ctx_->getTimestamps().processBegin = std::chrono::steady_clock::now();
    }
    (childClass_->*processFunc_)(
        std::move(req_), std::move(serializedRequest_), ctx_, eb_, tm_);
  }

 private:
  ChildType* childClass_;
  GeneratedAsyncProcessor::ProcessFunc<ChildType> processFunc_;
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

  Cpp2RequestContext* getRequestContext() const { return requestContext_; }
  concurrency::ThreadManager* getThreadManager() const {
    return threadManager_;
  }
  folly::EventBase* getEventBase() const { return eventBase_; }

 private:
  friend class ServerInterface;

  Cpp2RequestContext* requestContext_;
  concurrency::ThreadManager* threadManager_;
  folly::EventBase* eventBase_;
};

class ServiceHandler {
 public:
  virtual folly::SemiFuture<folly::Unit> semifuture_onStartServing() = 0;
  virtual folly::SemiFuture<folly::Unit> semifuture_onStopServing() = 0;

  virtual ~ServiceHandler() = default;
};

class ServerInterface : public virtual AsyncProcessorFactory,
                        public ServiceHandler {
 public:
  ServerInterface() = default;
  ServerInterface(const ServerInterface&) = delete;
  ServerInterface& operator=(const ServerInterface&) = delete;

  [[deprecated("Replaced by getRequestContext")]] Cpp2RequestContext*
  getConnectionContext() const {
    return requestParams_.requestContext_;
  }

  Cpp2RequestContext* getRequestContext() const {
    return requestParams_.requestContext_;
  }

  [[deprecated("Replaced by setRequestContext")]] void setConnectionContext(
      Cpp2RequestContext* c) {
    requestParams_.requestContext_ = c;
  }

  void setRequestContext(Cpp2RequestContext* c) {
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

  folly::EventBase* getEventBase() { return requestParams_.eventBase_; }

  void clearRequestParams() { requestParams_ = RequestParams(); }

  virtual concurrency::PRIORITY getRequestPriority(
      Cpp2RequestContext* ctx, concurrency::PRIORITY prio);
  // TODO: replace with getRequestExecutionScope.
  concurrency::PRIORITY getRequestPriority(Cpp2RequestContext* ctx) {
    return getRequestPriority(ctx, concurrency::NORMAL);
  }

  virtual concurrency::ThreadManager::ExecutionScope getRequestExecutionScope(
      Cpp2RequestContext* ctx, concurrency::PRIORITY defaultPriority) {
    concurrency::ThreadManager::ExecutionScope es(
        getRequestPriority(ctx, defaultPriority));
    return es;
  }
  concurrency::ThreadManager::ExecutionScope getRequestExecutionScope(
      Cpp2RequestContext* ctx) {
    return getRequestExecutionScope(ctx, concurrency::NORMAL);
  }

  folly::SemiFuture<folly::Unit> semifuture_onStartServing() override {
    return folly::makeSemiFuture();
  }
  folly::SemiFuture<folly::Unit> semifuture_onStopServing() override {
    return folly::makeSemiFuture();
  }

  std::vector<ServiceHandler*> getServiceHandlers() override { return {this}; }

  /**
   * The concrete instance of MethodMetadata that generated AsyncProcessors
   * expect will be passed to them. Therefore, generated service handlers will
   * also create instances of these for entries in
   * AsyncProcessorFactory::createMethodMetadata.
   */
  template <typename Processor>
  struct GeneratedMethodMetadata final
      : public AsyncProcessorFactory::MethodMetadata {
    explicit GeneratedMethodMetadata(
        GeneratedAsyncProcessor::ProcessFuncs<Processor> funcs)
        : processFuncs(funcs) {}

    GeneratedAsyncProcessor::ProcessFuncs<Processor> processFuncs;
  };

 protected:
  folly::Executor::KeepAlive<> getInternalKeepAlive();

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
 private:
  IOWorkerContext::ReplyQueue& getReplyQueue() {
    auto worker = reinterpret_cast<IOWorkerContext*>(
        const_cast<Cpp2Worker*>(reqCtx_->getConnectionContext()->getWorker()));
    DCHECK(worker != nullptr);
    return worker->getReplyQueue();
  }

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
      Cpp2RequestContext* reqCtx,
      TilePtr&& interaction = {})
      : req_(std::move(req)),
        ctx_(std::move(ctx)),
        interaction_(std::move(interaction)),
        ewp_(ewp),
        eb_(eb),
        tm_(tm),
        reqCtx_(reqCtx),
        protoSeqId_(0) {}

  virtual ~HandlerCallbackBase();

  static void releaseRequest(
      ResponseChannelRequest::UniquePtr request,
      folly::EventBase* eb,
      TilePtr&& interaction = {});

  void exception(std::exception_ptr ex) { doException(ex); }

  void exception(folly::exception_wrapper ew) { doExceptionWrapped(ew); }

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

  template <class F>
  void runFuncInQueue(F&& func, bool oneway = false);

  folly::Executor::KeepAlive<> getInternalKeepAlive();

 protected:
  // HACK(tudorb): Call this to set up forwarding to the event base and
  // thread manager of the other callback.  Use when you want to create
  // callback wrappers that forward to another callback (and do some
  // pre- / post-processing).
  void forward(const HandlerCallbackBase& other);

  folly::Optional<uint32_t> checksumIfNeeded(
      LegacySerializedResponse& response);

  folly::Optional<uint32_t> checksumIfNeeded(SerializedResponse& response);

  virtual void transform(LegacySerializedResponse& reponse);

  // Can be called from IO or TM thread
  virtual void doException(std::exception_ptr ex) {
    doExceptionWrapped(folly::exception_wrapper(ex));
  }

  virtual void doExceptionWrapped(folly::exception_wrapper ew);
  virtual void doAppOverloadedException(const std::string& message);

  template <typename F, typename T>
  void callExceptionInEventBaseThread(F&& f, T&& ex);

  template <typename Reply, typename... A>
  void putMessageInReplyQueue(std::in_place_type_t<Reply> tag, A&&... a);

  void sendReply(LegacySerializedResponse response);
  void sendReply(ResponseAndServerStreamFactory&& responseAndStream);

#if !FOLLY_HAS_COROUTINES
  [[noreturn]]
#endif
  void
  sendReply(
      FOLLY_MAYBE_UNUSED std::pair<
          apache::thrift::LegacySerializedResponse,
          apache::thrift::detail::SinkConsumerImpl>&& responseAndSinkConsumer);

  // Required for this call
  ResponseChannelRequest::UniquePtr req_;
  std::unique_ptr<ContextStack> ctx_;
  TilePtr interaction_;

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
  using Helper = apache::thrift::detail::HandlerCallbackHelper<T>;
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
      Cpp2RequestContext* reqCtx,
      folly::Executor::KeepAlive<> streamEx = nullptr,
      TilePtr&& interaction = {});

  void result(InputType r) { doResult(std::forward<InputType>(r)); }
  void result(std::unique_ptr<ResultType> r);

  void complete(folly::Try<T>&& r);

 protected:
  virtual void doResult(InputType r);

  cob_ptr cp_;
  folly::Executor::KeepAlive<> streamEx_;
};

template <>
class HandlerCallback<void> : public HandlerCallbackBase {
  using cob_ptr =
      LegacySerializedResponse (*)(int32_t protoSeqId, ContextStack*);

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
      Cpp2RequestContext* reqCtx,
      TilePtr&& interaction = {});

  void done() { doDone(); }

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
  if (c) {
    c->preRead();
  }
  SerializedMessage smsg;
  smsg.protocolType = iprot.protocolType();
  smsg.buffer = serializedRequest.buffer.get();
  smsg.methodName = methodName;
  if (c) {
    c->onReadData(smsg);
  }
  uint32_t bytes = 0;
  try {
    bytes = apache::thrift::detail::deserializeRequestBody(&iprot, &args);
    iprot.readMessageEnd();
  } catch (const std::exception& ex) {
    throw RequestParsingError(ex.what());
  } catch (...) {
    throw RequestParsingError(
        folly::exceptionStr(std::current_exception()).toStdString());
  }
  if (c) {
    c->postRead(nullptr, bytes);
  }
}

template <typename ProtocolOut, typename Result>
LegacySerializedResponse GeneratedAsyncProcessor::serializeLegacyResponse(
    const char* method,
    ProtocolOut* prot,
    int32_t protoSeqId,
    ContextStack* ctx,
    const Result& result) {
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  size_t bufSize =
      apache::thrift::detail::serializedResponseBodySizeZC(prot, &result);
  bufSize += prot->serializedMessageSize(method);

  // Preallocate small buffer headroom for transports metadata & framing.
  constexpr size_t kHeadroomBytes = 128;
  auto buf = folly::IOBuf::create(kHeadroomBytes + bufSize);
  buf->advance(kHeadroomBytes);
  queue.append(std::move(buf));

  prot->setOutput(&queue, bufSize);
  if (ctx) {
    ctx->preWrite();
  }
  prot->writeMessageBegin(method, MessageType::T_REPLY, protoSeqId);
  apache::thrift::detail::serializeResponseBody(prot, &result);
  prot->writeMessageEnd();
  SerializedMessage smsg;
  smsg.protocolType = prot->protocolType();
  smsg.buffer = queue.front();
  if (ctx) {
    ctx->onWriteData(smsg);
  }
  DCHECK_LE(
      queue.chainLength(),
      static_cast<size_t>(std::numeric_limits<int>::max()));
  if (ctx) {
    ctx->postWrite(folly::to_narrow(queue.chainLength()));
  }
  return LegacySerializedResponse{queue.move()};
}

template <typename ChildType>
std::unique_ptr<concurrency::Runnable>
GeneratedAsyncProcessor::makeEventTaskForRequest(
    ResponseChannelRequest::UniquePtr req,
    SerializedCompressedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    RpcKind kind,
    ProcessFunc<ChildType> processFunc,
    ChildType* childClass,
    Tile* tile) {
  auto task = std::make_unique<RequestTask<ChildType>>(
      std::move(req),
      std::move(serializedRequest),
      eb,
      tm,
      ctx,
      kind == RpcKind::SINGLE_REQUEST_NO_RESPONSE,
      childClass,
      processFunc);
  if (tile) {
    task->setTile({tile, eb});
  }
  return task;
}

template <typename ChildType>
void GeneratedAsyncProcessor::processInThread(
    ResponseChannelRequest::UniquePtr req,
    SerializedCompressedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm,
    RpcKind kind,
    ProcessFunc<ChildType> processFunc,
    ChildType* childClass) {
  Tile* tile = nullptr;
  if (auto interactionId = ctx->getInteractionId()) { // includes create
    try {
      tile = &ctx->getConnectionContext()->getTile(interactionId);
    } catch (const std::out_of_range&) {
      req->sendErrorWrapped(
          TApplicationException(
              "Invalid interaction id " + std::to_string(interactionId)),
          kInteractionIdUnknownErrorCode);
      return;
    }
  }

  auto scope = ctx->getRequestExecutionScope();
  auto task = makeEventTaskForRequest(
      std::move(req),
      std::move(serializedRequest),
      ctx,
      eb,
      tm,
      kind,
      processFunc,
      childClass,
      tile);

  if (tile && tile->__fbthrift_maybeEnqueue(std::move(task), scope)) {
    return;
  }

  using Source = concurrency::ThreadManager::Source;
  auto source = tile && !ctx->getInteractionCreate()
      ? Source::EXISTING_INTERACTION
      : Source::UPSTREAM;
  tm->getKeepAlive(std::move(scope), source)->add([task = std::move(task)] {
    task->run();
  });
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
                                          reqCtx = reqCtx_,
                                          interaction = std::move(interaction_),
                                          eb = getEventBase()]() mutable {
      f(std::move(req), protoSeqId, ctx.get(), ex, reqCtx);
    });
  }
}

template <typename Reply, typename... A>
void HandlerCallbackBase::putMessageInReplyQueue(
    std::in_place_type_t<Reply> tag, A&&... a) {
  if constexpr (folly::kIsWindows) {
    // TODO(T88449658): We are seeing performance regression on Windows if we
    // use the reply queue. The exact cause is under investigation. Before it is
    // fixed, we can use the default EventBase queue on Windows for now.
    auto eb = getEventBase();
    eb->runInEventBaseThread(
        [eb, reply = Reply(static_cast<A&&>(a)...)]() mutable { reply(*eb); });
  } else {
    getReplyQueue().putMessage(tag, static_cast<A&&>(a)...);
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
    Cpp2RequestContext* reqCtx,
    folly::Executor::KeepAlive<> streamEx,
    TilePtr&& interaction)
    : HandlerCallbackBase(
          std::move(req),
          std::move(ctx),
          ewp,
          eb,
          tm,
          reqCtx,
          std::move(interaction)),
      cp_(cp),
      streamEx_(std::move(streamEx)) {
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
      std::move(this->streamEx_),
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
  using InputType = const typename apache::thrift::detail::inner_type<T>::type&;
  using CobPtr = apache::thrift::LegacySerializedResponse (*)(
      int32_t protoSeqId, ContextStack*, InputType);
  static apache::thrift::LegacySerializedResponse call(
      CobPtr cob,
      int32_t protoSeqId,
      ContextStack* ctx,
      folly::Executor::KeepAlive<>,
      InputType input) {
    return cob(protoSeqId, ctx, input);
  }
};

template <typename StreamInputType>
struct HandlerCallbackHelperServerStream {
  using InputType = StreamInputType&&;
  using CobPtr = ResponseAndServerStreamFactory (*)(
      int32_t protoSeqId,
      ContextStack*,
      folly::Executor::KeepAlive<>,
      InputType);
  static ResponseAndServerStreamFactory call(
      CobPtr cob,
      int32_t protoSeqId,
      ContextStack* ctx,
      folly::Executor::KeepAlive<> streamEx,
      InputType input) {
    return cob(protoSeqId, ctx, std::move(streamEx), std::move(input));
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
  using InputType = SinkInputType&&;
  using CobPtr =
      std::pair<apache::thrift::LegacySerializedResponse, SinkConsumerImpl> (*)(
          ContextStack*, InputType, folly::Executor::KeepAlive<>);
  static std::pair<apache::thrift::LegacySerializedResponse, SinkConsumerImpl>
  call(
      CobPtr cob,
      int32_t,
      ContextStack* ctx,
      folly::Executor::KeepAlive<> streamEx,
      InputType input) {
    return cob(ctx, std::move(input), std::move(streamEx));
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
