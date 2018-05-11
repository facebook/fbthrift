/*
 * Copyright 2014-present Facebook, Inc.
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

#include <memory>
#include <string>
#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/ContextStack.h>
#include <thrift/lib/cpp/concurrency/Mutex.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>

namespace apache {
namespace thrift {
namespace async {
class TAsyncChannel;
}
} // namespace thrift
} // namespace apache

namespace apache {
namespace thrift {

using server::TConnectionContext;

extern std::shared_ptr<server::TServerObserverFactory> observerFactory_;

/**
 * A helper class used by the generated code to free each context.
 */
class TProcessorContextFreer {
 public:
  TProcessorContextFreer(
      std::shared_ptr<TProcessorEventHandler> handler,
      void* context,
      const char* method)
      : handler_(handler), context_(context), method_(method) {}
  ~TProcessorContextFreer() {
    if (handler_ != nullptr) {
      handler_->freeContext(context_, method_);
    }
  }

  void unregister() {
    handler_.reset();
  }

 private:
  std::shared_ptr<TProcessorEventHandler> handler_;
  void* context_;
  const char* method_;
};

class EventHandlerBase {
 public:
  EventHandlerBase()
      : handlers_(std::make_shared<
                  std::vector<std::shared_ptr<TProcessorEventHandler>>>()),
        setEventHandlerPos_(-1) {}

  void addEventHandler(const std::shared_ptr<TProcessorEventHandler>& handler) {
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
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  std::shared_ptr<TProcessorEventHandler> eventHandler_;

 private:
  int setEventHandlerPos_;
};

class TProcessorEventHandlerFactory {
 public:
  virtual ~TProcessorEventHandlerFactory() {}
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
  TClientBase(const TClientBase& original) : EventHandlerBase(original), s_() {}

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
  void generateClientContextStack(
      const char* fn_name,
      TConnectionContext* connectionContext) {
    auto s = getContextStack("", fn_name, connectionContext);
    s_ = std::move(s);
  }
  void generateClientContextStack(
      const char* service_name,
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

 private:
  static concurrency::ReadWriteMutex& getRWMutex();

  static std::vector<std::shared_ptr<TProcessorEventHandlerFactory>>&
  getFactories();

  std::unique_ptr<ContextStack> s_;
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_EVENTHANDLERBASE_H_
