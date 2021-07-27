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

#include <thrift/lib/cpp2/server/ThriftServer.h>

#include <fcntl.h>
#include <signal.h>

#include <iostream>
#include <random>

#include <glog/logging.h>
#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/executors/IOThreadPoolDeadlockDetectorObserver.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/GlobalShutdownSocketSet.h>
#include <folly/portability/Sockets.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/Flags.h>
#include <thrift/lib/cpp2/PluggableFunction.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/LoggingEvent.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/server/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/core/ManagedConnectionIf.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <wangle/acceptor/FizzConfigUtil.h>
#include <wangle/acceptor/SharedSSLContextManager.h>

using namespace std::literals::chrono_literals;

DEFINE_bool(
    thrift_abort_if_exceeds_shutdown_deadline,
    true,
    "Abort the server if failed to drain active requests within deadline");

DEFINE_string(
    thrift_ssl_policy, "disabled", "SSL required / permitted / disabled");

DEFINE_string(
    service_identity,
    "",
    "The name of the service. Associates the service with ACLs and keys");

THRIFT_FLAG_DEFINE_bool(server_alpn_prefer_rocket, true);
THRIFT_FLAG_DEFINE_bool(server_enable_stoptls, false);
THRIFT_FLAG_DEFINE_bool(ssl_policy_default_required, false);

THRIFT_PLUGGABLE_FUNC_REGISTER(
    apache::thrift::ThriftServer::DumpSnapshotOnLongShutdownResult,
    dumpSnapshotOnLongShutdown) {
  return {folly::makeSemiFuture(folly::unit), 0ms};
}

namespace {

[[noreturn]] void try_quick_exit(int code) {
#if defined(_GLIBCXX_HAVE_AT_QUICK_EXIT)
  std::quick_exit(code);
#else
  std::exit(code);
#endif
}
} // namespace

