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

#include <thrift/lib/cpp/util/ScopedServerThread.h>

#include <memory>
#include <utility>

#include <thrift/lib/cpp/concurrency/Monitor.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>

using std::shared_ptr;
using std::weak_ptr;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;

namespace apache { namespace thrift { namespace util {

/**
 * ScopedServerThread::Helper runs the server loop in the new server thread.
 *
 * It also provides a waitUntilStarted() method that the main thread can use to
 * block until the server has started listening for new connections.
 */
class ScopedServerThread::Helper : public Runnable,
                                   public TServerEventHandler {
 public:
  Helper() : state_(STATE_NOT_STARTED) {}

  ~Helper() override;

  void init(shared_ptr<TServer> server, shared_ptr<Helper> self);

  void run() override;

  /**
   * Stop the server.
   *
   * This may be called from the main thread.
   */
  void stop();

  /**
   * Get the address that the server is listening on.
   *
   * This may be called from the main thread.
   */
  const folly::SocketAddress* getAddress() const {
    return &address_;
  }

  /**
   * Wait until the server has started.
   *
   * Raises TException if the server failed to start.
   */
  void waitUntilStarted();

  void preServe(const folly::SocketAddress* address) override;

  const shared_ptr<TServer>& getServer() const {
    return server_;
  }

 private:
  enum StateEnum {
    STATE_NOT_STARTED,
    STATE_RUNNING,
    STATE_START_ERROR
  };

  class EventHandler : public TServerEventHandler {
   public:
    explicit EventHandler(const shared_ptr<Helper>& outer) : outer_(outer) {}
    void preServe(const folly::SocketAddress* address) override;
   private:
    weak_ptr<Helper> outer_;
  };

  // Helper class to polymorphically re-throw a saved exception type
  class SavedException {
   public:
    virtual ~SavedException() {}
    virtual void rethrow() = 0;
  };

  template<typename ExceptionT>
  class SavedExceptionImpl : public SavedException {
   public:
    explicit SavedExceptionImpl(const ExceptionT& x) : exception_(x) {}

    void rethrow() override { throw exception_; }

   private:
    ExceptionT exception_;
  };

  // Attempt to downcast to a specific type to avoid slicing.
  template<typename ExceptionT>
  bool tryHandleServeError(const std::exception& x) {
    auto e = dynamic_cast<const ExceptionT*>(&x);
    if (e) {
      handleServeError<ExceptionT>(*e);
    }
    return e;
  }

  // Copying exceptions is difficult because of the slicing problem. Match the
  // most commonly expected exception types, and try our best to copy as much
  // state as possible.
  void handleServeError(const std::exception& x) override {
    tryHandleServeError<TTransportException>(x) ||
    tryHandleServeError<TException>(x) ||
    tryHandleServeError<std::system_error>(x) ||
    tryHandleServeError<std::exception>(x);
  }

  template<typename ExceptionT>
  void handleServeError(const ExceptionT& x) {
    if (eventHandler_ && *eventHandler_) {
      (*eventHandler_)->handleServeError(x);
    }

    Synchronized s(stateMonitor_);

    if (state_ == STATE_NOT_STARTED) {
      // If the error occurred before the server started,
      // save a copy of the error and notify the main thread
      savedError_.reset(new SavedExceptionImpl<ExceptionT>(x));
      state_ = STATE_START_ERROR;
      stateMonitor_.notify();
    } else {
      // The error occurred during normal server execution.
      // Just log an error message.
      T_ERROR("ScopedServerThread: serve() raised a %s while running: %s",
              typeid(x).name(), x.what());
    }
  }

  StateEnum state_;
  Monitor stateMonitor_;

  shared_ptr<TServer> server_;
  // If the server event handler has been intercepted, then this field will be
  // set to the replaced event handler, which could be nullptr.
  folly::Optional<shared_ptr<TServerEventHandler>> eventHandler_;
  shared_ptr<SavedException> savedError_;
  folly::SocketAddress address_;
};

ScopedServerThread::Helper::~Helper() {
  if (eventHandler_) {
    server_->setServerEventHandler(*eventHandler_);
  }
}

void ScopedServerThread::Helper::init(shared_ptr<TServer> server,
    shared_ptr<Helper> self) {
  server_ = std::move(server);

  // Install ourself as the server event handler, so that our preServe() method
  // will be called once we've successfully bound to the port and are about to
  // start listening. If there is already an installed event handler, uninstall
  // it, but remember it so we can re-install it once the server starts.
  eventHandler_ = server_->getEventHandler();
  // This function takes self as an argument since we need a shared_ptr to
  // ourselves.
  server_->setServerEventHandler(std::make_shared<EventHandler>(self));
}

void ScopedServerThread::Helper::run() {
  // Call serve()
  //
  // If an error occurs before the server finishes starting, save the error and
  // notify the main thread of the failure.
  try {
    server_->serve();
  } catch (const std::exception& x) {
    handleServeError(x);
  } catch (...) {
    TServerEventHandler::handleServeError();
  }
}

void ScopedServerThread::Helper::stop() {
  server_->stop();
}

void ScopedServerThread::Helper::waitUntilStarted() {
  concurrency::Synchronized s(stateMonitor_);
  while (state_ == STATE_NOT_STARTED) {
    stateMonitor_.waitForever();
  }

  // If an error occurred starting the server,
  // throw the saved error in the main thread
  if (state_ == STATE_START_ERROR) {
    assert(savedError_);
    savedError_->rethrow();
  }
}

void ScopedServerThread::Helper::preServe(
    const folly::SocketAddress* address) {
  // Save a copy of the address
  address_ = *address;

  // The eventHandler_ should have been assigned in init, even if it is only
  // set to an empty shared_ptr.
  CHECK(eventHandler_);
  auto eventHandler = *eventHandler_;
  // Re-install the old eventHandler_.
  server_->setServerEventHandler(eventHandler);
  eventHandler_ = folly::none;
  if (eventHandler) {
    // Invoke preServe() on eventHandler,
    // since we intercepted the real preServe() call.
    eventHandler->preServe(address);
  }

  // Inform the main thread that the server has started
  concurrency::Synchronized s(stateMonitor_);
  assert(state_ == STATE_NOT_STARTED);
  state_ = STATE_RUNNING;
  stateMonitor_.notify();
}

void ScopedServerThread::Helper::EventHandler::preServe(
    const folly::SocketAddress* address) {
  auto outer = outer_.lock();
  CHECK(outer);
  outer->preServe(address);
}

/*
 * ScopedServerThread methods
 */

ScopedServerThread::ScopedServerThread() {
}

ScopedServerThread::ScopedServerThread(shared_ptr<TServer> server) {
  start(std::move(server));
}

ScopedServerThread::~ScopedServerThread() {
  stop();
}

void ScopedServerThread::start(shared_ptr<TServer> server) {
  if (helper_) {
    throw TLibraryException("ScopedServerThread is already running");
  }

  // Thrift forces us to use shared_ptr for both Runnable and
  // TServerEventHandler objects, so we have to allocate Helper on the heap,
  // rather than having it as a member variable or deriving from Runnable and
  // TServerEventHandler ourself.
  auto helper = std::make_shared<Helper>();

  helper->init(std::move(server), helper);

  // Start the thread
  PosixThreadFactory threadFactory;
  threadFactory.setDetached(false);
  auto thread = threadFactory.newThread(helper);
  thread->start();
  auto threadGuard = folly::makeGuard([&] {
    helper->stop();
    thread->join();
  });

  // Wait for the server to start
  helper->waitUntilStarted();

  helper_ = std::move(helper);
  thread_ = std::move(thread);
  threadGuard.dismiss();
}

void ScopedServerThread::stop() {
  if (!helper_) {
    return;
  }

  helper_->stop();
  thread_->join();
  helper_.reset();
  thread_.reset();
}

void ScopedServerThread::join() {
  thread_->join();
}

const folly::SocketAddress* ScopedServerThread::getAddress() const {
  if (!helper_) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "attempted to get address of stopped "
                              "ScopedServerThread");
  }

  return helper_->getAddress();
}

weak_ptr<TServer> ScopedServerThread::getServer() const {
  if (!helper_) {
    return weak_ptr<server::TServer>();
  }
  return helper_->getServer();
}

bool ScopedServerThread::setServeThreadName(const std::string& name) {
  return thread_->setName(name);
}

ServerStartHelper::ServerStartHelper(shared_ptr<TServer> server) {
  helper_ = std::make_shared<ScopedServerThread::Helper>();
  helper_->init(std::move(server), helper_);
}

void ServerStartHelper::waitUntilStarted() {
  helper_->waitUntilStarted();
}

}}} // apache::thrift::util
