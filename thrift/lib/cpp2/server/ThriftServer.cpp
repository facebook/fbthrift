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
#include <folly/Logging.h>
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
using folly::wangle::IOThreadPoolExecutor;
using folly::wangle::NamedThreadFactory;

const int ThriftServer::T_ASYNC_DEFAULT_WORKER_THREADS =
  sysconf(_SC_NPROCESSORS_ONLN);

const std::chrono::milliseconds ThriftServer::DEFAULT_TASK_EXPIRE_TIME =
    std::chrono::milliseconds(5000);

const std::chrono::milliseconds ThriftServer::DEFAULT_TIMEOUT =
    std::chrono::milliseconds(60000);

ThriftServer::ThriftServer() :
  apache::thrift::server::TServer(
    std::shared_ptr<apache::thrift::server::TProcessor>()),
  port_(-1),
  saslEnabled_(false),
  nonSaslEnabled_(true),
  shutdownSocketSet_(
    folly::make_unique<folly::ShutdownSocketSet>()),
  serveEventBase_(nullptr),
  nWorkers_(T_ASYNC_DEFAULT_WORKER_THREADS),
  nPoolThreads_(0),
  threadStackSizeMB_(1),
  timeout_(DEFAULT_TIMEOUT),
  eventBaseManager_(nullptr),
  workerFactory_(std::make_shared<Cpp2WorkerFactory>(this)),
  ioThreadPool_(std::make_shared<IOThreadPoolExecutor>(0, workerFactory_)),
  taskExpireTime_(DEFAULT_TASK_EXPIRE_TIME),
  listenBacklog_(DEFAULT_LISTEN_BACKLOG),
  acceptRateAdjustSpeed_(0),
  maxNumMsgsInQueue_(T_MAX_NUM_MESSAGES_IN_QUEUE),
  maxConnections_(0),
  maxRequests_(
    apache::thrift::concurrency::ThreadManager::DEFAULT_MAX_QUEUE_SIZE),
  isUnevenLoad_(true),
  useClientTimeout_(true),
  activeRequests_(0),
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
  if (!eventBaseManager_) {
    eventBaseManager_ = TEventBaseManager::get();
  }
  return eventBaseManager_;
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
        std::make_shared<apache::thrift::concurrency::NumaThreadFactory>(
          -1, // default setNode
          threadStackSizeMB_
        )
      );
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
                        getMaxRequests() /*maxQueueLen*/,
                        threadStackSizeMB_));
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

      if (socket_) {
        socket_->attachEventBase(eventBaseManager_->getEventBase());
      }
      eventBaseAttached = true;

      // Resize the IO pool
      ioThreadPool_->setNumThreads(nWorkers_);

      if (socket_) {
        socket_->startAccepting();
      }
    } else {
      CHECK(ioThreadPool_->numThreads() == 0);
      duplexWorker_ = folly::make_unique<Cpp2Worker>(this, 0, serverChannel_);
    }
  } catch (...) {
    if (!serverChannel_) {
      ioThreadPool_->join();
    }

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

void ThriftServer::stopWorkers() {
  if (serverChannel_) {
    return;
  }
  ioThreadPool_->join();
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
  int32_t count = 0;
  workerFactory_->forEachWorker([&](const Cpp2Worker& worker){
    count += worker.getPendingCount();
  });
  return count;
}

bool ThriftServer::isOverloaded(uint32_t workerActiveRequests) {
  if (UNLIKELY(isOverloaded_())) {
    return true;
  }

  if (maxRequests_ > 0) {
    if (isUnevenLoad_) {
      return activeRequests_ + getPendingCount() >= maxRequests_;
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

int64_t ThriftServer::getLoad(const std::string& counter, bool check_custom) {
  if (check_custom && getLoad_) {
    return getLoad_(counter);
  }

  int reqload = 0;
  int connload = 0;
  int queueload = 0;
  if (maxRequests_ > 0) {
    reqload = (100*(activeRequests_ + getPendingCount()))
      / ((float)maxRequests_);
  }
  if (maxConnections_ > 0) {
    int32_t connections = 0;
    workerFactory_->forEachWorker([&](const Cpp2Worker& worker){
      connections += worker.getPendingCount();
    });
    connload = (100*connections) / (float)maxConnections_;
  }

  queueload = threadManager_->getCodel()->getLoad();

  int load = std::max({reqload, connload, queueload});
  FB_LOG_EVERY_MS(INFO, 1000*10)
    << workerFactory_->getNamePrefix()
    << ": Load is: " << reqload << "% requests "
    << connload << "% connections "
    << queueload << "% queue time"
    << " active reqs " << activeRequests_
    << " pending reqs  " << getPendingCount();
  return load;
}

std::thread ThriftServer::Cpp2WorkerFactory::newThread(
    folly::wangle::Func&& func) {
  return internalFactory_->newThread([=](){
    CHECK(!server_->serverChannel_);
    auto id = nextWorkerId_++;
    auto worker = std::make_shared<Cpp2Worker>(server_, id, nullptr);
    {
      folly::RWSpinLock::WriteHolder guard(workersLock_);
      workers_.insert({id, worker});
    }
    server_->socket_->getEventBase()->runInEventBaseThread([this, worker](){
      server_->socket_->addAcceptCallback(worker.get(), worker->getEventBase());
    });

    server_->getEventBaseManager()->setEventBase(worker->getEventBase(), false);
    func();
    server_->getEventBaseManager()->clearEventBase();

    worker->closeConnections();
    {
      folly::RWSpinLock::WriteHolder guard(workersLock_);
      workers_.erase(id);
    }
  });
}

void ThriftServer::Cpp2WorkerFactory::setInternalFactory(
    std::shared_ptr<NamedThreadFactory> internalFactory) {
  CHECK(workers_.empty());
  internalFactory_ = internalFactory;
}

void ThriftServer::Cpp2WorkerFactory::setNamePrefix(folly::StringPiece prefix) {
  CHECK(workers_.empty());
  internalFactory_->setNamePrefix(prefix);
}

folly::StringPiece ThriftServer::Cpp2WorkerFactory::getNamePrefix() {
  return namePrefix_;
}

template <typename F>
void ThriftServer::Cpp2WorkerFactory::forEachWorker(F&& f) {
  folly::RWSpinLock::ReadHolder guard(workersLock_);
  for (const auto& kv : workers_) {
    f(*kv.second);
  }
}

}} // apache::thrift
