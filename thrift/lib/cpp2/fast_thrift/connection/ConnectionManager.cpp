/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/fast_thrift/connection/ConnectionManager.h>

#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::connection {

ConnectionManager::Ptr ConnectionManager::create(
    folly::SocketAddress address,
    folly::Executor::KeepAlive<folly::IOThreadPoolExecutorBase> executor,
    fast_security::SSLPolicy sslPolicy,
    std::shared_ptr<const fast_security::TLSParams> tlsParams,
    SocketOptions socketOptions,
    folly::Executor::KeepAlive<folly::IOThreadPoolExecutorBase>
        acceptorExecutor) {
  return Ptr(new ConnectionManager(
      std::move(address),
      std::move(executor),
      sslPolicy,
      std::move(tlsParams),
      std::move(socketOptions),
      std::move(acceptorExecutor)));
}

ConnectionManager::ConnectionManager(
    folly::SocketAddress address,
    folly::Executor::KeepAlive<folly::IOThreadPoolExecutorBase> executor,
    fast_security::SSLPolicy sslPolicy,
    std::shared_ptr<const fast_security::TLSParams> tlsParams,
    SocketOptions socketOptions,
    folly::Executor::KeepAlive<folly::IOThreadPoolExecutorBase>
        acceptorExecutor)
    : address_(std::move(address)),
      workerExecutor_(std::move(executor)),
      acceptorExecutor_(
          acceptorExecutor ? std::move(acceptorExecutor)
                           : workerExecutor_.copy()),
      sslPolicy_(sslPolicy),
      tlsParamsObservable_(std::move(tlsParams)),
      socketOptions_(socketOptions),
      workerObserver_(std::make_shared<WorkerIOObserver>(*this)),
      acceptorObserver_(std::make_shared<AcceptorIOObserver>(*this)) {}

void ConnectionManager::start() {
  CHECK(configureHandler_)
      << "ConnectionManager::start called before setConnectionFactory";
  DCHECK(state_.load() != State::STARTED);
  // Workers must exist before listeners start: an acceptor snapshots its
  // dispatch targets when it is registered.
  state_.store(State::STARTED);
  workerExecutor_->addObserver(workerObserver_);
  acceptorExecutor_->addObserver(acceptorObserver_);
}

void ConnectionManager::stop() {
  State expected = State::STARTED;
  if (!state_.compare_exchange_strong(expected, State::STOPPED)) {
    return;
  }
  // Stop all producers first. ConnectionAcceptor::stop waits until each remote
  // accept queue has stopped before its ConnectionHandler can be destroyed.
  std::vector<std::reference_wrapper<ConnectionAcceptor>> acceptors;
  acceptors_.withRLock([&](const auto& map) {
    acceptors.reserve(map.size());
    for (const auto& [_, acceptor] : map) {
      acceptors.emplace_back(*acceptor);
    }
  });
  for (auto acceptor : acceptors) {
    acceptor.get().stop();
  }

  // Keep a manager-level barrier after every listener has stopped, so all raw
  // fds have either been installed or rejected before connection draining.
  for (auto& evb : workerExecutor_->getAllEventBases()) {
    evb->runInEventBaseThreadAndWait([] {});
  }

  // Snapshot handler pointers under the rlock so we can drive each through
  // its full shutdown without holding the lock across EVB hops.
  std::vector<std::pair<folly::EventBase*, ConnectionHandler*>> snapshot;
  handlers_.withRLock([&](const auto& map) {
    snapshot.reserve(map.size());
    for (const auto& [evb, handler] : map) {
      snapshot.emplace_back(evb, handler.get());
    }
  });
  for (auto& [_, handler] : snapshot) {
    // handler->stop() bounces to its EVB for the synchronous phases and
    // waits off-EVB for the per-connection teardown — safe to call from
    // this thread.
    handler->stop();
  }
  // Drop observers last: unregister callbacks only erase already-stopped
  // objects, so they are cheap. Acceptor objects go first because they retain
  // raw pointers to workers as dispatch targets.
  acceptorExecutor_->removeObserver(acceptorObserver_);
  workerExecutor_->removeObserver(workerObserver_);
}

ConnectionManager::~ConnectionManager() {
  stop();
}

folly::SocketAddress ConnectionManager::getAddress() const {
  folly::SocketAddress address;
  acceptors_.withRLock([&](const auto& map) {
    if (!map.empty()) {
      address = map.begin()->second->getAddress();
    }
  });
  return address;
}

void ConnectionManager::registerWorkerEventBase(folly::EventBase& evb) {
  DestructorGuard dg(this);

  // After stop() / dtor, refuse to spin up listeners on EVBs the
  // executor adds during shutdown.
  if (state_.load(std::memory_order_acquire) != State::STARTED) {
    return;
  }

  auto handler = std::make_unique<ConnectionHandler>(
      evb,
      sslPolicy_,
      tlsParamsObservable_.getObserver(),
      socketOptions_,
      socketOptions_.maxConnectionsPerIOThread,
      stats_,
      tlsStats_);
  configureHandler_(*handler);

  handlers_.withWLock([&](auto& map) {
    auto [_, inserted] = map.emplace(&evb, std::move(handler));
    if (!inserted) {
      LOG(FATAL) << "EventBase already registered";
    }
  });
}

void ConnectionManager::unregisterWorkerEventBase(folly::EventBase& evb) {
  DestructorGuard dg(this);

  // Contract: caller has already driven the handler through stop(); we
  // just drop the map entry.
  handlers_.withWLock([&](auto& map) { map.erase(&evb); });
}

void ConnectionManager::registerAcceptorEventBase(folly::EventBase& evb) {
  DestructorGuard dg(this);
  if (state_.load(std::memory_order_acquire) != State::STARTED) {
    return;
  }

  const bool acceptsOnWorkerExecutor =
      acceptorExecutor_.get() == workerExecutor_.get();
  std::vector<ConnectionWorkerTarget> workers;
  handlers_.withRLock([&](const auto& map) {
    workers.reserve(acceptsOnWorkerExecutor ? 1 : map.size());
    if (acceptsOnWorkerExecutor) {
      auto it = map.find(&evb);
      CHECK(it != map.end()) << "Missing colocated connection worker";
      workers.push_back(ConnectionWorkerTarget{evb, it->second->listener()});
      return;
    }
    for (const auto& [workerEvb, handler] : map) {
      workers.push_back(
          ConnectionWorkerTarget{*workerEvb, handler->listener()});
    }
  });
  CHECK(!workers.empty()) << "ConnectionManager has no data EventBases";

  ConnectionAcceptor::Ptr acceptor;
  {
    // The first acceptor chooses the ephemeral port; later acceptors must see
    // that port before binding so they join the same SO_REUSEPORT group.
    std::lock_guard lock(addressMutex_);
    acceptor = std::make_unique<ConnectionAcceptor>(
        evb,
        address_,
        socketOptions_,
        enableReusePortBpfSpread_ && acceptsOnWorkerExecutor,
        std::move(workers));
    acceptor->setConnectionEventCallback(connectionEventCallback_);
    acceptor->start();
    if (address_.getPort() == 0) {
      address_ = acceptor->getAddress();
    }
  }
  acceptors_.withWLock([&](auto& map) {
    auto [_, inserted] = map.emplace(&evb, std::move(acceptor));
    CHECK(inserted) << "Acceptor EventBase already registered";
  });
}

void ConnectionManager::unregisterAcceptorEventBase(folly::EventBase& evb) {
  DestructorGuard dg(this);
  acceptors_.withWLock([&](auto& map) { map.erase(&evb); });
}

} // namespace apache::thrift::fast_thrift::connection