namespace apache {
namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::PriorityThreadManager;
using apache::thrift::concurrency::Runnable;
using apache::thrift::concurrency::ThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using folly::IOThreadPoolExecutor;
using folly::NamedThreadFactory;
using RequestSnapshot = ThriftServer::RequestSnapshot;
using std::shared_ptr;
using wangle::TLSCredProcessor;

TLSCredentialWatcher::TLSCredentialWatcher(ThriftServer* server)
    : credProcessor_() {
  credProcessor_.addCertCallback([server] { server->updateTLSCert(); });
  credProcessor_.addTicketCallback([server](wangle::TLSTicketKeySeeds seeds) {
    server->updateTicketSeeds(std::move(seeds));
  });
}

ThriftServer::ThriftServer()
    : BaseThriftServer(),
      wShutdownSocketSet_(folly::tryGetShutdownSocketSet()),
      lastRequestTime_(
          std::chrono::steady_clock::now().time_since_epoch().count()) {
  if (FLAGS_thrift_ssl_policy == "required") {
    sslPolicy_ = SSLPolicy::REQUIRED;
  } else if (FLAGS_thrift_ssl_policy == "permitted") {
    sslPolicy_ = SSLPolicy::PERMITTED;
  }
  metadata().wrapper = "ThriftServer-cpp";
}

ThriftServer::ThriftServer(
    const std::shared_ptr<HeaderServerChannel>& serverChannel)
    : ThriftServer() {
  serverChannel_ = serverChannel;
  setNumIOWorkerThreads(1);
  setIdleTimeout(std::chrono::milliseconds(0));
}

ThriftServer::~ThriftServer() {
  tracker_.reset();
  if (duplexWorker_) {
    // usually ServerBootstrap::stop drains the workers, but ServerBootstrap
    // doesn't know about duplexWorker_
    duplexWorker_->drainAllConnections();

    LOG_IF(ERROR, !duplexWorker_.unique())
        << getActiveRequests() << " active Requests while in destructing"
        << " duplex ThriftServer. Consider using startDuplex & stopDuplex";
  }

  if (stopWorkersOnStopListening_) {
    // Everything is already taken care of.
    return;
  }
  // If the flag is false, neither i/o nor CPU workers are stopped at this
  // point. Stop them now.
  if (!joinRequestsWhenServerStops_) {
    stopAcceptingAndJoinOutstandingRequests();
  }
  stopCPUWorkers();
  stopWorkers();
}

SSLPolicy ThriftServer::getSSLPolicy() const {
  // If it's explicitly set in constructor through gflags or through the
  // setSSLPolicy setter, then use that value.
  if (sslPolicy_.has_value()) {
    return *sslPolicy_;
  }
  // Otherwise, fallback to default (currently defined through a ThriftFlag).
  // PERMITTED is the old default. REQUIRED is the new default we're migrating
  // to. We can use ThriftFlags to opt-out services.
  return THRIFT_FLAG(ssl_policy_default_required) ? SSLPolicy::REQUIRED
                                                  : SSLPolicy::PERMITTED;
}

void ThriftServer::setProcessorFactory(
    std::shared_ptr<AsyncProcessorFactory> pFac) {
  CHECK(configMutable());
  BaseThriftServer::setProcessorFactory(pFac);
  thriftProcessor_.reset(new ThriftProcessor(*this));
}

void ThriftServer::useExistingSocket(
    folly::AsyncServerSocket::UniquePtr socket) {
  socket_ = std::move(socket);
}

void ThriftServer::useExistingSockets(const std::vector<int>& socketFds) {
  folly::AsyncServerSocket::UniquePtr socket(new folly::AsyncServerSocket);
  std::vector<folly::NetworkSocket> sockets;
  sockets.reserve(socketFds.size());
  for (auto s : socketFds) {
    sockets.push_back(folly::NetworkSocket::fromFd(s));
  }
  socket->useExistingSockets(sockets);
  useExistingSocket(std::move(socket));
}

void ThriftServer::useExistingSocket(int socket) {
  useExistingSockets({socket});
}

std::vector<int> ThriftServer::getListenSockets() const {
  std::vector<int> sockets;
  for (const auto& socket : getSockets()) {
    auto newsockets = socket->getNetworkSockets();
    sockets.reserve(sockets.size() + newsockets.size());
    for (auto s : newsockets) {
      sockets.push_back(s.toFd());
    }
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
    ThriftServer& server,
    folly::HHWheelTimer& timer,
    std::chrono::milliseconds timeout)
    : server_(server), timer_(timer), timeout_(timeout) {
  timer_.scheduleTimeout(this, timeout_);
}

void ThriftServer::IdleServerAction::timeoutExpired() noexcept {
  try {
    auto const lastRequestTime = server_.lastRequestTime();
    auto const elapsed = std::chrono::steady_clock::now() - lastRequestTime;
    if (elapsed >= timeout_) {
      LOG(INFO) << "shutting down server due to inactivity after "
                << std::chrono::duration_cast<std::chrono::milliseconds>(
                       elapsed)
                       .count()
                << "ms";
      server_.stop();
      return;
    }

    timer_.scheduleTimeout(this, timeout_);
  } catch (std::exception const& e) {
    LOG(ERROR) << e.what();
  }
}

std::chrono::steady_clock::time_point ThriftServer::lastRequestTime()
    const noexcept {
  return std::chrono::steady_clock::time_point(
      std::chrono::steady_clock::duration(
          lastRequestTime_.load(std::memory_order_relaxed)));
}

void ThriftServer::touchRequestTimestamp() noexcept {
  if (idleServer_.has_value()) {
    lastRequestTime_.store(
        std::chrono::steady_clock::now().time_since_epoch().count(),
        std::memory_order_relaxed);
  }
}

void ThriftServer::setup() {
  THRIFT_SERVER_EVENT(serve).log(*this);

  DCHECK(getProcessorFactory().get());
  auto nWorkers = getNumIOWorkerThreads();
  DCHECK_GT(nWorkers, 0u);

  stopAcceptingAndJoinOutstandingRequestsDone_ = false;

  addRoutingHandler(
      std::make_unique<apache::thrift::RocketRoutingHandler>(*this));

  // Initialize event base for this thread
  auto serveEventBase = eventBaseManager_->getEventBase();
  serveEventBase_ = serveEventBase;
  if (idleServerTimeout_.count() > 0) {
    idleServer_.emplace(*this, serveEventBase->timer(), idleServerTimeout_);
  }
  // Print some libevent stats
  VLOG(1) << "libevent " << folly::EventBase::getLibeventVersion() << " method "
          << serveEventBase->getLibeventMethod();

  try {
#ifndef _WIN32
    // OpenSSL might try to write to a closed socket if the peer disconnects
    // abruptly, raising a SIGPIPE signal. By default this will terminate the
    // process, which we don't want. Hence we need to handle SIGPIPE specially.
    //
    // We don't use SIG_IGN here as child processes will inherit that handler.
    // Instead, we swallow the signal to enable SIGPIPE in children to behave
    // normally.
    // Furthermore, setting flags to 0 and using sigaction prevents SA_RESTART
    // from restarting syscalls after the handler completed. This is important
    // for code using SIGPIPE to interrupt syscalls in other threads.
    struct sigaction sa = {};
    sa.sa_handler = [](int) {};
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGPIPE, &sa, nullptr);
#endif

    if (!getObserver() && server::observerFactory_) {
      setObserver(server::observerFactory_->getObserver());
    }

    // We always need a threadmanager for cpp2.
    setupThreadManager();
    threadManager_->setExpireCallback([&](std::shared_ptr<Runnable> r) {
      EventTask* task = dynamic_cast<EventTask*>(r.get());
      if (task) {
        task->expired();
      }
    });
    threadManager_->setCodelCallback([&](std::shared_ptr<Runnable>) {
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
      ServerBootstrap::socketConfig.acceptBacklog = getListenBacklog();
      ServerBootstrap::socketConfig.maxNumPendingConnectionsPerWorker =
          getMaxNumPendingConnectionsPerWorker();
      if (reusePort_.value_or(false)) {
        ServerBootstrap::setReusePort(true);
      }
      if (enableTFO_) {
        ServerBootstrap::socketConfig.enableTCPFastOpen = *enableTFO_;
        ServerBootstrap::socketConfig.fastOpenQueueSize = fastOpenQueueSize_;
      }

      ioThreadPool_->addObserver(
          folly::IOThreadPoolDeadlockDetectorObserver::create(
              ioThreadPool_->getName()));

      // Resize the IO pool
      ioThreadPool_->setNumThreads(nWorkers);
      if (!acceptPool_) {
        acceptPool_ = std::make_shared<folly::IOThreadPoolExecutor>(
            nAcceptors_,
            std::make_shared<folly::NamedThreadFactory>("Acceptor Thread"));
      }

      auto acceptorFactory = acceptorFactory_
          ? acceptorFactory_
          : std::make_shared<DefaultThriftAcceptorFactory>(this);
      if (auto factory = dynamic_cast<wangle::AcceptorFactorySharedSSLContext*>(
              acceptorFactory.get())) {
        sharedSSLContextManager_ = factory->initSharedSSLContextManager();
      }
      ServerBootstrap::childHandler(std::move(acceptorFactory));

      {
        std::lock_guard<std::mutex> lock(ioGroupMutex_);
        ServerBootstrap::group(acceptPool_, ioThreadPool_);
      }
      if (socket_) {
        ServerBootstrap::bind(std::move(socket_));
      } else if (!getAddress().isInitialized()) {
        ServerBootstrap::bind(port_);
      } else {
        for (auto& address : addresses_) {
          ServerBootstrap::bind(address);
        }
      }
      // Update address_ with the address that we are actually bound to.
      // (This is needed if we were supplied a pre-bound socket, or if
      // address_'s port was set to 0, so an ephemeral port was chosen by
      // the kernel.)
      ServerBootstrap::getSockets()[0]->getAddress(&addresses_.at(0));

      // THRIFT_PORT_OUT should be set with value of length 5. After thrift
      // server setup the env var will contain the port.
      if (auto portOutput = getenv("THRIFT_PORT_OUT")) {
        if (strlen(portOutput) == 5) {
          snprintf(portOutput, 6, "%05d", addresses_.at(0).getPort());
        }
      }

      // we enable zerocopy for the server socket if the
      // zeroCopyEnableFunc_ is valid
      bool useZeroCopy = !!zeroCopyEnableFunc_;
      for (auto& socket : getSockets()) {
        auto* evb = socket->getEventBase();
        evb->runImmediatelyOrRunInEventBaseThreadAndWait([&] {
          socket->setShutdownSocketSet(wShutdownSocketSet_);
          socket->setAcceptRateAdjustSpeed(acceptRateAdjustSpeed_);
          socket->setZeroCopy(useZeroCopy);
          socket->setQueueTimeout(getSocketQueueTimeout());

          try {
            socket->setTosReflect(tosReflect_);
            socket->setListenerTos(listenerTos_);
          } catch (std::exception const& ex) {
            LOG(ERROR) << "Got exception setting up TOS settings: "
                       << folly::exceptionStr(ex);
          }
        });
      }
    } else {
      startDuplex();
    }

    // Notify handler of the preStart event
    for (const auto& eventHandler : getEventHandlersUnsafe()) {
      eventHandler->preStart(&addresses_.at(0));
    }

    stoppedListening_ = false;
#if FOLLY_HAS_COROUTINES
    asyncScope_ = std::make_unique<folly::coro::CancellableAsyncScope>();
#endif

    // Called after setup
    callOnStartServing();

    started_.store(true, std::memory_order_release);

    // Notify handler of the preServe event
    for (const auto& eventHandler : getEventHandlersUnsafe()) {
      eventHandler->preServe(&addresses_.at(0));
    }

    // Do not allow setters to be called past this point until the IO worker
    // threads have been joined in stopWorkers().
    configMutable_ = false;
  } catch (std::exception& ex) {
    // This block allows us to investigate the exception using gdb
    LOG(ERROR) << "Got an exception while setting up the server: " << ex.what();
    handleSetupFailure();
    throw;
  } catch (...) {
    handleSetupFailure();
    throw;
  }
}

void ThriftServer::setupThreadManager() {
  if (!threadManager_) {
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager(
        PriorityThreadManager::newPriorityThreadManager(
            getNumCPUWorkerThreads()));
    threadManager->enableCodel(getEnableCodel());
    // If a thread factory has been specified, use it.
    if (threadFactory_) {
      threadManager->threadFactory(threadFactory_);
    }
    auto poolThreadName = getCPUWorkerThreadName();
    if (!poolThreadName.empty()) {
      threadManager->setNamePrefix(poolThreadName);
    }
    threadManager->start();
    setThreadManager(threadManager);
  }
}

/**
 * Preferably use this method in order to start ThriftServer created for
 * DuplexChannel instead of the serve() method.
 */
void ThriftServer::startDuplex() {
  CHECK(configMutable());
  duplexWorker_ = Cpp2Worker::create(this, serverChannel_);
  // we don't control the EventBase for the duplexWorker, so when we shut
  // it down, we need to ensure there's no delay
  duplexWorker_->setGracefulShutdownTimeout(std::chrono::milliseconds(0));
}

/**
 * This method should be used to cleanly stop a ThriftServer created for
 * DuplexChannel before disposing the ThriftServer. The caller should pass in
 * a shared_ptr to this ThriftServer since the ThriftServer does not have a
 * way of getting that (does not inherit from enable_shared_from_this)
 */
void ThriftServer::stopDuplex(std::shared_ptr<ThriftServer> thisServer) {
  DCHECK(this == thisServer.get());
  DCHECK(duplexWorker_ != nullptr);

  // Try to stop our Worker but this cannot stop in flight requests
  // Instead, it will capture a shared_ptr back to us, keeping us alive
  // until it really goes away (when in-flight requests are gone)
  duplexWorker_->stopDuplex(thisServer);

  // Get rid of our reference to the worker to avoid forming a cycle
  duplexWorker_ = nullptr;
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

  auto sslContextConfigCallbackHandle = sslContextObserver_
      ? getSSLCallbackHandle()
      : folly::observer::CallbackHandle{};

  tracker_.emplace(instrumentation::kThriftServerTrackerKey, *this);
  eventBaseManager_->getEventBase()->loopForever();
}

void ThriftServer::cleanUp() {
  DCHECK(!serverChannel_);

  // tlsCredWatcher_ uses a background thread that needs to be joined prior
  // to any further writes to ThriftServer members.
  tlsCredWatcher_.withWLock([](auto& credWatcher) { credWatcher.reset(); });

  // It is users duty to make sure that setup() call
  // should have returned before doing this cleanup
  idleServer_.reset();
  serveEventBase_ = nullptr;
  stopListening();

  // Stop the routing handlers.
  for (auto& handler : routingHandlers_) {
    handler->stopListening();
  }

  if (stopWorkersOnStopListening_) {
    // Wait on the i/o worker threads to actually stop
    stopWorkers();
  } else if (joinRequestsWhenServerStops_) {
    stopAcceptingAndJoinOutstandingRequests();
  }

  // Now clear all the handlers
  routingHandlers_.clear();
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
  // We have to make sure stopListening() is not called twice when both
  // stopListening() and cleanUp() are called
  if (stoppedListening_.exchange(true)) {
    // stopListening() was called earlier
    return;
  }
  // Called before cleanUp
  callOnStopServing();

#if FOLLY_HAS_COROUTINES
  asyncScope_->requestCancellation();
#endif

  {
    auto sockets = getSockets();
    folly::Baton<> done;
    SCOPE_EXIT { done.wait(); };
    std::shared_ptr<folly::Baton<>> doneGuard(
        &done, [](folly::Baton<>* done) { done->post(); });

    for (auto& socket : sockets) {
      // Stop accepting new connections
      auto eb = socket->getEventBase();
      eb->runInEventBaseThread([socket = std::move(socket), doneGuard] {
        socket->pauseAccepting();
      });
    }
  }

  if (stopWorkersOnStopListening_) {
    stopAcceptingAndJoinOutstandingRequests();
    stopCPUWorkers();
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

void ThriftServer::stopAcceptingAndJoinOutstandingRequests() {
  if (std::exchange(stopAcceptingAndJoinOutstandingRequestsDone_, true)) {
    return;
  }
  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (auto worker = dynamic_cast<Cpp2Worker*>(acceptor)) {
      worker->requestStop();
    }
  });
  // tlsCredWatcher_ uses a background thread that needs to be joined prior
  // to any further writes to ThriftServer members.
  tlsCredWatcher_.withWLock([](auto& credWatcher) { credWatcher.reset(); });
  sharedSSLContextManager_ = nullptr;

  {
    auto sockets = getSockets();
    folly::Baton<> done;
    SCOPE_EXIT { done.wait(); };
    std::shared_ptr<folly::Baton<>> doneGuard(
        &done, [](folly::Baton<>* done) { done->post(); });

    for (auto& socket : sockets) {
      // Stop accepting new connections
      auto eb = socket->getEventBase();
      eb->runInEventBaseThread([socket = std::move(socket), doneGuard] {
        // Close the listening socket
        // This will also cause the workers to stop
        socket->stopAccepting();
      });
    }
  }

  auto joinDeadline =
      std::chrono::steady_clock::now() + getWorkersJoinTimeout();

  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (auto worker = dynamic_cast<Cpp2Worker*>(acceptor)) {
      if (!worker->waitForStop(joinDeadline)) {
        // Before we crash, let's dump a snapshot of the server.
        folly::CPUThreadPoolExecutor dumpSnapshotExecutor{1};
        // The IO threads may be deadlocked in which case we won't be able to
        // dump snapshots. It still shouldn't block shutdown indefinitely.
        auto dumpSnapshotResult =
            THRIFT_PLUGGABLE_FUNC(dumpSnapshotOnLongShutdown)();
        try {
          std::move(dumpSnapshotResult.task)
              .via(folly::getKeepAliveToken(dumpSnapshotExecutor))
              .get(dumpSnapshotResult.timeout);
        } catch (...) {
          LOG(ERROR) << "Failed to dump server snapshot on long shutdown: "
                     << folly::exceptionStr(std::current_exception());
        }

        auto msgTemplate =
            "Could not drain active requests within allotted deadline. "
            "Deadline value: {} secs. {} because undefined behavior is possible. "
            "Underlying reasons could be either requests that have never "
            "terminated, long running requests, or long queues that could "
            "not be fully processed.";
        if (quickExitOnShutdownTimeout_) {
          LOG(ERROR) << fmt::format(
              msgTemplate,
              getWorkersJoinTimeout().count(),
              "quick_exiting (no coredump)");
          // similar to abort but without generating a coredump
          try_quick_exit(124);
        }
        if (FLAGS_thrift_abort_if_exceeds_shutdown_deadline) {
          LOG(FATAL) << fmt::format(
              msgTemplate, getWorkersJoinTimeout().count(), "Aborting");
        }
      }
    }
  });

