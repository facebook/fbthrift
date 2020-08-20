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

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>
#include <folly/io/GlobalShutdownSocketSet.h>
#include <folly/portability/Sockets.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketRoutingHandler.h>
#include <wangle/acceptor/FizzConfigUtil.h>
#include <wangle/acceptor/SharedSSLContextManager.h>

DEFINE_bool(
    thrift_abort_if_exceeds_shutdown_deadline,
    true,
    "Abort the server if failed to drain active requests within deadline");

DEFINE_string(
    thrift_ssl_policy,
    "disabled",
    "SSL required / permitted / disabled");

DEFINE_string(
    service_identity,
    "",
    "The name of the service. Associates the service with ACLs and keys");

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

ThriftServer::ThriftServer()
    : BaseThriftServer(),
      wShutdownSocketSet_(folly::tryGetShutdownSocketSet()),
      lastRequestTime_(
          std::chrono::steady_clock::now().time_since_epoch().count()) {
  addRoutingHandler(std::make_unique<apache::thrift::RocketRoutingHandler>());
  if (FLAGS_thrift_ssl_policy == "required") {
    sslPolicy_ = SSLPolicy::REQUIRED;
  } else if (FLAGS_thrift_ssl_policy == "permitted") {
    sslPolicy_ = SSLPolicy::PERMITTED;
  }
  ServerInstrumentation::registerServer(*this);
}

ThriftServer::ThriftServer(
    const std::shared_ptr<HeaderServerChannel>& serverChannel)
    : ThriftServer() {
  serverChannel_ = serverChannel;
  setNumIOWorkerThreads(1);
  setIdleTimeout(std::chrono::milliseconds(0));
}

ThriftServer::~ThriftServer() {
  ServerInstrumentation::removeServer(*this);
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
    if (lastRequestTime.time_since_epoch() !=
        std::chrono::steady_clock::duration::zero()) {
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
    }

    timer_.scheduleTimeout(this, timeout_);
  } catch (std::exception const& e) {
    LOG(ERROR) << e.what();
  }
}

std::chrono::steady_clock::time_point ThriftServer::lastRequestTime() const
    noexcept {
  return std::chrono::steady_clock::time_point(
      std::chrono::steady_clock::duration(
          lastRequestTime_.load(std::memory_order_acquire)));
}

void ThriftServer::touchRequestTimestamp() noexcept {
  if (idleServer_.has_value()) {
    lastRequestTime_.store(
        std::chrono::steady_clock::now().time_since_epoch().count(),
        std::memory_order_release);
  }
}

void ThriftServer::setup() {
  DCHECK_NOTNULL(getProcessorFactory().get());
  auto nWorkers = getNumIOWorkerThreads();
  DCHECK_GT(nWorkers, 0);

  // Initialize event base for this thread, ensure event_init() is called
  serveEventBase_ = eventBaseManager_->getEventBase();
  if (idleServerTimeout_.count() > 0) {
    idleServer_.emplace(
        *this, serveEventBase_.load()->timer(), idleServerTimeout_);
  }
  // Print some libevent stats
  VLOG(1) << "libevent " << folly::EventBase::getLibeventVersion() << " method "
          << folly::EventBase::getLibeventMethod();

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
    if (thriftProcessor_) {
      thriftProcessor_->setThreadManager(threadManager_.get());
      thriftProcessor_->setCpp2Processor(getCpp2Processor());
    }

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

      // Resize the IO pool
      ioThreadPool_->setNumThreads(nWorkers);
      if (!acceptPool_) {
        acceptPool_ = std::make_shared<folly::IOThreadPoolExecutor>(
            nAcceptors_,
            std::make_shared<folly::NamedThreadFactory>("Acceptor Thread"));
      }

      // Resize the SSL handshake pool
      size_t nSSLHandshakeWorkers = getNumSSLHandshakeWorkerThreads();
      VLOG(1) << "Using " << nSSLHandshakeWorkers << " SSL handshake threads";
      sslHandshakePool_->setNumThreads(nSSLHandshakeWorkers);

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
      } else if (port_ != -1) {
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

      // we enable zerocopy for the server socket if the
      // zeroCopyEnableFunc_ is valid
      bool useZeroCopy = !!zeroCopyEnableFunc_;
      for (auto& socket : getSockets()) {
        auto* evb = socket->getEventBase();
        evb->runImmediatelyOrRunInEventBaseThreadAndWait([&] {
          socket->setShutdownSocketSet(wShutdownSocketSet_);
          socket->setAcceptRateAdjustSpeed(acceptRateAdjustSpeed_);
          socket->setZeroCopy(useZeroCopy);

          try {
            socket->setTosReflect(tosReflect_);
          } catch (std::exception const& ex) {
            LOG(ERROR) << "Got exception setting up TOS reflect: "
                       << folly::exceptionStr(ex);
          }
        });
      }

      // Notify handler of the preServe event
      if (eventHandler_ != nullptr) {
        eventHandler_->preServe(&addresses_.at(0));
      }
    } else {
      startDuplex();
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
            getNumCPUWorkerThreads(), true /*stats*/));
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
  SCOPE_EXIT {
    this->cleanUp();
  };
  eventBaseManager_->getEventBase()->loopForever();
}

