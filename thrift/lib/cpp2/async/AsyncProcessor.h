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

#ifndef THRIFT_ASYNCPROCESSOR2_H
#define THRIFT_ASYNCPROCESSOR2_H 1

#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <folly/String.h>
#include <folly/MoveWrapper.h>
#include <folly/futures/Future.h>
#include <wangle/deprecated/rx/Observer.h>

namespace apache { namespace thrift {

class EventTask : public virtual apache::thrift::concurrency::Runnable {
 public:
  EventTask(std::function<void()>&& taskFunc,
  apache::thrift::ResponseChannel::Request* req,
    apache::thrift::async::TEventBase* base,
    bool oneway)
      : taskFunc_(std::move(taskFunc))
      , req_(req)
      , base_(base)
      , oneway_(oneway) {
}

  void run() override {
    if (!oneway_) {
      if (req_ && !req_->isActive()) {
        auto req = req_;
        base_->runInEventBaseThread([req]() {
          delete req;
        });

        return;
      }
    }
    taskFunc_();
  }

  void expired() {
    if (!oneway_) {
      auto req = req_;
      if (req) {
        base_->runInEventBaseThread([req] () {
            req->sendErrorWrapped(
                folly::make_exception_wrapper<TApplicationException>(
                  "Failed to add task to queue, too full"),
                kQueueOverloadedErrorCode);
            delete req;
          });
      }
    }
  }

 private:
  std::function<void()> taskFunc_;
  apache::thrift::ResponseChannel::Request* req_;
  apache::thrift::async::TEventBase* base_;
  bool oneway_;
};

class PriorityEventTask : public apache::thrift::concurrency::PriorityRunnable,
                          public EventTask {
 public:
  PriorityEventTask(
    apache::thrift::concurrency::PriorityThreadManager::PRIORITY priority,
    std::function<void()>&& taskFunc,
    apache::thrift::ResponseChannel::Request* req,
    apache::thrift::async::TEventBase* base,
    bool oneway)
      : EventTask(std::move(taskFunc), req, base, oneway)
      , priority_(priority) {}

  apache::thrift::concurrency::PriorityThreadManager::PRIORITY getPriority()
      const override {
    return priority_;
  }
  using EventTask::run;
 private:
  apache::thrift::concurrency::PriorityThreadManager::PRIORITY priority_;
};

class AsyncProcessor : public TProcessorBase {
 public:
  virtual ~AsyncProcessor() {}

  virtual void process(std::unique_ptr<ResponseChannel::Request> req,
                       std::unique_ptr<folly::IOBuf> buf,
                       apache::thrift::protocol::PROTOCOL_TYPES protType,
                       Cpp2RequestContext* context,
                       apache::thrift::async::TEventBase* eb,
                       apache::thrift::concurrency::ThreadManager* tm) = 0;

  virtual bool isOnewayMethod(const folly::IOBuf* buf,
                              const transport::THeader* header) = 0;

};

class GeneratedAsyncProcessor : public AsyncProcessor {
 public:
  ~GeneratedAsyncProcessor() override {}