  started_.store(false, std::memory_order_relaxed);
}

void ThriftServer::callOnStartServing() {
  auto handlerList = getProcessorFactory()->getServiceHandlers();
  std::vector<folly::SemiFuture<folly::Unit>> futures;
  futures.reserve(handlerList.size());
  for (auto handler : handlerList) {
    futures.emplace_back(handler->semifuture_onStartServing());
  }
  folly::collectAll(futures.begin(), futures.end())
      .via(getThreadManager().get())
      .get();
}

void ThriftServer::callOnStopServing() {
  auto handlerList = getProcessorFactory()->getServiceHandlers();
  std::vector<folly::SemiFuture<folly::Unit>> futures;
  futures.reserve(handlerList.size());
  for (auto handler : handlerList) {
    futures.emplace_back(handler->semifuture_onStopServing());
  }
  folly::collectAll(futures.begin(), futures.end())
      .via(getThreadManager().get())
      .get();
}

void ThriftServer::stopCPUWorkers() {
#if FOLLY_HAS_COROUTINES
  // Wait for tasks running on AsyncScope to join
  folly::coro::blockingWait(asyncScope_->joinAsync());
#endif
  // Wait for any tasks currently running on the task queue workers to
  // finish, then stop the task queue workers. Have to do this now, so
  // there aren't tasks completing and trying to write to i/o thread
  // workers after we've stopped the i/o workers.
  threadManager_->join();
}