void ThriftServer::cleanUp() {
  DCHECK(!serverChannel_);

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

  // Force the cred processor to stop polling if it's set up
  tlsCredProcessor_.reset();
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
  {
    auto sockets = getSockets();
    folly::Baton<> done;
    SCOPE_EXIT {
      done.wait();
    };
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
  sslHandshakePool_->join();
  configMutable_ = true;
}

void ThriftServer::stopAcceptingAndJoinOutstandingRequests() {
  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (auto worker = dynamic_cast<Cpp2Worker*>(acceptor)) {
      worker->requestStop();
    }
  });

  {
    auto sockets = getSockets();
    folly::Baton<> done;
    SCOPE_EXIT {
      done.wait();
    };
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

  auto deadline = std::chrono::system_clock::now() + workersJoinTimeout_;
  forEachWorker([&](wangle::Acceptor* acceptor) {
    if (auto worker = dynamic_cast<Cpp2Worker*>(acceptor)) {
      if (!worker->waitForStop(deadline)) {
        if (FLAGS_thrift_abort_if_exceeds_shutdown_deadline) {
          LOG(FATAL)
              << "Could not drain active requests within allotted deadline. "
              << "Abort because undefined behavior is possible.";
        }
      }
    }
  });
}

void ThriftServer::stopCPUWorkers() {
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
  auto& processor = getCredProcessor();
  processor.setCertPathsToWatch(std::move(certPaths));
}

void ThriftServer::watchTicketPathForChanges(
    const std::string& ticketPath,
    bool initializeTickets) {
  if (initializeTickets) {
    auto seeds = TLSCredProcessor::processTLSTickets(ticketPath);
    if (seeds) {
      setTicketSeeds(std::move(*seeds));
    }
  }
  auto& processor = getCredProcessor();
  processor.setTicketPathToWatch(ticketPath);
}

TLSCredProcessor& ThriftServer::getCredProcessor() {
  if (!tlsCredProcessor_) {
    tlsCredProcessor_ = std::make_unique<TLSCredProcessor>();
    // setup callbacks once.  These will not be fired unless files are being
    // watched and modified.
    tlsCredProcessor_->addTicketCallback(
        [this](wangle::TLSTicketKeySeeds seeds) {
          updateTicketSeeds(std::move(seeds));
        });
    tlsCredProcessor_->addCertCallback([this] { updateTLSCert(); });
  }
  return *tlsCredProcessor_;
}

PreprocessResult ThriftServer::preprocess(
    const transport::THeader::StringToStringMap* readHeaders,
    const std::string* method) const {
  if (preprocess_) {
    return preprocess_(readHeaders, method);
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

folly::SemiFuture<std::vector<RequestSnapshot>>
ThriftServer::snapshotActiveRequests() {
  std::vector<folly::SemiFuture<std::vector<RequestSnapshot>>> tasks;

  forEachWorker([&tasks](wangle::Acceptor* acceptor) {
    auto worker = dynamic_cast<Cpp2Worker*>(acceptor);
    if (!worker) {
      return;
    }
    auto fut = folly::via(
        worker->getEventBase(),
        [reqRegistry = worker->getRequestsRegistry()]() {
          std::vector<RequestSnapshot> reqSnapshots;
          for (const auto& stub : reqRegistry->getActive()) {
            reqSnapshots.emplace_back(stub);
          }
          for (const auto& stub : reqRegistry->getFinished()) {
            reqSnapshots.emplace_back(stub);
          }
          return reqSnapshots;
        });
    tasks.emplace_back(std::move(fut));
  });

  return folly::collect(tasks.begin(), tasks.end())
      .deferValue([](std::vector<std::vector<RequestSnapshot>> results) {
        std::vector<RequestSnapshot> flat_result;
        for (auto& vec : results) {
          std::move(vec.begin(), vec.end(), std::back_inserter(flat_result));
        }
        return flat_result;
      });
}
} // namespace thrift
} // namespace apache
