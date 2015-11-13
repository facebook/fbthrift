/*
 * Copyright 2015 Facebook, Inc.
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

#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

#include <folly/Memory.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/transport/TTransportUtils.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache { namespace thrift {

typedef std::function<void(
  folly::EventBase*,
  std::shared_ptr<apache::thrift::async::TAsyncTransport>,
  std::unique_ptr<folly::IOBuf>)> getHandlerFunc;

typedef std::function<void(const apache::thrift::transport::THeader*,
                           const folly::SocketAddress*)> GetHeaderHandlerFunc;

template <typename T>
class ThriftServerAsyncProcessorFactory : public AsyncProcessorFactory {
  public:
    explicit ThriftServerAsyncProcessorFactory(std::shared_ptr<T> t) {
      svIf_ = t;
    }
    std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
      return std::unique_ptr<apache::thrift::AsyncProcessor> (
        new typename T::ProcessorType(svIf_.get()));
    }
  private:
    std::shared_ptr<T> svIf_;
};

/**
 *   Base class for thrift servers using cpp2 style generated code.
 */

class BaseThriftServer : public apache::thrift::server::TServer {
 protected:

  //! Default number of worker threads (should be # of processor cores).
  static const int T_ASYNC_DEFAULT_WORKER_THREADS;

  static const uint32_t T_MAX_NUM_PENDING_CONNECTIONS_PER_WORKER = 0xffffffff;

  static const std::chrono::milliseconds DEFAULT_TIMEOUT;

  static const std::chrono::milliseconds DEFAULT_TASK_EXPIRE_TIME;

  /// Listen backlog
  static const int DEFAULT_LISTEN_BACKLOG = 1024;

  //! Prefix for pool thread names
  std::string poolThreadName_;

  // Cpp2 ProcessorFactory.
  std::shared_ptr<apache::thrift::AsyncProcessorFactory> cpp2Pfac_;

  //! The server's listening address
  folly::SocketAddress address_;

  //! The server's listening port
  int port_ = -1;

  //! Number of io worker threads (may be set) (should be # of CPU cores)
  int nWorkers_ = T_ASYNC_DEFAULT_WORKER_THREADS;

  //! Number of sync pool threads (may be set) (should be set to expected
  //  sync load)
  int nPoolThreads_ = 0;

  /**
   * The thread manager used for sync calls.
   */
  std::mutex threadManagerMutex_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;

  bool enableCodel_ = false;

  //! Milliseconds we'll wait for data to appear (0 = infinity)
  std::chrono::milliseconds timeout_ = DEFAULT_TIMEOUT;

  /**
   * The time in milliseconds before an unperformed task expires
   * (0 == infinite)
   */
  std::chrono::milliseconds taskExpireTime_ = DEFAULT_TASK_EXPIRE_TIME;

  /**
   * The number of incoming connections the TCP stack will buffer up while
   * waiting for the Thrift server to call accept() on them.
   *
   * If the Thrift server cannot keep up, and this limit is reached, the
   * TCP stack will start sending resets to drop excess connections.
   *
   * Actual behavior of the socket backlog is dependent on the TCP
   * implementation, and it may be further limited or even ignored on some
   * systems. See manpage for listen(2) for details.
   */
  int listenBacklog_ = DEFAULT_LISTEN_BACKLOG;

  /**
   * The maximum number of pending connections each io worker thread can hold.
   */
  uint32_t maxNumPendingConnectionsPerWorker_ =
    T_MAX_NUM_PENDING_CONNECTIONS_PER_WORKER;

  // Notification of various server events
  std::shared_ptr<apache::thrift::server::TServerObserver> observer_;

  // Max number of active connections
  uint32_t maxConnections_ = 0;

  // Max active requests
  uint32_t maxRequests_ = concurrency::ThreadManager::DEFAULT_MAX_QUEUE_SIZE;

  // If it is set true, server will check and use client timeout header
  bool useClientTimeout_ = true;

  std::function<bool(const transport::THeader*)> isOverloaded_ =
      [](const transport::THeader* header) { return false; };
  std::function<int64_t(const std::string&)> getLoad_;

  enum class InjectedFailure {
    NONE,
    ERROR,
    DROP,
    DISCONNECT
  };

  class CumulativeFailureInjection {
   public:
    CumulativeFailureInjection()
      : empty_(true),
        errorThreshold_(0),
        dropThreshold_(0),
        disconnectThreshold_(0) {
    }

    InjectedFailure test() const;

    void set(const FailureInjection& fi);

   private:
    std::atomic<bool> empty_;
    mutable std::mutex mutex_;
    float errorThreshold_;
    float dropThreshold_;
    float disconnectThreshold_;
  };

  // Unlike FailureInjection, this is cumulative and thread-safe
  CumulativeFailureInjection failureInjection_;

  InjectedFailure maybeInjectFailure() const {
    return failureInjection_.test();
  }

  getHandlerFunc getHandler_;
  GetHeaderHandlerFunc getHeaderHandler_;

  // Flag indicating whether it is safe to mutate the server config through its
  // setters.
  std::atomic<bool> configMutable_{true};

