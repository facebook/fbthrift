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

#ifndef THRIFT_EVENTHANDLERBASE_H_
#define THRIFT_EVENTHANDLERBASE_H_ 1

#include <string>
#include <vector>
#include <memory>
#include <thrift/lib/cpp/concurrency/Mutex.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/SocketAddress.h>

namespace folly {
class IOBuf;
}

namespace apache { namespace thrift { namespace async {
class TAsyncChannel;
}}}

namespace apache { namespace thrift {

using server::TConnectionContext;

extern std::shared_ptr<server::TServerObserverFactory> observerFactory_;

/**
 * Structure representing a serialized message, encoded with the given
 * protocol in the given buffer.
 */
struct SerializedMessage {
  protocol::PROTOCOL_TYPES protocolType;
  const folly::IOBuf* buffer;
};

/**
 * Virtual interface class that can handle events from the processor. To
 * use this you should subclass it and implement the methods that you care
 * about. Your subclass can also store local data that you may care about,
 * such as additional "arguments" to these methods (stored in the object
 * instance's state).
 */
class TProcessorEventHandler {
 public:

  virtual ~TProcessorEventHandler() {}

  /**
   * Called before calling other callback methods.
   * Expected to return some sort of context object.
   * The return value is passed to all other callbacks
   * for that function invocation.
   */
  virtual void* getServiceContext(const char* /*service_name*/,
                           const char* fn_name,
                           TConnectionContext* connectionContext) {
    return getContext(fn_name, connectionContext);
  }
  virtual void* getContext(const char* /*fn_name*/,
                           TConnectionContext* /*connectionContext*/) {
    return nullptr;
  }

  /**
   * Expected to free resources associated with a context.
   */
  virtual void freeContext(void* /*ctx*/, const char* /*fn_name*/) { }

  /**
   * Called before reading arguments.
   */
  virtual void preRead(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called before postRead, after reading arguments (server) / after reading
   * reply (client), with the actual (unparsed, serialized) data.
   *
   * The data is framed by message begin / end entries, call readMessageBegin /
   * readMessageEnd on the protocol.
   *
   * Only called for Cpp2.
   */
  virtual void onReadData(void* /*ctx*/, const char* /*fn_name*/,
                          const SerializedMessage& /*msg*/) {}

  /**
   * Called between reading arguments and calling the handler.
   */
  virtual void postRead(void* /*ctx*/,
                        const char* /*fn_name*/,
                        apache::thrift::transport::THeader* /*header*/,
                        uint32_t /*bytes*/) {}

  /**
   * Called between calling the handler and writing the response.
   */
  virtual void preWrite(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called before postWrite, after serializing response (server) / after
   * serializing request (client), with the actual (serialized) data.
   *
   * The data is framed by message begin / end entries, call readMessageBegin /
   * readMessageEnd on the protocol.
   *
   * Only called for Cpp2.
   */
  virtual void onWriteData(void* /*ctx*/, const char* /*fn_name*/,
                           const SerializedMessage& /*msg*/) {}

  /**
   * Called after writing the response.
   */
  virtual void postWrite(void* /*ctx*/,
                         const char* /*fn_name*/,
                         uint32_t /*bytes*/) {}

  /**
   * Called when an async function call completes successfully.
   */
  virtual void asyncComplete(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called if the handler throws an undeclared exception.
   */
  virtual void handlerError(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called if the handler throws a declared exception
   *
   * Only called for Cpp2
   */
  virtual void userException(void* /*ctx*/,
                             const char* /*fn_name*/,
                             const std::string& /*ex*/,
                             const std::string& /*ex_what*/) {}

 protected:
  TProcessorEventHandler() {}
};

/**
 * A helper class used by the generated code to free each context.
 */
class TProcessorContextFreer {
 public:
  TProcessorContextFreer(std::shared_ptr<TProcessorEventHandler> handler,
                         void* context, const char* method) :
    handler_(handler), context_(context), method_(method) {}
  ~TProcessorContextFreer() {
    if (handler_ != nullptr) {
      handler_->freeContext(context_, method_);
    }
  }

  void unregister() { handler_.reset(); }

 private:
  std::shared_ptr<TProcessorEventHandler> handler_;
  void* context_;
  const char* method_;
};

class ContextStack {
  friend class EventHandlerBase;

 public:
  explicit ContextStack(const char* method)
      : ctxs_()
      , handlers_()
      , method_(method) {}

  ContextStack(
    const std::shared_ptr<
      std::vector<std::shared_ptr<TProcessorEventHandler>>
      >& handlers,
    const char* serviceName,
    const char* method,
    TConnectionContext* connectionContext)
      : ctxs_()
      , handlers_(handlers)
      , method_(method) {
    for (auto handler: *handlers) {
      ctxs_.push_back(handler->getServiceContext(serviceName,
                                                 method, connectionContext));
    }
  }

  ContextStack(
    const std::shared_ptr<
      std::vector<std::shared_ptr<TProcessorEventHandler>>
      >& handlers,
    const char* method,
    TConnectionContext* connectionContext)
      : ctxs_()
      , handlers_(handlers)
      , method_(method) {
    for (auto handler: *handlers) {
      ctxs_.push_back(handler->getContext(method, connectionContext));
    }
  }

  ~ContextStack() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->freeContext(ctxs_[i], method_);
      }
    }
  }

  void preWrite() {
    if (handlers_) {
      for (size_t  i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->preWrite(ctxs_[i], method_);
      }
    }
  }

  void onWriteData(const SerializedMessage& msg) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->onWriteData(ctxs_[i], method_, msg);
      }
    }
  }