  template <typename Derived, typename ProtocolReader>
  using ProcessFunc = void(Derived::*)(
      std::unique_ptr<apache::thrift::ResponseChannel::Request>,
      std::unique_ptr<folly::IOBuf>,
      std::unique_ptr<ProtocolReader> iprot,
      apache::thrift::Cpp2RequestContext* context,
      apache::thrift::async::TEventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProcessFunc>
  using ProcessMap = std::unordered_map<std::string, ProcessFunc>;

 protected:
  template <typename ProtocolIn, typename Args>
  static void deserializeRequest(Args& args,
                                 folly::IOBuf* buf,
                                 ProtocolIn* iprot,
                                 apache::thrift::ContextStack* c) {
    c->preRead();
    apache::thrift::SerializedMessage smsg;
    smsg.protocolType = iprot->protocolType();
    smsg.buffer = buf;
    c->onReadData(smsg);
    uint32_t bytes = ::apache::thrift::Cpp2Ops<Args>::read(iprot, &args);
    iprot->readMessageEnd();
    c->postRead(nullptr, bytes);
  }

  template <typename ProtocolOut, typename Result>
  static folly::IOBufQueue serializeResponse(const char* method,
                                             ProtocolOut* prot,
                                             int32_t protoSeqId,
                                             apache::thrift::ContextStack* ctx,
                                             const Result& result) {
    folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
    size_t bufSize = Cpp2Ops<Result>::serializedSizeZC(prot, &result);
    bufSize += prot->serializedMessageSize(method);
    prot->setOutput(&queue, bufSize);
    ctx->preWrite();
    prot->writeMessageBegin(method, apache::thrift::T_REPLY, protoSeqId);
    Cpp2Ops<Result>::write(prot, &result);
    prot->writeMessageEnd();
    ::apache::thrift::SerializedMessage smsg;
    smsg.protocolType = prot->protocolType();
    smsg.buffer = queue.front();
    ctx->onWriteData(smsg);
    ctx->postWrite(queue.chainLength());
    return queue;
  }

  template <typename ProtocolOut>
  static folly::IOBufQueue serializeException(
      const char* method,
      ProtocolOut* prot,
      int32_t protoSeqId,
      apache::thrift::ContextStack* ctx,
      const apache::thrift::TApplicationException& x) {
    folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
    size_t bufSize = x.serializedSizeZC(prot);
    bufSize += prot->serializedMessageSize(method);
    prot->setOutput(&queue, bufSize);
    if (ctx) {
      ctx->handlerError();
    }
    prot->writeMessageBegin(method, apache::thrift::T_EXCEPTION, protoSeqId);
    x.write(prot);
    prot->writeMessageEnd();
    return queue;
  }

  template <typename ProtocolIn_, typename ProtocolOut_,
            typename ProcessFunc, typename ChildType>
  static void processInThread(
      std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ProtocolIn_> iprot,
      apache::thrift::Cpp2RequestContext* ctx,
      apache::thrift::async::TEventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm,
      apache::thrift::concurrency::PRIORITY pri,
      bool oneway,
      ProcessFunc processFunc,
      ChildType* childClass) {
    using folly::makeMoveWrapper;
    if (oneway) {
      if (!req->isOneway()) {
        req->sendReply(std::unique_ptr<folly::IOBuf>());
      }
    }
    auto preq = req.get();
    auto iprot_holder = folly::makeMoveWrapper(std::move(iprot));
    auto buf_mw = folly::makeMoveWrapper(std::move(buf));
    try {
      tm->add(
        std::make_shared<apache::thrift::PriorityEventTask>(
          pri,
          [=]() mutable {
            auto req_mw = folly::makeMoveWrapper(
              std::unique_ptr<apache::thrift::ResponseChannel::Request>(preq));
            if ((*req_mw)->getTimestamps().processBegin != 0) {
              // Since this request was queued, reset the processBegin
              // time to the actual start time, and not the queue time.
              (*req_mw)->getTimestamps().processBegin =
                apache::thrift::concurrency::Util::currentTimeUsec();
            }
            // Oneway request won't be canceled if expired. see
            // D1006482 for furhter details.  TODO: fix this
            if (!oneway) {
              if (!(*req_mw)->isActive()) {
                eb->runInEventBaseThread([=]() mutable {
                  delete req_mw->release();
                });
                return;
              }
            }
            (childClass->*processFunc)(std::move(*req_mw), std::move(*buf_mw),
                        std::move(*iprot_holder), ctx, eb, tm);

          },
          preq, eb, oneway),
        0, // timeout
        0, // expiration
        true, // cancellable
        true); // numa
      req.release();
    } catch (const std::exception& e) {
      if (!oneway) {
        req->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(
              "Failed to add task to queue, too full"),
            kQueueOverloadedErrorCode);
      }
    }
  }
};

/**
 * HandlerCallback class for async callbacks.
 *
 * These are constructed by the generated code, and your handler calls
 * either result(value), done(), or exception(ex) to finish the async
 * call.  Only one of these must be called, otherwise your client
 * will likely get confused with multiple response messages.
 *
 *
 * If you passed the HandlerCallback to another thread, you may call
 * the *InThread() version of these methods.  The callback will be
 * called in the correct thread, then it will *delete* itself (because
 * otherwise you wouldnoy know when to delete it). So make sure to
 * .release() the unique_ptr on the HandlerCallback if you call the
 * *InThread() method.
 */
class HandlerCallbackBase {
 protected:
  typedef void(*exn_ptr)(std::unique_ptr<ResponseChannel::Request>,
                         int32_t protoSeqId,
                         apache::thrift::ContextStack*,
                         std::exception_ptr,
                         Cpp2RequestContext*);
  typedef void(*exnw_ptr)(std::unique_ptr<ResponseChannel::Request>,
                          int32_t protoSeqId,
                          apache::thrift::ContextStack*,
                          folly::exception_wrapper,
                          Cpp2RequestContext*);
 public:

