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
#include <folly/portability/Sockets.h>
#include <thrift/lib/cpp2/async/GssSaslServer.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

#include <wangle/ssl/SSLContextManager.h>

#include <iostream>
#include <random>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include <signal.h>

DEFINE_string(sasl_policy, "permitted",
             "SASL handshake required / permitted / disabled");

DEFINE_string(thrift_ssl_policy, "disabled",
  "SSL required / permitted / disabled");

DEFINE_string(
  service_identity,
  "",
  "The name of the service. Associates the service with ACLs and keys");
DEFINE_bool(
  pin_service_identity,
  false,
  "Force the service to use keys associated with the service_identity. "
  "Set this only if you're setting service_identity.");

namespace apache { namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using std::shared_ptr;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::concurrency::PriorityThreadManager;
using wangle::IOThreadPoolExecutor;
using wangle::NamedThreadFactory;

namespace {
struct DisableKerberosReplayCacheSingleton {
  DisableKerberosReplayCacheSingleton() {
    // Disable replay caching since we're doing mutual auth. Enabling
    // this will significantly degrade perf. Force this to overwrite
    // existing env variables to avoid performance regressions.
    setenv("KRB5RCACHETYPE", "none", 1);
  }
} kDisableKerberosReplayCacheSingleton;
}

class ThriftAcceptorFactory : public wangle::AcceptorFactory {
 public:
  explicit ThriftAcceptorFactory(ThriftServer* server)
      : server_(server) {}