void ThriftServer::handleSetupFailure(void) {
  ServerBootstrap::stop();

  // avoid crash on stop()
  idleServer_.reset();
  serveEventBase_ = nullptr;
}

void ThriftServer::updateTicketSeeds(wangle::TLSTicketKeySeeds seeds) {
  if (sharedSSLContextManager_) {
    sharedSSLContextManager_->updateTLSTicketKeys(seeds);
  } else {
    forEachWorker([&](wangle::Acceptor* acceptor) {
      if (!acceptor) {
        return;
      }
      auto evb = acceptor->getEventBase();
      if (!evb) {
        return;
      }
      evb->runInEventBaseThread([acceptor, seeds] {
        acceptor->setTLSTicketSecrets(
            seeds.oldSeeds, seeds.currentSeeds, seeds.newSeeds);
      });
    });
  }
}

void ThriftServer::updateTLSCert() {
  if (sharedSSLContextManager_) {
    sharedSSLContextManager_->reloadSSLContextConfigs();
  } else {
    forEachWorker([&](wangle::Acceptor* acceptor) {
      if (!acceptor) {
        return;
      }
      auto evb = acceptor->getEventBase();
      if (!evb) {
        return;
      }
      evb->runInEventBaseThread(
          [acceptor] { acceptor->resetSSLContextConfigs(); });
    });
  }
}