  void postWrite(uint32_t bytes) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->postWrite(ctxs_[i], method_, bytes);
      }
    }
  }

  void preRead() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->preRead(ctxs_[i], method_);
      }
    }
  }

  void onReadData(const SerializedMessage& msg) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->onReadData(ctxs_[i], method_, msg);
      }
    }
  }

  void postRead(apache::thrift::transport::THeader* header, uint32_t bytes) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->postRead(ctxs_[i], method_, header, bytes);
      }
    }
  }

  void handlerError() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->handlerError(ctxs_[i], method_);
      }
    }
  }

  void userException(const std::string& ex, const std::string& ex_what) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->userException(ctxs_[i], method_, ex, ex_what);
      }
    }
  }

  void asyncComplete() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->asyncComplete(ctxs_[i], method_);
      }
    }
  }

  const char* getMethod() {
    return method_;
  }

 private:
  std::vector<void*> ctxs_;
  std::shared_ptr<
    std::vector<std::shared_ptr<TProcessorEventHandler>>
    >handlers_;
  const char* method_;
};

class EventHandlerBase {
 public:
  EventHandlerBase()
    : handlers_(std::make_shared<
                std::vector<std::shared_ptr<TProcessorEventHandler> > >())
    , setEventHandlerPos_(-1) {}

  void addEventHandler(
      const std::shared_ptr<TProcessorEventHandler>& handler) {
    handlers_->push_back(handler);
  }

  void clearEventHandlers() {
    handlers_->clear();
    setEventHandlerPos_ = -1;
    if (eventHandler_) {
      setEventHandler(eventHandler_);
    }
  }

  std::shared_ptr<TProcessorEventHandler> getEventHandler() {
    return eventHandler_;
  }

  std::vector<std::shared_ptr<TProcessorEventHandler>>& getEventHandlers() {
    return *handlers_;
  }

  void setEventHandler(std::shared_ptr<TProcessorEventHandler> eventHandler) {
    eventHandler_ = eventHandler;
    if (setEventHandlerPos_ > 0) {
      handlers_->erase(handlers_->begin() + setEventHandlerPos_);
    }
    setEventHandlerPos_ = handlers_->size();
    handlers_->push_back(eventHandler);
  }

 protected:
  std::unique_ptr<ContextStack> getContextStack(
      const char* service_name,
      const char* fn_name,
      TConnectionContext* connectionContext) {
    std::unique_ptr<ContextStack> ctx(
      new ContextStack(handlers_, service_name, fn_name, connectionContext));
    return ctx;
  }

 public:
  std::shared_ptr<
    std::vector<std::shared_ptr<TProcessorEventHandler>>
    > handlers_;
  std::shared_ptr<TProcessorEventHandler> eventHandler_;