  HandlerCallbackBase()
      : ep_(nullptr)
      , eb_(nullptr)
      , tm_(nullptr)
      , reqCtx_(nullptr)
      , protoSeqId_(0) {}

  HandlerCallbackBase(
    std::unique_ptr<ResponseChannel::Request> req,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    exn_ptr ep,
    exnw_ptr ewp,
    apache::thrift::async::TEventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx) :
      req_(std::move(req)),
      ctx_(std::move(ctx)),
      ep_(ep),
      ewp_(ewp),
      eb_(eb),
      tm_(tm),
      reqCtx_(reqCtx),
      protoSeqId_(0) {
  }

  virtual ~HandlerCallbackBase() {
    // req must be deleted in the eb
    if (req_) {
      DCHECK(eb_);
      auto req_mw = folly::makeMoveWrapper(std::move(req_));
      eb_->runInEventBaseThread([=]() mutable {
        req_mw->reset();
      });
    }
  }

  void exception(std::exception_ptr ex) {
    doException(ex);
  }

  void exception(folly::exception_wrapper ew) {
    doExceptionWrapped(ew);
  }

  // Warning: just like "throw ex", this captures the STATIC type of ex, not
  // the dynamic type.  If you need the dynamic type, then either you should
  // be using exception_ptr instead of a reference to a base exception class,
  // or your exception hierarchy should be equipped to throw polymorphically,
  // see // http://www.parashift.com/c++-faq/throwing-polymorphically.html
  template <class Exception>
  void exception(const Exception& ex) {
    exception(folly::make_exception_wrapper<Exception>(ex));
  }

  void deleteInThread() {
    getEventBase()->runInEventBaseThread([=](){
        delete this;
      });
  }

  void exceptionInThread(std::exception_ptr ex) {
    getEventBase()->runInEventBaseThread([=](){
        this->exception(ex);
        delete this;
      });
  }

  void exceptionInThread(folly::exception_wrapper ew) {
    getEventBase()->runInEventBaseThread([=](){
        this->exception(ew);
        delete this;
      });
  }

  template <class Exception>
  void exceptionInThread(const Exception& ex) {
    exceptionInThread(folly::make_exception_wrapper<Exception>(ex));
  }

  static void exceptionInThread(std::unique_ptr<HandlerCallbackBase> thisPtr,
                                std::exception_ptr ex) {
    DCHECK(thisPtr);
    thisPtr.release()->exceptionInThread(std::move(ex));
  }

  static void exceptionInThread(std::unique_ptr<HandlerCallbackBase> thisPtr,
                                folly::exception_wrapper ew) {
    DCHECK(thisPtr);
    thisPtr.release()->exceptionInThread(std::move(ew));
  }

  template <class Exception>
  static void exceptionInThread(std::unique_ptr<HandlerCallbackBase> thisPtr,
                                const Exception& ex) {
    DCHECK(thisPtr);
    thisPtr.release()->exceptionInThread(ex);
  }

  apache::thrift::async::TEventBase* getEventBase() {
    assert(eb_);
    return eb_;
  }

  apache::thrift::concurrency::ThreadManager* getThreadManager() {
    assert(tm_);
    return tm_;
  }

  Cpp2RequestContext* getConnectionContext() {
    return reqCtx_;
  }

  bool isRequestActive() {
    // If req_ is nullptr probably it is not managed by this HandlerCallback
    // object and we just return true. An example can be found in task 3106731
    return !req_ || req_->isActive();
  }