void ThriftServer::updateCertsToWatch() {
  std::set<std::string> certPaths;
  if (sslContextObserver_.has_value()) {
    auto sslContext = *sslContextObserver_->getSnapshot();
    if (!sslContext.certificates.empty()) {
      const auto& cert = sslContext.certificates[0];
      certPaths.insert(cert.certPath);
      certPaths.insert(cert.keyPath);
      certPaths.insert(cert.passwordPath);
    }
    certPaths.insert(sslContext.clientCAFile);
  }
  tlsCredWatcher_.withWLock([this, &certPaths](auto& credWatcher) {
    if (!credWatcher) {
      credWatcher.emplace(this);
    }
    credWatcher->setCertPathsToWatch(std::move(certPaths));
  });
}

void ThriftServer::watchTicketPathForChanges(const std::string& ticketPath) {
  auto seeds = TLSCredProcessor::processTLSTickets(ticketPath);
  if (seeds) {
    setTicketSeeds(std::move(*seeds));
  }
  tlsCredWatcher_.withWLock([this, &ticketPath](auto& credWatcher) {
    if (!credWatcher) {
      credWatcher.emplace(this);
    }
    credWatcher->setTicketPathToWatch(ticketPath);
  });
}

PreprocessResult ThriftServer::preprocess(
    const PreprocessParams& params) const {
  if (preprocess_) {
    return preprocess_(params);
  }
  return {};
}