  BaseThriftServer()
    : apache::thrift::server::TServer(std::shared_ptr<server::TProcessor>()){
  }

  ~BaseThriftServer() override {}

 public:

  /**
   * Indicate whether it is safe to modify the server config through setters.
   * This roughly corresponds to whether the IO thread pool could be servicing
   * requests.
   *
   * @return true if the configuration can be modified, false otherwise
   */
  bool configMutable() {
    return configMutable_;
  }

  /**
   * Get the prefix for naming the pool threads.
   *
   * @return current setting.
   */
  const std::string& getPoolThreadName() const {
    return poolThreadName_;
  }

  /**
   * Set the prefix for naming the pool threads. Not set by default.
   * must be called before serve() for it to take effect
   * ignored if setThreadManager() is called.
   *
   * @param poolThreadName thread name prefix
   */
  void setPoolThreadName(const std::string& poolThreadName) {
    poolThreadName_ = poolThreadName;
  }

  /**
   * Set Thread Manager (for queuing mode).
   * If not set, defaults to the number of worker threads.
   *
   * @param threadManager a shared pointer to the thread manager
   */
  void setThreadManager(
    std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    threadManager) {
    CHECK(configMutable());
    std::lock_guard<std::mutex> lock(threadManagerMutex_);
    threadManager_ = threadManager;
  }

  /**
   * Get Thread Manager (for queuing mode).
   *
   * @return a shared pointer to the thread manager
   */
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
  getThreadManager() {
    std::lock_guard<std::mutex> lock(threadManagerMutex_);
    return threadManager_;
  }

  /**
   * Get the maximum # of connections allowed before overload.
   *
   * @return current setting.
   */
  uint32_t getMaxConnections() const {
    return maxConnections_;
  }

  /**
   * Set the maximum # of connections allowed before overload.
   *
   * @param maxConnections new setting for maximum # of connections.
   */
  void setMaxConnections(uint32_t maxConnections) {
    maxConnections_ = maxConnections;
  }

  /**
   * Get the maximum # of connections waiting in handler/task before overload.
   *
   * @return current setting.
   */
  uint32_t getMaxRequests() const {
    return maxRequests_;
  }

  /**
   * Set the maximum # of requests being processed in handler before overload.
   *
   * @param maxRequests new setting for maximum # of active requests.
   */
  void setMaxRequests(uint32_t maxRequests) {
    maxRequests_ = maxRequests;
  }

  bool getUseClientTimeout() const {
    return useClientTimeout_;
  }

  void setUseClientTimeout(bool useClientTimeout) {
    useClientTimeout_ = useClientTimeout;
  }

  virtual bool isOverloaded(uint32_t workerActiveRequests = 0,
                    const apache::thrift::transport::THeader* header = nullptr) = 0;

  // Get load percent of the server.  Must be a number between 0 and 100:
  // 0 - no load, 100-fully loaded.
  int64_t getLoad(const std::string& counter = "", bool check_custom = true);
  virtual int64_t getRequestLoad() = 0;
  virtual int64_t getConnectionLoad() = 0;
  int64_t getQueueLoad();
  virtual std::string getLoadInfo(int64_t reqload, int64_t connload, int64_t queueload);

  void setObserver(
    const std::shared_ptr<apache::thrift::server::TServerObserver>& observer) {
    observer_ = observer;
  }

  const std::shared_ptr<apache::thrift::server::TServerObserver>&
  getObserver() const {
    return observer_;
  }

  std::unique_ptr<apache::thrift::AsyncProcessor> getCpp2Processor() {
    return cpp2Pfac_->getProcessor();
  }

  /**
   * Set the address to listen on.
   */
  void setAddress(const folly::SocketAddress& address) {
    CHECK(configMutable());
    port_ = -1;
    address_ = address;
  }

  void setAddress(folly::SocketAddress&& address) {
    CHECK(configMutable());
    port_ = -1;
    address_ = std::move(address);
  }

  void setAddress(const char* ip, uint16_t port) {
    CHECK(configMutable());
    port_ = -1;
    address_.setFromIpPort(ip, port);
  }
  void setAddress(const std::string& ip, uint16_t port) {
    CHECK(configMutable());
    port_ = -1;
    setAddress(ip.c_str(), port);
  }

  /**
   * Get the address the server is listening on.
   *
   * This should generally only be called after setup() has finished.
   *
   * (The address may be uninitialized until setup() has run.  If called from
   * another thread besides the main server thread, the caller is responsible
   * for providing their own synchronization to ensure that setup() is not
   * modifying the address while they are using it.)
   */
  const folly::SocketAddress& getAddress() const {
    return address_;
  }

  /**
   * Set the port to listen on.
   */
  void setPort(uint16_t port) {
    CHECK(configMutable());
    port_ = port;
  }

  /**
   * Get the maximum number of pending connections each io worker thread can
   * hold.
   */
  uint32_t getMaxNumPendingConnectionsPerWorker() const {
    return maxNumPendingConnectionsPerWorker_;
  }
  /**
   * Set the maximum number of pending connections each io worker thread can
   * hold. No new connections will be sent to that io worker thread if there
   * are more than such number of unprocessed connections in that queue. If
   * every io worker thread's queue is full the connection will be dropped.
   */
  void setMaxNumPendingConnectionsPerWorker(uint32_t num) {
    CHECK(configMutable());
    maxNumPendingConnectionsPerWorker_ = num;
  }