  ResponseChannel::Request* getRequest() {
    return req_.get();
  }

  template <class F>
  void runFuncInQueue(F&& func) {
    assert(tm_);
    try {
      tm_->add(concurrency::FunctionRunner::create(std::move(func)),
        0, // timeout
        0, // expiration
        true, // cancellable
        true); // numa
    } catch (...) {
      apache::thrift::TApplicationException ex(
        "Failed to add task to queue, too full");
      if (req_ && ep_) {
        req_->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(std::move(ex)),
            kQueueOverloadedErrorCode);
      } else {
        LOG(ERROR) << folly::exceptionStr(ex);
      }

      // Delete the callback if exception is thrown since error response
      // is already sent.
      deleteInThread();
    }
  }

 protected:
  // HACK(tudorb): Call this to set up forwarding to the event base and
  // thread manager of the other callback.  Use when you want to create
  // callback wrappers that forward to another callback (and do some
  // pre- / post-processing).
  void forward(const HandlerCallbackBase& other) {
    eb_ = other.eb_;
    tm_ = other.tm_;
    ep_ = other.ep_;
    ewp_ = other.ewp_;
  }

  virtual void transform(folly::IOBufQueue& queue) {
    // Do any compression or other transforms in this thread, the same thread
    // that serialization happens on.
    queue.append(
      transport::THeader::transform(queue.move(),
                                    reqCtx_->getTransforms(),
                                    reqCtx_->getMinCompressBytes()));
  }

  // Can be called from IO or TM thread
  virtual void doException(std::exception_ptr ex) {
    if (req_ == nullptr) {
      LOG(ERROR) << folly::exceptionStr(ex);
    } else {
      callExceptionInEventBaseThread(ep_, ex);
    }
  }

  virtual void doExceptionWrapped(folly::exception_wrapper ew) {
    if (req_ == nullptr) {
      LOG(ERROR) << ew.what();
    } else {
      callExceptionInEventBaseThread(ewp_, ew);
    }
  }

  template <typename F, typename T>
  void callExceptionInEventBaseThread(F&& f, T&& ex) {
    if (!f) {
      return;
    }
    if (getEventBase()->isInEventBaseThread()) {
      f(std::move(req_), protoSeqId_, ctx_.get(), ex, reqCtx_);
      ctx_.reset();
    } else {
      auto req = folly::makeMoveWrapper(std::move(req_));
      auto ctx = folly::makeMoveWrapper(std::move(ctx_));
      auto protoSeqId = protoSeqId_;
      auto reqCtx = reqCtx_;
      getEventBase()->runInEventBaseThread([=]() mutable {
        f(std::move(*req), protoSeqId, ctx->get(), ex, reqCtx);
      });
    }
  }

  void sendReply(folly::IOBufQueue queue) {
    transform(queue);
    if (getEventBase()->isInEventBaseThread()) {
      req_->sendReply(queue.move());
    } else {
      auto req_mw = folly::makeMoveWrapper(std::move(req_));
      auto queue_mw = folly::makeMoveWrapper(std::move(queue));
      getEventBase()->runInEventBaseThread([=]() mutable {
        (*req_mw)->sendReply(queue_mw->move());
      });
    }
  }

  // Required for this call
  std::unique_ptr<ResponseChannel::Request> req_;
  std::unique_ptr<apache::thrift::ContextStack> ctx_;

  // May be null in a oneway call
  exn_ptr ep_;
  exnw_ptr ewp_;

  // Useful pointers, so handler doesn't need to have a pointer to the server
  apache::thrift::async::TEventBase* eb_;
  apache::thrift::concurrency::ThreadManager* tm_;
  Cpp2RequestContext* reqCtx_;

  int32_t protoSeqId_;
};

namespace detail {

// template that typedefs type to its argument, unless the argument is a
// unique_ptr<S>, in which case it typedefs type to S.
template <class S>
struct inner_type {typedef S type;};
template <class S>
struct inner_type<std::unique_ptr<S>> {typedef S type;};

}

template <typename T>
class HandlerCallback : public HandlerCallbackBase {
 public:
  typedef typename detail::inner_type<T>::type ResultType;

