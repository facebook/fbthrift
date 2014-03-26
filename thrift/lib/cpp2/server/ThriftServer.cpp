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

#include "thrift/lib/cpp2/server/ThriftServer.h"

#include "folly/Conv.h"
#include "folly/Memory.h"
#include "folly/Random.h"
#include "folly/ScopeGuard.h"
#include "thrift/lib/cpp2/server/Cpp2Connection.h"
#include "thrift/lib/cpp2/server/Cpp2Worker.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"

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
  "host",
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
  stopWorkersOnStopListening_(true) {

  if (FLAGS_sasl_policy == "required") {
    setSaslEnabled(true);
    setNonSaslEnabled(false);
  } else if (FLAGS_sasl_policy == "permitted") {
    setSaslEnabled(true);
    setNonSaslEnabled(true);
  }

  if (FLAGS_sasl_policy == "required" || FLAGS_sasl_policy == "permitted") {
    // Enable both secure / insecure connections
    char hostname[256];
    if (gethostname(hostname, 255)) {
      LOG(FATAL) << "Failed getting hostname";
      return;
    }
    setServicePrincipal(FLAGS_kerberos_service_name + "/" + hostname);
  }
}

ThriftServer::~ThriftServer() {
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

    // We always need a threadmanager for cpp2.
    if (!threadFactory_) {
      setThreadFactory(std::shared_ptr<ThreadFactory>(
        new PosixThreadFactory));
    }

    if (!threadManager_) {
      std::shared_ptr<apache::thrift::concurrency::ThreadManager>
        threadManager(PriorityThreadManager::newPriorityThreadManager(
                        nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_,
                        true /*stats*/));
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
    // (This is needed if we were supplied a pre-bound socket, or if address_'s
    // port was set to 0, so an ephemeral port was chosen by the kernel.)
    socket_->getAddress(&address_);

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

    socket_->attachEventBase(eventBaseManager_->getEventBase());
    eventBaseAttached = true;
    socket_->startAccepting();
  } catch (...) {
    // XXX: Cpp2Worker::acceptStopped() calls
    //      eventBase_.terminateLoopSoon().  Normally this stops the
    //      worker from processing more work and stops the event loop.
    //      However, if startConsuming() and eventBase_.loop() haven't
    //      run yet this won't do the right thing. The worker thread
    //      will still continue to call startConsuming() later, and
    //      will then start the event loop.
    for (uint32_t i = 0; i < threadsStarted; ++i) {
      workers_[i].worker->acceptStopped();
      workers_[i].thread->join();
    }
    workers_.clear();

    if (socket_) {
      if (eventBaseAttached) {
        socket_->detachEventBase();
      }

      socket_.reset();
    }

    throw;
  }
}

/**
 * Loop and accept incoming connections.
 */
void ThriftServer::serve() {
  setup();
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
  socket_->addAcceptCallback(info.worker.get(), info.worker->getEventBase());

  workers_.push_back(info);
}

void ThriftServer::stopWorkers() {
  for (auto& info : workers_) {
    info.worker->stopEventBase();
    info.thread->join();
    info.worker->closeConnections();
  }
}

void ThriftServer::immediateShutdown() {
  shutdownSocketSet_->shutdownAll();
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


}} // apache::thrift