folly::Optional<std::string> ThriftServer::checkOverload(
    const transport::THeader::StringToStringMap* readHeaders,
    const std::string* method) const {
  if (UNLIKELY(isOverloaded_ && isOverloaded_(readHeaders, method))) {
    return kAppOverloadedErrorCode;
  }

  uint32_t maxRequests = getMaxRequests();
  if (maxRequests > 0 &&
      (method == nullptr ||
       getMethodsBypassMaxRequestsLimit().count(*method) == 0)) {
    if (static_cast<uint32_t>(getActiveRequests()) >= maxRequests) {
      return kOverloadedErrorCode;
    }
  }

  return {};
}

std::string ThriftServer::getLoadInfo(int64_t load) const {
  auto ioGroup = getIOGroupSafe();
  auto workerFactory = ioGroup != nullptr
      ? std::dynamic_pointer_cast<folly::NamedThreadFactory>(
            ioGroup->getThreadFactory())
      : nullptr;

  if (!workerFactory) {
    return "";
  }

  std::stringstream stream;

  stream << workerFactory->getNamePrefix() << " load is: " << load
         << "% requests, " << getActiveRequests() << " active reqs";

  return stream.str();
}

void ThriftServer::replaceShutdownSocketSet(
    const std::shared_ptr<folly::ShutdownSocketSet>& newSSS) {
  wShutdownSocketSet_ = newSSS;
}