 private:
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      apache::thrift::ContextStack*,
      const ResultType&);
 public:

  HandlerCallback()
      : cp_(nullptr) {}

  HandlerCallback(
    std::unique_ptr<ResponseChannel::Request> req,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    cob_ptr cp,
    exn_ptr ep,
    exnw_ptr ewp,
    int32_t protoSeqId,
    apache::thrift::async::TEventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx) :
      HandlerCallbackBase(std::move(req), std::move(ctx), ep, ewp,
                          eb, tm, reqCtx),
      cp_(cp) {
    this->protoSeqId_ = protoSeqId;
  }

  void result(const ResultType& r) {
    doResult(r);
  }
  void result(std::unique_ptr<ResultType> r) {
    doResult(*r);
  }
  void resultInThread(const ResultType& r) {
    result(r);
    delete this;
  }
  void resultInThread(std::unique_ptr<ResultType> r) {
    result(*r);
    delete this;
  }
  void resultInThread(const std::shared_ptr<ResultType>& r) {
    result(*r);
    delete this;
  }

  static void resultInThread(
      std::unique_ptr<HandlerCallback> thisPtr,
      const ResultType& r) {
    DCHECK(thisPtr);
    thisPtr.release()->resultInThread(r);
  }
  static void resultInThread(
      std::unique_ptr<HandlerCallback> thisPtr,
      std::unique_ptr<ResultType> r) {
    DCHECK(thisPtr);
    thisPtr.release()->resultInThread(std::move(r));
  }
  static void resultInThread(
      std::unique_ptr<HandlerCallback> thisPtr,
      const std::shared_ptr<ResultType>& r) {
    DCHECK(thisPtr);
    thisPtr.release()->resultInThread(r);
  }

  void complete(folly::Try<T>&& r) {
    try {
      result(std::move(r.value()));
    } catch (...) {
      exception(std::current_exception());
    }
  }
  void completeInThread(folly::Try<T>&& r) {
    try {
      resultInThread(std::move(r.value()));
    } catch (...) {
      exceptionInThread(std::current_exception());
    }
  }
  static void completeInThread(
      std::unique_ptr<HandlerCallback> thisPtr,
      folly::Try<T>&& r) {
    DCHECK(thisPtr);
    thisPtr.release()->completeInThread(std::forward<folly::Try<T>>(r));
  }

 protected:

  virtual void doResult(const ResultType& r) {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_,
                     this->ctx_.get(),
                     r);
    this->ctx_.reset();
    sendReply(std::move(queue));
  }

  cob_ptr cp_;
};

template <>
class HandlerCallback<void> : public HandlerCallbackBase {
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      apache::thrift::ContextStack*);
 public:
  typedef void ResultType;

  HandlerCallback()
      : cp_(nullptr) {}

  HandlerCallback(
    std::unique_ptr<ResponseChannel::Request> req,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    cob_ptr cp,
    exn_ptr ep,
    exnw_ptr ewp,
    int32_t protoSeqId,
    apache::thrift::async::TEventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx) :
      HandlerCallbackBase(std::move(req), std::move(ctx), ep, ewp,
                          eb, tm, reqCtx),
      cp_(cp) {
    this->protoSeqId_ = protoSeqId;
  }

  ~HandlerCallback() override {}

  void done() {
    doDone();
  }
  void doneInThread() {
    done();
    delete this;
  }
  static void doneInThread(
      std::unique_ptr<HandlerCallback> thisPtr) {
    DCHECK(thisPtr);
    thisPtr.release()->doneInThread();
  }

  void complete(folly::Try<folly::Unit>&& r) {
    try {
      r.throwIfFailed();
      done();
    } catch (...) {
      exception(std::current_exception());
    }
  }
  void completeInThread(folly::Try<folly::Unit>&& r) {
    try {
      r.throwIfFailed();
      doneInThread();
    } catch (...) {
      exceptionInThread(std::current_exception());
    }
  }
  static void completeInThread(
      std::unique_ptr<HandlerCallback> thisPtr,
      folly::Try<folly::Unit>&& r) {
    DCHECK(thisPtr);
    thisPtr.release()->completeInThread(
        std::forward<folly::Try<folly::Unit>>(r));
  }

 protected:
  virtual void doDone() {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_,
                     this->ctx_.get());
    this->ctx_.reset();
    sendReply(std::move(queue));
  }

  cob_ptr cp_;
};

