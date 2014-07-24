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

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/Random.h>
#include <folly/ScopeGuard.h>
#include <thrift/lib/cpp2/async/GssSaslServer.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/concurrency/NumaThreadManager.h>

#include <boost/thread/barrier.hpp>

#include <iostream>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

DEFINE_string(sasl_policy, "permitted",
             "SASL handshake required / permitted / disabled");

DEFINE_string(
  kerberos_service_name,
  "",
  "The service part of the principal name for the service");

namespace apache { namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using std::shared_ptr;
using apache::thrift::async::TEventBaseManager;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::concurrency::PriorityThreadManager;

const int ThriftServer::T_ASYNC_DEFAULT_WORKER_THREADS =
  sysconf(_SC_NPROCESSORS_ONLN);

const std::chrono::milliseconds ThriftServer::DEFAULT_TASK_EXPIRE_TIME =
    std::chrono::milliseconds(5000);

const std::chrono::milliseconds ThriftServer::DEFAULT_TIMEOUT =
    std::chrono::milliseconds(60000);

std::atomic<int32_t> ThriftServer::globalActiveRequests_(0);

ThriftServer::ThriftServer() :
  apache::thrift::server::TServer(
    std::shared_ptr<apache::thrift::server::TProcessor>()),
  cpp2WorkerThreadName_("Cpp2Worker"),
  port_(-1),
  saslEnabled_(false),
  nonSaslEnabled_(true),
  shutdownSocketSet_(
    folly::make_unique<apache::thrift::ShutdownSocketSet>()),
  serveEventBase_(nullptr),
  nWorkers_(T_ASYNC_DEFAULT_WORKER_THREADS),
  nPoolThreads_(0),
  timeout_(DEFAULT_TIMEOUT),
  eventBaseManager_(nullptr),
  workerChoice_(0),
  taskExpireTime_(DEFAULT_TASK_EXPIRE_TIME),
  listenBacklog_(DEFAULT_LISTEN_BACKLOG),
  acceptRateAdjustSpeed_(0),
  maxNumMsgsInQueue_(T_MAX_NUM_MESSAGES_IN_QUEUE),
  maxConnections_(0),
  maxRequests_(
    apache::thrift::concurrency::ThreadManager::DEFAULT_MAX_QUEUE_SIZE),
  isUnevenLoad_(true),
  useClientTimeout_(true),
  minCompressBytes_(0),
  isOverloaded_([]() { return false; }),
  queueSends_(true),
  enableCodel_(false),
  stopWorkersOnStopListening_(true),
  isDuplex_(false) {

  // SASL setup
  if (FLAGS_sasl_policy == "required") {
    setSaslEnabled(true);
    setNonSaslEnabled(false);
  } else if (FLAGS_sasl_policy == "permitted") {
    setSaslEnabled(true);
    setNonSaslEnabled(true);
  }
}

ThriftServer::ThriftServer(
    const std::shared_ptr<HeaderServerChannel>& serverChannel)
    : ThriftServer()
{
  serverChannel_ = serverChannel;
  nWorkers_ = 1;
  timeout_ = std::chrono::milliseconds(0);
}

ThriftServer::~ThriftServer() {
  if (saslThreadManager_) {
    saslThreadManager_->stop();
  }

  if (stopWorkersOnStopListening_) {
    // Everything is already taken care of.
    return;
  }
  // If the flag is false, neither i/o nor CPU workers aren't stopped at this
  // point. Stop them now.
  threadManager_->join();
  stopWorkers();
}

void ThriftServer::useExistingSocket(TAsyncServerSocket::UniquePtr socket) {
  if (socket_ == nullptr) {
    socket_ = std::move(socket);
    socket_->setShutdownSocketSet(shutdownSocketSet_.get());
    socket_->getAddress(&address_);
  }
}

void ThriftServer::useExistingSockets(const std::vector<int>& sockets) {
  TAsyncServerSocket::UniquePtr socket(new TAsyncServerSocket);
  socket->useExistingSockets(sockets);
  useExistingSocket(std::move(socket));
}

void ThriftServer::useExistingSocket(int socket) {
  useExistingSockets({socket});
}

std::vector<int> ThriftServer::getListenSockets() const {
  return (socket_ != nullptr) ? socket_->getSockets() : std::vector<int>{};
}

int ThriftServer::getListenSocket() const {
  if (socket_ != nullptr) {
    std::vector<int> sockets = socket_->getSockets();
    CHECK(sockets.size() == 1);
    return sockets[0];
  }

  return -1;
}

TEventBaseManager* ThriftServer::getEventBaseManager() {
  // It is legal to call this before setup().  No atomics needed -- this
  // transitions only once from NULL to non-NULL.
  if (!eventBaseManager_) {
    std::lock_guard<std::mutex> lock(ebmMutex_);
    if (!eventBaseManager_) {
      eventBaseManagerHolder_.reset(new TEventBaseManager);
      eventBaseManager_ = eventBaseManagerHolder_.get();
    }
  }
  return eventBaseManager_;
}

void ThriftServer::setEventBaseManager(TEventBaseManager* ebm) {
  std::lock_guard<std::mutex> lock(ebmMutex_);
  assert(!eventBaseManager_);
  eventBaseManager_ = ebm;
}

void ThriftServer::setup() {
  DCHECK_NOTNULL(cpp2Pfac_.get());
  DCHECK_GT(nWorkers_, 0);

  uint32_t threadsStarted = 0;
  bool eventBaseAttached = false;

  // Make sure EBM exists if we haven't set one explicitly
  getEventBaseManager();

  // Initialize event base for this thread, ensure event_init() is called
  serveEventBase_ = eventBaseManager_->getEventBase();
  // Print some libevent stats
  LOG(INFO) << "libevent " <<
    TEventBase::getLibeventVersion() << " method " <<
    TEventBase::getLibeventMethod();

  try {
    // We check for write success so we don't need or want SIGPIPEs.
    signal(SIGPIPE, SIG_IGN);

    if (!observer_ && apache::thrift::observerFactory_) {
      observer_ = apache::thrift::observerFactory_->getObserver();
    }

    // bind to the socket
    if (!serverChannel_) {
      if (socket_ == nullptr) {
        socket_.reset(new TAsyncServerSocket());
        socket_->setShutdownSocketSet(shutdownSocketSet_.get());
        if (port_ != -1) {
          socket_->bind(port_);
        } else {
          DCHECK(address_.isInitialized());
          socket_->bind(address_);
        }
      }

      socket_->listen(listenBacklog_);
      socket_->setMaxNumMessagesInQueue(maxNumMsgsInQueue_);
      socket_->setAcceptRateAdjustSpeed(acceptRateAdjustSpeed_);
    }

    // We always need a threadmanager for cpp2.
    if (!threadFactory_) {
      setThreadFactory(
        std::make_shared<apache::thrift::concurrency::NumaThreadFactory>());
    }

    if (FLAGS_sasl_policy == "required" || FLAGS_sasl_policy == "permitted") {
      if (!saslThreadManager_) {
        saslThreadManager_ = ThreadManager::newSimpleThreadManager(
          nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_, /* count */
          0, /* pendingTaskCountMax -- no limit */
          false, /* enableTaskStats */
          0 /* maxQueueLen -- large default */);
        saslThreadManager_->setNamePrefix("thrift-sasl");
        saslThreadManager_->threadFactory(threadFactory_);
        saslThreadManager_->start();
      }
      auto saslThreadManager = saslThreadManager_;

      if (getSaslServerFactory()) {
        // If the factory is already set, don't override it with the default
      } else if (FLAGS_kerberos_service_name.empty()) {
        // If the service name is not specified, not need to pin the principal.
        // Allow the server to accept anything in the keytab.
        setSaslServerFactory([=] (TEventBase* evb) {
          return std::unique_ptr<SaslServer>(
            new GssSaslServer(evb, saslThreadManager));
        });
      } else {
        char hostname[256];
        if (gethostname(hostname, 255)) {
          LOG(FATAL) << "Failed getting hostname";
        }
        setSaslServerFactory([=] (TEventBase* evb) {
          auto saslServer = std::unique_ptr<SaslServer>(
            new GssSaslServer(evb, saslThreadManager));
          saslServer->setServiceIdentity(
            FLAGS_kerberos_service_name + "/" + hostname);
          return std::move(saslServer);
        });
      }
    }

    if (!threadManager_) {
      std::shared_ptr<apache::thrift::concurrency::ThreadManager>
        threadManager(new apache::thrift::concurrency::NumaThreadManager(
                        nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_,
                        true /*stats*/,
                        getMaxRequests() /*maxQueueLen*/));
      threadManager->enableCodel(getEnableCodel());
      if (!poolThreadName_.empty()) {
        threadManager->setNamePrefix(poolThreadName_);
      }
      threadManager->start();
      setThreadManager(threadManager);
    }
    threadManager_->setExpireCallback([&](std::shared_ptr<Runnable> r) {
        EventTask* task = dynamic_cast<EventTask*>(r.get());
        if (task) {
          task->expired();
        }
    });
    threadManager_->setCodelCallback([&](std::shared_ptr<Runnable> r) {
        auto observer = getObserver();
        if (observer) {
          observer->queueTimeout();
        }
    });

    if (!serverChannel_) {
      // regular server
      auto b = std::make_shared<boost::barrier>(nWorkers_ + 1);

      // Create the worker threads.
      workers_.reserve(nWorkers_);
      for (uint32_t n = 0; n < nWorkers_; ++n) {
        addWorker();
        workers_[n].worker->getEventBase()->runInLoop([b](){
          b->wait();
        });
      }

      // Update address_ with the address that we are actually bound to.
      // (This is needed if we were supplied a pre-bound socket, or if
      // address_'s port was set to 0, so an ephemeral port was chosen by
      // the kernel.)
      if (socket_) {
        socket_->getAddress(&address_);
      }

      // Notify handler of the preServe event
      if (eventHandler_ != nullptr) {
        eventHandler_->preServe(&address_);
      }

      for (auto& worker: workers_) {
        worker.thread->start();
        ++threadsStarted;
        worker.thread->setName(folly::to<std::string>(cpp2WorkerThreadName_,
                                                      threadsStarted));
      }

      // Wait for all workers to start
      b->wait();

      if (socket_) {
        socket_->attachEventBase(eventBaseManager_->getEventBase());
      }
      eventBaseAttached = true;
      if (socket_) {
        socket_->startAccepting();
      }
    } else {
      // duplex server
      // Create the Cpp2Worker
      DCHECK(workers_.empty());
      WorkerInfo info;
      uint32_t workerID = 0;
      info.worker.reset(new Cpp2Worker(this, workerID, serverChannel_));
      // no thread, use current one (shared with client)
      info.thread = nullptr;

      workers_.push_back(info);
    }
  } catch (...) {
    // XXX: Cpp2Worker::acceptStopped() calls
    //      eventBase_.terminateLoopSoon().  Normally this stops the
    //      worker from processing more work and stops the event loop.
    //      However, if startConsuming() and eventBase_.loop() haven't
    //      run yet this won't do the right thing. The worker thread
    //      will still continue to call startConsuming() later, and
    //      will then start the event loop.
    if (!serverChannel_) {
      for (uint32_t i = 0; i < threadsStarted; ++i) {
        workers_[i].worker->acceptStopped();
        workers_[i].thread->join();
      }
    }
    workers_.clear();

    if (socket_) {
      if (eventBaseAttached) {
        socket_->detachEventBase();
      }

      socket_.reset();
    }

    // avoid crash on stop()
    serveEventBase_ = nullptr;

    throw;
  }
}

/**
 * Loop and accept incoming connections.
 */
void ThriftServer::serve() {
  setup();
  if (serverChannel_ != nullptr) {
    // A duplex server (the one running on a client) doesn't uses its own EB
    // since it reuses the client's EB
    return;
  }
  SCOPE_EXIT { this->cleanUp(); };
  eventBaseManager_->getEventBase()->loop();
}

void ThriftServer::cleanUp() {
  // It is users duty to make sure that setup() call
  // should have returned before doing this cleanup
  serveEventBase_ = nullptr;
  stopListening();

  if (stopWorkersOnStopListening_) {
    // Wait on the i/o worker threads to actually stop
    stopWorkers();
  }
}

uint64_t ThriftServer::getNumDroppedConnections() const {
  if (!socket_) {
    return 0;
  }
  return socket_->getNumDroppedConnections();
}

void ThriftServer::stop() {
  TEventBase* eventBase = serveEventBase_;
  if (eventBase) {
    eventBase->terminateLoopSoon();
  }
}

void ThriftServer::stopListening() {
  if (socket_ != nullptr) {
    // Stop accepting new connections
    socket_->pauseAccepting();
    socket_->detachEventBase();

    if (stopWorkersOnStopListening_) {
      // Wait for any tasks currently running on the task queue workers to
      // finish, then stop the task queue workers. Have to do this now, so
      // there aren't tasks completing and trying to write to i/o thread
      // workers after we've stopped the i/o workers.
      threadManager_->join();
    }

    // Close the listening socket. This will also cause the workers to stop.
    socket_.reset();

    // Return now and don't wait for worker threads to stop
  }
}

void ThriftServer::addWorker() {
  // Create the Cpp2Worker
  WorkerInfo info;
  uint32_t workerID = workers_.size();
  info.worker.reset(new Cpp2Worker(this, workerID));

  // Create the thread
  info.thread = threadFactory_->newThread(info.worker, ThreadFactory::ATTACHED);

  // Add the worker as an accept callback
  if (socket_) {
    socket_->addAcceptCallback(info.worker.get(), info.worker->getEventBase());
  }

  workers_.push_back(info);
}

void ThriftServer::stopWorkers() {
  if (serverChannel_) {
    return;
  }
  for (auto& info : workers_) {
    info.worker->stopEventBase();
    info.thread->join();
    info.worker->closeConnections();
  }
}

void ThriftServer::immediateShutdown(bool abortConnections) {
  shutdownSocketSet_->shutdownAll(abortConnections);
}

void ThriftServer::CumulativeFailureInjection::set(
    const FailureInjection& fi) {
  CHECK_GE(fi.errorFraction, 0);
  CHECK_GE(fi.dropFraction, 0);
  CHECK_GE(fi.disconnectFraction, 0);
  CHECK_LE(fi.errorFraction + fi.dropFraction + fi.disconnectFraction, 1);

  std::lock_guard<std::mutex> lock(mutex_);
  errorThreshold_ = fi.errorFraction;
  dropThreshold_ = errorThreshold_ + fi.dropFraction;
  disconnectThreshold_ = dropThreshold_ + fi.disconnectFraction;
  empty_.store((disconnectThreshold_ == 0), std::memory_order_relaxed);
}

ThriftServer::InjectedFailure
ThriftServer::CumulativeFailureInjection::test() const {
  if (empty_.load(std::memory_order_relaxed)) {
    return InjectedFailure::NONE;
  }

  static folly::ThreadLocalPtr<std::mt19937> rng;
  if (!rng) {
    rng.reset(new std::mt19937(folly::randomNumberSeed()));
  }

  std::uniform_real_distribution<float> dist(0, 1);
  float val = dist(*rng);

  std::lock_guard<std::mutex> lock(mutex_);
  if (val <= errorThreshold_) {
    return InjectedFailure::ERROR;
  } else if (val <= dropThreshold_) {
    return InjectedFailure::DROP;
  } else if (val <= disconnectThreshold_) {
    return InjectedFailure::DISCONNECT;
  }
  return InjectedFailure::NONE;
}

int32_t ThriftServer::getPendingCount() const {
  int pendingCount = 0;
  for (const auto& worker : workers_) {
    pendingCount += worker.worker->getPendingCount();
  }
  return pendingCount;
}

bool ThriftServer::isOverloaded(uint32_t workerActiveRequests) {
  if (UNLIKELY(isOverloaded_())) {
    return true;
  }

  if (maxRequests_ > 0) {
    if (isUnevenLoad_) {
      return globalActiveRequests_ + getPendingCount() >= maxRequests_;
    } else {
      return workerActiveRequests >= maxRequests_ / nWorkers_;
    }
  }

  return false;
}

std::chrono::milliseconds ThriftServer::getTaskExpireTimeForRequest(
  const apache::thrift::transport::THeader& requestHeader
) const {
  auto timeoutTime = getTaskExpireTime();
  if (getUseClientTimeout()) {
    auto clientTimeoutTime = requestHeader.getClientTimeout();
    if (clientTimeoutTime > std::chrono::milliseconds(0) &&
        clientTimeoutTime < timeoutTime) {
      timeoutTime = clientTimeoutTime;
    }
  }
  return timeoutTime;
}

}} // apache::thrift