folly::SemiFuture<ThriftServer::ServerSnapshot> ThriftServer::getServerSnapshot(
    const SnapshotOptions& options) {
  // WorkerSnapshots look the same as the server, except they are unaggregated
  using WorkerSnapshot = ServerSnapshot;
  std::vector<folly::SemiFuture<WorkerSnapshot>> tasks;
  const auto snapshotTime = std::chrono::steady_clock::now();

  forEachWorker([&tasks, snapshotTime, options](wangle::Acceptor* acceptor) {
    auto worker = dynamic_cast<Cpp2Worker*>(acceptor);
    if (!worker) {
      return;
    }
    auto fut =
        folly::via(worker->getEventBase(), [worker, snapshotTime, options]() {
          auto reqRegistry = worker->getRequestsRegistry();
          DCHECK(reqRegistry);
          RequestSnapshots requestSnapshots;
          if (reqRegistry != nullptr) {
            for (const auto& stub : reqRegistry->getActive()) {
              requestSnapshots.emplace_back(stub);
            }
            for (const auto& stub : reqRegistry->getFinished()) {
              requestSnapshots.emplace_back(stub);
            }
          }

          std::unordered_map<folly::SocketAddress, ConnectionSnapshot>
              connectionSnapshots;
          worker->getConnectionManager()->forEachConnection(
              [&](wangle::ManagedConnection* wangleConnection) {
                if (auto managedConnection =
                        dynamic_cast<ManagedConnectionIf*>(wangleConnection)) {
                  auto numActiveRequests =
                      managedConnection->getNumActiveRequests();
                  auto numPendingWrites =
                      managedConnection->getNumPendingWrites();
                  auto creationTime = managedConnection->getCreationTime();
                  auto minCreationTime =
                      snapshotTime - options.connectionsAgeMax;
                  if (numActiveRequests > 0 || numPendingWrites > 0 ||
                      creationTime > minCreationTime) {
                    connectionSnapshots.emplace(
                        managedConnection->getPeerAddress(),
                        ConnectionSnapshot{
                            numActiveRequests, numPendingWrites, creationTime});
                  }
                }
              });
          return WorkerSnapshot{
              worker->getRequestsRegistry()->getRequestCounter().get(),
              std::move(requestSnapshots),
              std::move(connectionSnapshots)};
        });
    tasks.emplace_back(std::move(fut));
  });

  return folly::collect(tasks.begin(), tasks.end())
      .deferValue(
          [](std::vector<WorkerSnapshot> workerSnapshots) -> ServerSnapshot {
            ServerSnapshot ret{};

            // Sum all request and connection counts
            size_t numRequests = 0;
            size_t numConnections = 0;
            for (const auto& workerSnapshot : workerSnapshots) {
              for (uint64_t i = 0; i < ret.recentCounters.size(); ++i) {
                ret.recentCounters[i].first +=
                    workerSnapshot.recentCounters[i].first;
                ret.recentCounters[i].second +=
                    workerSnapshot.recentCounters[i].second;
              }
              numRequests += workerSnapshot.requests.size();
              numConnections += workerSnapshot.connections.size();
            }

            // Move all RequestSnapshots and ConnectionSnapshots to
            // ServerSnapshot
            ret.requests.reserve(numRequests);
            ret.connections.reserve(numConnections);
            for (auto& workerSnapshot : workerSnapshots) {
              auto& requests = workerSnapshot.requests;
              std::move(
                  requests.begin(),
                  requests.end(),
                  std::back_inserter(ret.requests));

              auto& connections = workerSnapshot.connections;
              std::move(
                  connections.begin(),
                  connections.end(),
                  std::inserter(ret.connections, ret.connections.end()));
            }
            return ret;
          });
}