  /**
   * Get the number of connections dropped by the AsyncServerSocket
   */
  virtual uint64_t getNumDroppedConnections() const = 0;

  /** Get maximum number of milliseconds we'll wait for data (0 = infinity).
   *
   *  @return number of milliseconds, or 0 if no timeout set.
   */
  std::chrono::milliseconds getIdleTimeout() const {
    return timeout_;
  }

  /** Set maximum number of milliseconds we'll wait for data (0 = infinity).
   *  Note: existing connections are unaffected by this call.
   *
   *  @param timeout number of milliseconds, or 0 to disable timeouts.
   */
  void setIdleTimeout(std::chrono::milliseconds timeout) {
    CHECK(configMutable());
    timeout_ = timeout;
  }

  /**
   * Set the number of worker threads
   *
   * @param number of worker threads
   */
  void setNWorkerThreads(int nWorkers) {
    CHECK(configMutable());
    nWorkers_ = nWorkers;
  }

  /**
   * Get the number of worker threads
   *
   * @return number of worker threads
   */
  int getNWorkerThreads() {
    return nWorkers_;
  }

  /**
   * Set the number of pool threads
   * Only valid if you do not also set a threadmanager.
   *
   * @param number of pool threads
   */
  void setNPoolThreads(int nPoolThreads) {
    CHECK(configMutable());
    CHECK(!threadManager_);

    nPoolThreads_ = nPoolThreads;
  }

  /**
   * Get the number of pool threads
   *
   * @return number of pool threads
   */
  int getNPoolThreads() {
    return nPoolThreads_;
  }

  /**
   * Codel queuing timeout - limit queueing time before overload
   * http://en.wikipedia.org/wiki/CoDel
   */
  void setEnableCodel(bool enableCodel) {
    enableCodel_ = enableCodel;
  }

  bool getEnableCodel() {
    return enableCodel_;
  }

  /**
   * Set the processor factory as the one built into the
   * ServerInterface.
   *
   * setInterface() can take both unique_ptr and shared_ptr to handler
   * interface.
   *
   * @param handler interface shared_ptr
   */
  void setInterface(std::shared_ptr<ServerInterface> iface) {
    setProcessorFactory(std::move(iface));
  }

  /**
   * Sets an explicit AsyncProcessorFactory
   *
   */
  void setProcessorFactory(
      std::shared_ptr<AsyncProcessorFactory> pFac) {
    CHECK(configMutable());
    cpp2Pfac_ = pFac;
  }

  /**
   * Set the task expire time
   *
   */
  void setTaskExpireTime(std::chrono::milliseconds timeout) {
    taskExpireTime_ = timeout;
  }

  /**
   * Get the task expire time
   *
   * @return task expire time
   */
  std::chrono::milliseconds getTaskExpireTime() const {
    return taskExpireTime_;
  }

  /**
   * A task has two timeouts:
   *
   * If the task hasn't started processing the request by the time the soft
   * timeout has expired, we should throw the task away.
   *
   * However, if the task has started processing the request by the time the
   * soft timeout has expired, we shouldn't expire the task until the hard
   * timeout has expired.
   *
   * The soft timeout protects the server from starting to process too many
   * requests.  The hard timeout protects us from sending responses that
   * are never read.
   *
   * @returns whether or not the soft and hard timeouts are different
   */
  bool getTaskExpireTimeForRequest(
    const apache::thrift::transport::THeader& header,
    std::chrono::milliseconds& softTimeout,
    std::chrono::milliseconds& hardTimeout
  ) const;

  /**
   * Set the listen backlog. Refer to the comment on listenBacklog_ member for
   * details.
   */
  void setListenBacklog(int listenBacklog) {
    listenBacklog_ = listenBacklog;
  }

  /**
   * Get the listen backlog.
   *
   * @return listen backlog.
   */
  int getListenBacklog() const {
    return listenBacklog_;
  }

  void setIsOverloaded(std::function<
      bool(const apache::thrift::transport::THeader*)> isOverloaded) {
    isOverloaded_ = isOverloaded;
  }

  void setGetLoad(std::function<int64_t(const std::string&)> getLoad) {
    getLoad_ = getLoad;
  }

  std::function<int64_t(const std::string&)> getGetLoad() {
    return getLoad_;
  }

  /**
   * Set failure injection parameters.
   */
  void setFailureInjection(FailureInjection fi) override {
    failureInjection_.set(fi);
  }

  void setGetHandler(getHandlerFunc func) {
    getHandler_ = func;
  }

  getHandlerFunc getGetHandler() {
    return getHandler_;
  }

  void setGetHeaderHandler(GetHeaderHandlerFunc func) {
    getHeaderHandler_ = func;
  }

  GetHeaderHandlerFunc getGetHeaderHandler() {
    return getHeaderHandler_;
  }
};

}} // apache::thrift
