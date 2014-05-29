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
#ifndef THRIFT_ASYNCPROCESSOR2_H
#define THRIFT_ASYNCPROCESSOR2_H 1

#include "thrift/lib/cpp/TProcessor.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/transport/THeader.h"
#include "thrift/lib/cpp/concurrency/Thread.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/TApplicationException.h"
#include "thrift/lib/cpp/protocol/TProtocolTypes.h"
#include "thrift/lib/cpp2/async/ResponseChannel.h"
#include "thrift/lib/cpp2/protocol/Protocol.h"
#include "thrift/lib/cpp2/server/Cpp2ConnContext.h"
#include "thrift/lib/cpp2/Thrift.h"
#include "folly/String.h"
#include "folly/MoveWrapper.h"

namespace apache { namespace thrift {

enum class SerializationThread {
  CURRENT,
  EVENT_BASE
};

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

  void run() {
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
                kOverloadedErrorCode);
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

  apache::thrift::concurrency::PriorityThreadManager::PRIORITY
  getPriority() const {
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
  virtual ~GeneratedAsyncProcessor() {}

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
    c->postRead(bytes);
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
          preq, eb, oneway));
      req.release();
    } catch (const std::exception& e) {
      if (!oneway) {
        req->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(
              "Failed to add task to queue, too full"),
            kOverloadedErrorCode);
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
                         std::unique_ptr<apache::thrift::ContextStack>,
                         std::exception_ptr);
  typedef void(*exnw_ptr)(std::unique_ptr<ResponseChannel::Request>,
                          int32_t protoSeqId,
                          std::unique_ptr<apache::thrift::ContextStack>,
                          folly::exception_wrapper);
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

  template <class Exception>
  void exceptionInThread(const Exception& ex) {
    exceptionInThread(std::make_exception_ptr(ex));
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

  void runInQueue(
    std::shared_ptr<apache::thrift::concurrency::Runnable> task) {
    assert(tm_);
    try {
      tm_->add(task);
    } catch (...) {
      apache::thrift::TApplicationException ex(
        "Failed to add task to queue, too full");
      if (req_ && ep_) {
        req_->sendErrorWrapped(
            folly::make_exception_wrapper<TApplicationException>(std::move(ex)),
            kOverloadedErrorCode);
      } else {
        LOG(ERROR) << folly::exceptionStr(ex);
      }
    }
  }

 protected:
  // HACK(tudorb): Call this to set up forwarding to the event base and
  // thread manager of the other callback.  Use when you want to create
  // callback wrappers that forward to another callback (and do some
  // pre- / post-processing).
  void forward(const HandlerCallbackBase& other) {
    ep_ = other.ep_;
    eb_ = other.eb_;
    tm_ = other.tm_;
  }

  // Always called in IO thread
  virtual void doException(std::exception_ptr ex) {
    if (req_ == nullptr) {
      LOG(ERROR) << folly::exceptionStr(ex);
    } else {
      if (ep_) {
        ep_(std::move(req_), protoSeqId_, std::move(ctx_), ex);
      }
    }
  }

  virtual void transform(folly::IOBufQueue& queue) {
    // Do any compression or other transforms in this thread, the same thread
    // that serialization happens on.
    queue.append(
      transport::THeader::transform(queue.move(),
                                    reqCtx_->getTransforms(),
                                    reqCtx_->getMinCompressBytes()));
  }

  virtual void doExceptionWrapped(folly::exception_wrapper ew) {
    if (req_ == nullptr) {
      LOG(ERROR) << folly::exceptionStr(*ew);
    } else {
      if (ewp_) {
        ewp_(std::move(req_), protoSeqId_, std::move(ctx_), ew);
      }
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

template <typename T>
class HandlerCallback : public HandlerCallbackBase {
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      std::unique_ptr<apache::thrift::ContextStack>,
      const T&);
 public:

  typedef T ResultType;

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

  void result(const T& r) {
    doResult(r);
  }
  void resultInThread(
      const T& r,
      SerializationThread st = SerializationThread::CURRENT) {
    if (st == SerializationThread::CURRENT) {
      result(r);
      delete this;
    } else {
      getEventBase()->runInEventBaseThread([=](){
          this->result(r);
          delete this;
      });
    }
  }
 protected:

  // Always called in IO thread
  virtual void doResult(const T& r) {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_,
                     std::move(this->ctx_),
                     r);
    transform(queue);
    sendReply(std::move(queue), r);
  }

  void sendReply(folly::IOBufQueue queue,
                 const T& r);

  cob_ptr cp_;
};

template <typename T>
void HandlerCallback<T>::sendReply(folly::IOBufQueue queue,
                                   const T& r) {
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

template <typename T>
class HandlerCallback<std::unique_ptr<T>> : public HandlerCallbackBase {
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      std::unique_ptr<apache::thrift::ContextStack>,
      const T&);
 public:

  typedef T ResultType;

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
    Cpp2RequestContext* reqCtx):
      HandlerCallbackBase(std::move(req), std::move(ctx), ep, ewp,
                          eb, tm, reqCtx),
      cp_(cp) {
    this->protoSeqId_ = protoSeqId;
  }

  void result(const T& r) {
    doResult(r);
  }
  void result(std::unique_ptr<T> r) {
    doResult(*r);
  }
  void resultInThread(
      T r,
      SerializationThread st = SerializationThread::CURRENT) {
    if (st == SerializationThread::CURRENT) {
      result(r);
      delete this;
    } else {
      auto r_mw = folly::makeMoveWrapper(std::move(r));
      getEventBase()->runInEventBaseThread([=](){
        this->result(*r_mw);
        delete this;
      });
    }
  }
  void resultInThread(
      std::unique_ptr<T> r,
      SerializationThread st = SerializationThread::CURRENT) {
    if (st == SerializationThread::CURRENT) {
      result(*r);
      delete this;
    } else {
      auto r_mw = folly::makeMoveWrapper(std::move(r));
      getEventBase()->runInEventBaseThread([=](){
        this->result(**r_mw);
        delete this;
      });
    }
  }
  void resultInThread(
      const std::shared_ptr<T>& r,
      SerializationThread st = SerializationThread::CURRENT) {
    if (st == SerializationThread::CURRENT) {
      result(*r);
      delete this;
    } else {
      getEventBase()->runInEventBaseThread([=](){
        this->result(*r);
        delete this;
      });
    }
  }
 protected:
  virtual void doResult(const T& r) {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_,
                     std::move(this->ctx_),
                     r);

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

  cob_ptr cp_;
};