folly::observer::Observer<std::list<std::string>>
ThriftServer::defaultNextProtocols() {
  return folly::observer::makeObserver(
      [rocketPreferredObserver =
           THRIFT_FLAG_OBSERVE(server_alpn_prefer_rocket)] {
        const auto rocketPreferred = *rocketPreferredObserver.getSnapshot();
        if (rocketPreferred) {
          return std::list<std::string>{
              "rs",
              "thrift",
              "h2",
              // "http" is not a legit specifier but need to include it for
              // legacy.  Thrift's HTTP2RoutingHandler uses this, and clients
              // may be sending it.
              "http"};
        }
        return std::list<std::string>{
            "thrift",
            "h2",
            // "http" is not a legit specifier but need to include it for
            // legacy.  Thrift's HTTP2RoutingHandler uses this, and clients
            // may be sending it.
            "http",
            "rs"};
      });
}

folly::observer::Observer<bool> ThriftServer::enableStopTLS() {
  return THRIFT_FLAG_OBSERVE(server_enable_stoptls);
}

folly::observer::CallbackHandle ThriftServer::getSSLCallbackHandle() {
  return sslContextObserver_->addCallback([&](auto ssl) {
    if (sharedSSLContextManager_) {
      sharedSSLContextManager_->updateSSLConfigAndReloadContexts(*ssl);
    } else {
      // "this" needed due to
      // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67274
      this->forEachWorker([&](wangle::Acceptor* acceptor) {
        auto evb = acceptor->getEventBase();
        if (!evb) {
          return;
        }
        evb->runInEventBaseThread([acceptor, ssl] {
          for (auto& sslContext : acceptor->getConfig().sslContextConfigs) {
            sslContext = *ssl;
          }
          acceptor->resetSSLContextConfigs();
        });
      });
    }
    this->updateCertsToWatch();
  });
}
} // namespace thrift
} // namespace apache