  std::shared_ptr<wangle::Acceptor> newAcceptor(
      folly::EventBase* eventBase) override {
    return Cpp2Worker::create(server_, nullptr, eventBase);
  }
 private:
  ThriftServer* server_;
};

ThriftServer::ThriftServer() :
  ThriftServer("", false) {}

ThriftServer::ThriftServer(const std::string& saslPolicy,
                           bool allowInsecureLoopback)
  : BaseThriftServer()
  , saslPolicy_(saslPolicy.empty() ? FLAGS_sasl_policy : saslPolicy)
  , allowInsecureLoopback_(allowInsecureLoopback)
  , lastRequestTime_(monotonic_clock::now().time_since_epoch().count())
{
  // SASL setup
  if (saslPolicy_ == "required") {
    setSaslEnabled(true);
    setNonSaslEnabled(false);
  } else if (saslPolicy_ == "permitted") {
    setSaslEnabled(true);
    setNonSaslEnabled(true);
  }

  if (FLAGS_thrift_ssl_policy == "required") {
    sslPolicy_ = SSLPolicy::REQUIRED;
  } else if (FLAGS_thrift_ssl_policy == "permitted") {
    sslPolicy_ = SSLPolicy::PERMITTED;
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

  if (duplexWorker_) {
    // usually ServerBootstrap::stop drains the workers, but ServerBootstrap
    // doesn't know about duplexWorker_
    duplexWorker_->drainAllConnections();
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

void ThriftServer::useExistingSocket(folly::AsyncServerSocket::UniquePtr socket)
{
  socket_ = std::move(socket);
}

void ThriftServer::useExistingSockets(const std::vector<int>& sockets) {
  folly::AsyncServerSocket::UniquePtr socket(new folly::AsyncServerSocket);
  socket->useExistingSockets(sockets);
  useExistingSocket(std::move(socket));
}

void ThriftServer::useExistingSocket(int socket) {
  useExistingSockets({socket});
}

std::vector<int> ThriftServer::getListenSockets() const {
  std::vector<int> sockets;
  for (const auto  & socket : getSockets()) {
    auto newsockets = socket->getSockets();
    sockets.insert(sockets.end(), newsockets.begin(), newsockets.end());
  }
  return sockets;
}

int ThriftServer::getListenSocket() const {
  std::vector<int> sockets = getListenSockets();
  if (sockets.size() == 0) {
    return -1;
  }

  CHECK(sockets.size() == 1);
  return sockets[0];
}

folly::EventBaseManager* ThriftServer::getEventBaseManager() {
  return eventBaseManager_;
}

ThriftServer::IdleServerAction::IdleServerAction(
  ThriftServer &server,
  folly::HHWheelTimer &timer,
  std::chrono::milliseconds timeout
):
  server_(server),
  timer_(timer),
  timeout_(timeout)
{
  timer_.scheduleTimeout(this, timeout_);
}

void ThriftServer::IdleServerAction::timeoutExpired() noexcept {
  try {
    auto const lastRequestTime = server_.lastRequestTime();
    if (lastRequestTime.time_since_epoch()
        != monotonic_clock::duration::zero()) {
      auto const elapsed = monotonic_clock::now() - lastRequestTime;
      if (elapsed >= timeout_) {
        LOG(INFO) << "shutting down server due to inactivity after "
          << std::chrono::duration_cast<std::chrono::milliseconds>(
            elapsed).count() << "ms";
        server_.stop();
        return;
      }
    }

    timer_.scheduleTimeout(this, timeout_);
  } catch (std::exception const &e) {
    LOG(ERROR) << e.what();
  }
}

ThriftServer::monotonic_clock::time_point
ThriftServer::lastRequestTime() const noexcept {
  return monotonic_clock::time_point(
    monotonic_clock::duration(
      lastRequestTime_.load(std::memory_order_acquire)
    )
  );
}

void ThriftServer::touchRequestTimestamp() noexcept {
  if (idleServer_.hasValue()) {
    lastRequestTime_.store(
      monotonic_clock::now().time_since_epoch().count(),
      std::memory_order_release
    );
  }
}

void ThriftServer::setup() {
  DCHECK_NOTNULL(cpp2Pfac_.get());
  DCHECK_GT(nWorkers_, 0);

  uint32_t threadsStarted = 0;

  // Initialize event base for this thread, ensure event_init() is called
  serveEventBase_ = eventBaseManager_->getEventBase();
  if (idleServerTimeout_.count() > 0) {
    serverTimer_ = folly::HHWheelTimer::newTimer(serveEventBase_);
    idleServer_.emplace(*this, *serverTimer_, idleServerTimeout_);
  }
  // Print some libevent stats
  VLOG(1) << "libevent " <<
    folly::EventBase::getLibeventVersion() << " method " <<
    folly::EventBase::getLibeventMethod();

  try {
    // We check for write success so we don't need or want SIGPIPEs.
    signal(SIGPIPE, SIG_IGN);

    if (!observer_ && apache::thrift::observerFactory_) {
      observer_ = apache::thrift::observerFactory_->getObserver();
    }

    // We always need a threadmanager for cpp2.
    if (!threadFactory_) {
      setThreadFactory(
        std::make_shared<apache::thrift::concurrency::PosixThreadFactory>(
          apache::thrift::concurrency::PosixThreadFactory::kDefaultPolicy,
          apache::thrift::concurrency::PosixThreadFactory::kDefaultPriority,
          threadStackSizeMB_
        )
      );
    }

    if (saslPolicy_ == "required" || saslPolicy_ == "permitted") {
      if (!saslThreadManager_) {
        auto numThreads = nSaslPoolThreads_ > 0
                              ? nSaslPoolThreads_
                              : (nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_);
        saslThreadManager_ = ThreadManager::newSimpleThreadManager(
            numThreads,
            0, /* pendingTaskCountMax -- no limit */
            false, /* enableTaskStats */
            0 /* maxQueueLen -- large default */);
        saslThreadManager_->setNamePrefix(saslThreadsNamePrefix_);
        saslThreadManager_->threadFactory(threadFactory_);
        saslThreadManager_->start();
      }
      auto saslThreadManager = saslThreadManager_;

      if (getSaslServerFactory()) {
        // If the factory is already set, don't override it with the default
      } else if (FLAGS_pin_service_identity &&
                 !FLAGS_service_identity.empty()) {
        // If pin_service_identity flag is set and service_identity is specified
        // force the server use the corresponding principal from keytab.
        char hostname[256];
        if (gethostname(hostname, 255)) {
          LOG(FATAL) << "Failed getting hostname";
        }
        setSaslServerFactory([=] (folly::EventBase* evb) {
          auto saslServer = std::unique_ptr<SaslServer>(
            new GssSaslServer(evb, saslThreadManager));
          saslServer->setServiceIdentity(
            FLAGS_service_identity + "/" + hostname);
          return saslServer;
        });
      } else {
        // Allow the server to accept anything in the keytab.
        setSaslServerFactory([=] (folly::EventBase* evb) {
          return std::unique_ptr<SaslServer>(
            new GssSaslServer(evb, saslThreadManager));
        });
      }
    }

    if (!threadManager_) {
      int numThreads = nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_;
      std::shared_ptr<apache::thrift::concurrency::ThreadManager>
        threadManager(PriorityThreadManager::newPriorityThreadManager(
                        numThreads,
                        true /*stats*/,
                        getMaxRequests() + numThreads /*maxQueueLen*/));
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
          if (getEnableCodel()) {
            observer->queueTimeout();
          } else {
            observer->shadowQueueTimeout();
          }
        }
    });

    if (!serverChannel_) {

      ServerBootstrap::socketConfig.acceptBacklog = listenBacklog_;
      if (enableTFO_) {
        ServerBootstrap::socketConfig.enableTCPFastOpen = *enableTFO_;
        ServerBootstrap::socketConfig.fastOpenQueueSize = fastOpenQueueSize_;
      }

      // Resize the IO pool
      ioThreadPool_->setNumThreads(nWorkers_);

      ServerBootstrap::childHandler(
          acceptorFactory_ ? acceptorFactory_
                           : std::make_shared<ThriftAcceptorFactory>(this));

      {
        std::lock_guard<std::mutex> lock(ioGroupMutex_);
        ServerBootstrap::group(acceptPool_, ioThreadPool_);
      }
      if (socket_) {
        ServerBootstrap::bind(std::move(socket_));
      } else if (port_ != -1) {
        ServerBootstrap::bind(port_);
      } else {
        ServerBootstrap::bind(address_);
      }
      // Update address_ with the address that we are actually bound to.
      // (This is needed if we were supplied a pre-bound socket, or if
      // address_'s port was set to 0, so an ephemeral port was chosen by
      // the kernel.)
      ServerBootstrap::getSockets()[0]->getAddress(&address_);

      for (auto& socket : getSockets()) {
        socket->setShutdownSocketSet(shutdownSocketSet_.get());
        socket->setMaxNumMessagesInQueue(maxNumPendingConnectionsPerWorker_);
        socket->setAcceptRateAdjustSpeed(acceptRateAdjustSpeed_);
      }

      // Notify handler of the preServe event
      if (eventHandler_ != nullptr) {
        eventHandler_->preServe(&address_);
      }

    } else {
      CHECK(configMutable());
      duplexWorker_ = Cpp2Worker::create(this, serverChannel_);
      // we don't control the EventBase for the duplexWorker, so when we shut
      // it down, we need to ensure there's no delay
      duplexWorker_->setGracefulShutdownTimeout(std::chrono::milliseconds(0));
    }

    // Do not allow setters to be called past this point until the IO worker
    // threads have been joined in stopWorkers().
    configMutable_ = false;
  } catch (std::exception& ex) {
    // This block allows us to investigate the exception using gdb
    LOG(ERROR) << "Got an exception while setting up the server: "
      << ex.what();
    handleSetupFailure();
    throw;
  } catch (...) {
    handleSetupFailure();
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
  eventBaseManager_->getEventBase()->loopForever();
}

void ThriftServer::cleanUp() {
  DCHECK(!serverChannel_);
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
  uint64_t droppedConns = 0;
  for (auto& socket : getSockets()) {
    droppedConns += socket->getNumDroppedConnections();
  }
  return droppedConns;
}

void ThriftServer::stop() {
  folly::EventBase* eventBase = serveEventBase_;
  if (eventBase) {
    eventBase->terminateLoopSoon();
  }
}

void ThriftServer::stopListening() {
  for (auto& socket : getSockets()) {
    // Stop accepting new connections
    socket->getEventBase()->runInEventBaseThreadAndWait([&](){
      socket->pauseAccepting();

      // Close the listening socket. This will also cause the workers to stop.
      socket->stopAccepting();
    });
  }

  if (stopWorkersOnStopListening_) {
    // Wait for any tasks currently running on the task queue workers to
    // finish, then stop the task queue workers. Have to do this now, so
    // there aren't tasks completing and trying to write to i/o thread
    // workers after we've stopped the i/o workers.
    threadManager_->join();
  }
}

void ThriftServer::stopWorkers() {
  if (serverChannel_) {
    return;
  }
  DCHECK(!duplexWorker_);
  ServerBootstrap::stop();
  ServerBootstrap::join();
  configMutable_ = true;
}

void ThriftServer::handleSetupFailure(void) {
  ServerBootstrap::stop();

  // avoid crash on stop()
  serveEventBase_ = nullptr;
}


void ThriftServer::immediateShutdown(bool abortConnections) {
  shutdownSocketSet_->shutdownAll(abortConnections);
}

int32_t ThriftServer::getPendingCount() const {
  int32_t count = 0;
  if (!getIOGroupSafe()) { // Not enabled in duplex mode
    return 0;
  }
  forEachWorker([&](wangle::Acceptor* acceptor) {
    auto worker = dynamic_cast<Cpp2Worker*>(acceptor);
    count += worker->getPendingCount();
  });

  return count;
}

void ThriftServer::updateTicketSeeds(wangle::TLSTicketKeySeeds seeds) {
  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (!acceptor) {
      return;
    }
    auto evb = acceptor->getEventBase();
    if (!evb) {
      return;
    }
    evb->runInEventBaseThread([acceptor, seeds] {
      auto ctxMgr = acceptor->getSSLContextManager();
      if (!ctxMgr) {
        return;
      }
      ctxMgr->reloadTLSTicketKeys(
          seeds.oldSeeds, seeds.currentSeeds, seeds.newSeeds);
    });
  });
}

void ThriftServer::updateTLSCert() {
  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (!acceptor) {
      return;
    }
    auto evb = acceptor->getEventBase();
    if (!evb) {
      return;
    }
    evb->runInEventBaseThread([acceptor] {
      acceptor->resetSSLContextConfigs();
    });
  });
}

bool ThriftServer::isOverloaded(const THeader* header) {
  if (UNLIKELY(isOverloaded_(header))) {
    return true;
  }

  if (maxRequests_ > 0) {
    return activeRequests_ + getPendingCount() >= maxRequests_;
  }

  return false;
}

int64_t ThriftServer::getRequestLoad() {
  return activeRequests_ + getPendingCount();
}

std::string ThriftServer::getLoadInfo(int64_t load) {
  auto ioGroup = getIOGroupSafe();
  auto workerFactory = ioGroup != nullptr ?
    std::dynamic_pointer_cast<wangle::NamedThreadFactory>(
      ioGroup->getThreadFactory()) : nullptr;

  if (!workerFactory) {
    return "";
  }

  std::stringstream stream;

  stream
    << workerFactory->getNamePrefix() << " load is: "
    << load << "% requests, "
    << activeRequests_ << " active reqs, "
    << getPendingCount() << " pending reqs";

  return stream.str();
}

}} // apache::thrift