template <>
class HandlerCallback<void> : public HandlerCallbackBase {
  typedef folly::IOBufQueue(*cob_ptr)(
      int32_t protoSeqId,
      std::unique_ptr<apache::thrift::ContextStack>);
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

  ~HandlerCallback(){}

  void done() {
    doDone();
  }
  void doneInThread(SerializationThread st = SerializationThread::CURRENT) {
    if (st == SerializationThread::CURRENT) {
      done();
      delete this;
    } else {
      assert(eb_);
      this->eb_->runInEventBaseThread([=](){
        this->done();
        delete this;
      });
    }
  }
 protected:
  virtual void doDone() {
    assert(cp_);
    auto queue = cp_(this->protoSeqId_,
                     std::move(this->ctx_));
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

  cob_ptr cp_;
};

class AsyncProcessorFactory {
  public:
    virtual std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() = 0;
    virtual ~AsyncProcessorFactory() {};
};

class ServerInterface : public AsyncProcessorFactory {
 public:
  virtual ~ServerInterface() {};

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

  virtual void setThreadEventBase(apache::thrift::async::TEventBase* eb) {}

  void setEventBase(apache::thrift::async::TEventBase* eb) {
    if (!eb_) {
      eb_ = eb;
      setThreadEventBase(eb_);
    } else {
      DCHECK(eb_ == eb);
    }
  }

  apache::thrift::async::TEventBase* getEventBase() {
    return eb_;
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
  apache::thrift::concurrency::ThreadManager* tm_;
  static __thread apache::thrift::async::TEventBase* eb_;
};

}} // apache::thrift

#endif // THRIFT_ASYNCPROCESSOR2_H