template <typename T>
class StreamingHandlerCallback :
    public HandlerCallbackBase,
    public wangle::Observer<typename detail::inner_type<T>::type> {
public:
  typedef typename detail::inner_type<T>::type ResultType;

 private:
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      apache::thrift::ContextStack*,
      const ResultType&);
 public:

  StreamingHandlerCallback(
    std::unique_ptr<ResponseChannel::Request> req,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    cob_ptr cp,
    exn_ptr ep,
    exnw_ptr ewp,
    int32_t protoSeqId,
    apache::thrift::async::TEventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm,
    Cpp2RequestContext* reqCtx) :
      HandlerCallbackBase(std::move(req), std::move(ctx), ep, ewp,
                          eb, tm, reqCtx),
      cp_(cp) {
    this->protoSeqId_ = protoSeqId;
  }

  void write(const ResultType& r) {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_, ctx_.get(), r);
    sendReplyNonDestructive(std::move(queue), "thrift_stream", "chunk");
  }

  void done() {
    auto queue = cp_(this->protoSeqId_, ctx_.get(), ResultType());
    ctx_.reset();
    sendReply(std::move(queue));
  }

  void doneAndDelete() {
    done();
    delete this;
  }

  void resultInThread(const ResultType& value) {
    LOG(FATAL) << "resultInThread";
  }

  // Observer overrides
  void onNext(const ResultType& r) override {
    write(r);
  }

  void onCompleted() override {
    done();
  }

  void onError(wangle::Error e) override {
    exception(e);
  }
 private:
  void sendReplyNonDestructive(folly::IOBufQueue queue,
      const std::string& key,
      const std::string& value) {
    transform(queue);
    if (getEventBase()->isInEventBaseThread()) {
      reqCtx_->setHeader(key, value);
      req_->sendReply(queue.move());
    } else {
      auto req_raw = req_.get();
      auto queue_mw = folly::makeMoveWrapper(std::move(queue));
      getEventBase()->runInEventBaseThread([=]() mutable {
        reqCtx_->setHeader(key, value);
        req_raw->sendReply(queue_mw->move());
      });
    }
  }

  cob_ptr cp_;
};

class AsyncProcessorFactory {
  public:
    virtual std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() = 0;
    virtual ~AsyncProcessorFactory() {};
};

class ServerInterface : public AsyncProcessorFactory {
 public:
  ~ServerInterface() override{};

  Cpp2RequestContext* getConnectionContext() {
    return reqCtx_;
  }

  void setConnectionContext(Cpp2RequestContext* c) {
    reqCtx_ = c;
  }

  void setThreadManager(apache::thrift::concurrency::ThreadManager* tm) {
    tm_ = tm;
  }

  apache::thrift::concurrency::ThreadManager* getThreadManager() {
    return tm_;
  }

  void setEventBase(apache::thrift::async::TEventBase* eb) {
    folly::RequestEventBase::set(eb);
    eb_ = eb;
  }

  apache::thrift::async::TEventBase* getEventBase() {
    return eb_;
  }

  apache::thrift::concurrency::PRIORITY getRequestPriority(
      apache::thrift::Cpp2RequestContext* ctx,
      apache::thrift::concurrency::PRIORITY prio=apache::thrift::concurrency::NORMAL) {
    apache::thrift::concurrency::PRIORITY callPriority = ctx->getCallPriority();
    if (callPriority != apache::thrift::concurrency::N_PRIORITIES) {
      return callPriority;
    }
    return prio;
  }

 private:
  /**
   * TODO(davejwatson): c++11 thread_local in gcc 4.8
   *
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
  static __thread Cpp2RequestContext* reqCtx_;
  static __thread apache::thrift::concurrency::ThreadManager* tm_;
  static __thread apache::thrift::async::TEventBase* eb_;
};

}} // apache::thrift

#endif // THRIFT_ASYNCPROCESSOR2_H