 private:
  int setEventHandlerPos_;

};

class TProcessorEventHandlerFactory {
 public:
  virtual ~TProcessorEventHandlerFactory() { }
  virtual std::shared_ptr<TProcessorEventHandler> getEventHandler() = 0;
};

/**
 * Base class for all thrift processors. Used to automatically attach event
 * handlers to processors at creation time.
 */
class TProcessorBase : public EventHandlerBase {
 public:
  TProcessorBase();

  static void addProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory);

  static void removeProcessorEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory);

 private:
  static concurrency::ReadWriteMutex& getRWMutex();

  static std::vector<std::shared_ptr<TProcessorEventHandlerFactory>>&
    getFactories();
};

/**
 * Base class for all thrift clients. Used to automatically attach event
 * handlers to clients at creation time.
 */
class TClientBase : public EventHandlerBase {
 public:
  TClientBase();

  // Explicit copy constructor to skip copying the current client context stack
  // (it's a unique pointer so it can't be copied, but it also wouldn't make
  // sense to copy it).
  TClientBase(const TClientBase& original) :
    EventHandlerBase(original),
    s_() {}

  virtual ~TClientBase() {}

  static void addClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory);

  static void removeClientEventHandlerFactory(
    std::shared_ptr<TProcessorEventHandlerFactory> factory);

  /**
   * These functions are only used in the client handler
   * implementation.  The server process functions maintain
   * ContextStack on the stack and binds ctx in to the async calls.
   *
   * Clients are not thread safe, so using a member variable is okay.
   * Client send_ and recv_ functions contain parameters based off of
   * the function call, and adding a parameter there would change the
   * function signature enough that other thrift users might break.
   *
   * The generated code should be the ONLY user of s_.  All other functions
   * should just use the ContextStack parameter.
   */
  void generateClientContextStack(const char* fn_name,
                                  TConnectionContext* connectionContext) {
    auto s = getContextStack("", fn_name, connectionContext);
    s_ = std::move(s);
  }
  void generateClientContextStack(const char* service_name,
                                  const char* fn_name,
                                  TConnectionContext* connectionContext) {
    auto s = getContextStack(service_name, fn_name, connectionContext);
    s_ = std::move(s);
  }

  void clearClientContextStack() {
    s_.reset();
  }

  ContextStack* getClientContextStack() {
    return s_.get();
  }

 protected:
  class ConnContext : public TConnectionContext {
   public:
    ConnContext(
        std::shared_ptr<protocol::TProtocol> inputProtocol,
        std::shared_ptr<protocol::TProtocol> outputProtocol);

    ConnContext(
        std::shared_ptr<apache::thrift::async::TAsyncChannel> channel,
        std::shared_ptr<protocol::TProtocol> inputProtocol,
        std::shared_ptr<protocol::TProtocol> outputProtocol);

    void init(const folly::SocketAddress* address,
              std::shared_ptr<protocol::TProtocol> inputProtocol,
              std::shared_ptr<protocol::TProtocol> outputProtocol);

    const folly::SocketAddress* getPeerAddress() const override {
      return &internalAddress_;
    }

    void reset() {
      internalAddress_.reset();
      address_ = nullptr;
      cleanupUserData();
    }

    std::shared_ptr<protocol::TProtocol> getInputProtocol() const override {
      return inputProtocol_;
    }

    std::shared_ptr<protocol::TProtocol> getOutputProtocol() const override {
      return outputProtocol_;
    }

   private:
    folly::SocketAddress* address_;
    folly::SocketAddress internalAddress_;
    std::shared_ptr<protocol::TProtocol> inputProtocol_;
    std::shared_ptr<protocol::TProtocol> outputProtocol_;
  };

 private:
  static concurrency::ReadWriteMutex& getRWMutex();

  static std::vector<std::shared_ptr<TProcessorEventHandlerFactory>>&
    getFactories();

  std::unique_ptr<ContextStack> s_;
};

}} // apache::thrift

#endif // #ifndef THRIFT_EVENTHANDLERBASE_H_
