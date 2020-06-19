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

#pragma once

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <vector>

#include <folly/Memory.h>
#include <folly/SocketAddress.h>
#include <folly/Synchronized.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServerEventHandler.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/ServerAttribute.h>
#include <thrift/lib/cpp2/server/ServerConfigs.h>

namespace apache {
namespace thrift {

class AdmissionStrategy;

typedef std::function<void(
    folly::EventBase*,
    std::shared_ptr<folly::AsyncTransport>,
    std::unique_ptr<folly::IOBuf>)>
    getHandlerFunc;

typedef std::function<void(
    const apache::thrift::transport::THeader*,
    const folly::SocketAddress*)>
    GetHeaderHandlerFunc;

using IsOverloadedFunc = folly::Function<bool(
    const transport::THeader::StringToStringMap*,
    const std::string*) const>;

using PreprocessFunc = folly::Function<PreprocessResult(
    const transport::THeader::StringToStringMap*,
    const std::string*) const>;

template <typename T>
class ThriftServerAsyncProcessorFactory : public AsyncProcessorFactory {
 public:
  explicit ThriftServerAsyncProcessorFactory(std::shared_ptr<T> t) {
    svIf_ = t;
  }
  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return std::unique_ptr<apache::thrift::AsyncProcessor>(
        new typename T::ProcessorType(svIf_.get()));
  }

 private:
  std::shared_ptr<T> svIf_;
};

/**
 *   Base class for thrift servers using cpp2 style generated code.
 */

class BaseThriftServer : public apache::thrift::concurrency::Runnable,
                         public apache::thrift::server::ServerConfigs {
 public:
  struct FailureInjection {
    FailureInjection()
        : errorFraction(0), dropFraction(0), disconnectFraction(0) {}

    // Cause a fraction of requests to fail
    float errorFraction;

    // Cause a fraction of requests to be dropped (and presumably time out
    // on the client)
    float dropFraction;

    // Cause a fraction of requests to cause the channel to be disconnected,
    // possibly failing other requests as well.
    float disconnectFraction;

    bool operator==(const FailureInjection& other) const {
      return errorFraction == other.errorFraction &&
          dropFraction == other.dropFraction &&
          disconnectFraction == other.disconnectFraction;
    }

    bool operator!=(const FailureInjection& other) const {
      return !(*this == other);
    }
  };

 private:
  //! Default number of worker threads (should be # of processor cores).
  static const size_t T_ASYNC_DEFAULT_WORKER_THREADS;

  static const uint32_t T_MAX_NUM_PENDING_CONNECTIONS_PER_WORKER = 4096;

  static const std::chrono::milliseconds DEFAULT_TIMEOUT;

  static const std::chrono::milliseconds DEFAULT_TASK_EXPIRE_TIME;

  static const std::chrono::milliseconds DEFAULT_STREAM_EXPIRE_TIME;

  static const std::chrono::milliseconds DEFAULT_QUEUE_TIMEOUT;

  /// Listen backlog
  static const int DEFAULT_LISTEN_BACKLOG = 1024;

  //! Prefix for pool thread names
  ServerAttribute<std::string> poolThreadName_{""};

  // Cpp2 ProcessorFactory.
  std::shared_ptr<apache::thrift::AsyncProcessorFactory> cpp2Pfac_;

  //! Number of io worker threads (may be set) (should be # of CPU cores)
  ServerAttribute<size_t> nWorkers_{T_ASYNC_DEFAULT_WORKER_THREADS};

  //! Number of SSL handshake worker threads (may be set)
  ServerAttribute<size_t> nSSLHandshakeWorkers_{0};

  //! Number of CPU worker threads
  ServerAttribute<size_t> nPoolThreads_{T_ASYNC_DEFAULT_WORKER_THREADS};

  ServerAttribute<bool> enableCodel_{false};

  //! Milliseconds we'll wait for data to appear (0 = infinity)
  ServerAttribute<std::chrono::milliseconds> timeout_{DEFAULT_TIMEOUT};

  /**
   * The time in milliseconds before an unperformed task expires
   * (0 == infinite)
   */
  ServerAttribute<std::chrono::milliseconds> taskExpireTime_{
      DEFAULT_TASK_EXPIRE_TIME};

  /**
   * The time in milliseconds before a stream starves of having no request.
   * (0 == infinite)
   */
  ServerAttribute<std::chrono::milliseconds> streamExpireTime_{
      DEFAULT_STREAM_EXPIRE_TIME};

  /**
   * The time we'll allow a task to wait on the queue and still perform it
   * (0 == infinite)
   */
  ServerAttribute<std::chrono::milliseconds> queueTimeout_{
      DEFAULT_QUEUE_TIMEOUT};

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
  ServerAttribute<int> listenBacklog_{DEFAULT_LISTEN_BACKLOG};

  /**
   * The maximum number of pending connections each io worker thread can hold.
   */
  ServerAttribute<uint32_t> maxNumPendingConnectionsPerWorker_{
      T_MAX_NUM_PENDING_CONNECTIONS_PER_WORKER};

  // Max number of active connections
  ServerAttribute<uint32_t> maxConnections_{0};

  // Max active requests
  ServerAttribute<uint32_t> maxRequests_{
      concurrency::ThreadManager::DEFAULT_MAX_QUEUE_SIZE};

  // If it is set true, server will check and use client timeout header
  ServerAttribute<bool> useClientTimeout_{true};

  // Max response size allowed. This is the size of the serialized and
  // transformed response, headers not included. 0 (default) means no limit.
  ServerAttribute<uint64_t> maxResponseSize_{0};

  // Admission strategy use for accepting new requests
  ServerAttribute<std::shared_ptr<AdmissionStrategy>> admissionStrategy_;

  /**
   * The maximum memory usage (in bytes) by each request debug payload.
   * Payloads larger than this value will be simply dropped by instrumentation.
   */
  ServerAttribute<uint64_t> maxDebugPayloadMemoryPerRequest_{0x1000000}; // 16MB

  /**
   * The maximum memory usage by each worker to keep track of debug payload.
   * Each time a request payload is added for tracking, the tracker should check
   * whether it's using memory beyond this value and evict payloads based on
   * its policies.
   */
  ServerAttribute<uint64_t> maxDebugPayloadMemoryPerWorker_{0x1000000}; // 16MB

  /**
   * The maximum number of debug payloads to track after request has finished.
   */
  ServerAttribute<uint16_t> maxFinishedDebugPayloadsPerWorker_{10};

  /**
   * Batch all writes withing given time interval.
   * (0 == disabled)
   */
  ServerAttribute<std::chrono::milliseconds> writeBatchingInterval_{
      std::chrono::milliseconds::zero()};

  /**
   * Trigger early flush when this number of writes are queued.
   * Ignored if write batching interval is not set.
   * (0 == disabled)
   */
  ServerAttribute<size_t> writeBatchingSize_{0};

  ServerAttributeUnsafe<folly::sorted_vector_set<std::string>>
      methodsBypassMaxRequestsLimit_{{}};

 protected:
  //! The server's listening addresses
  std::vector<folly::SocketAddress> addresses_;

  //! The server's listening port
  int port_ = -1;

  /**
   * The thread manager used for sync calls.
   */
  std::mutex threadManagerMutex_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;

  // If set, the thread factory that should be used to create worker threads.
  std::shared_ptr<concurrency::ThreadFactory> threadFactory_;

  std::shared_ptr<server::TServerEventHandler> eventHandler_;

  // Notification of various server events. Note that once observer_ has been
  // set, it cannot be set again and will remain alive for (at least) the
  // lifetime of *this.
  folly::Synchronized<std::shared_ptr<server::TServerObserver>> observer_;
  std::atomic<server::TServerObserver*> observerPtr_{nullptr};

  IsOverloadedFunc isOverloaded_;
  PreprocessFunc preprocess_;
  std::function<int64_t(const std::string&)> getLoad_;

  enum class InjectedFailure { NONE, ERROR, DROP, DISCONNECT };

  class CumulativeFailureInjection {
   public:
    CumulativeFailureInjection()
        : empty_(true),
          errorThreshold_(0),
          dropThreshold_(0),
          disconnectThreshold_(0) {}

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

  ClientIdentityHook clientIdentityHook_;

  // Flag indicating whether it is safe to mutate the server config through its
  // setters.
  std::atomic<bool> configMutable_{true};

  BaseThriftServer();
  ~BaseThriftServer() override {}

 public:
  std::shared_ptr<server::TServerEventHandler> getEventHandler() {
    return eventHandler_;
  }

  /**
   * If a view of the event handler is needed that does not need to extend its
   * lifetime beyond that of the BaseThriftServer, this method allows obtaining
   * the raw pointer rather than the more expensive shared_ptr.
   * Since unsynchronized setServerEventHandler / getEventHandler calls are not
   * permitted, use cases that get the handler, inform it of some action, and
   * then discard the handle immediately can use getEventHandlerUnsafe.
   */
  server::TServerEventHandler* getEventHandlerUnsafe() {
    return eventHandler_.get();
  }

  void setServerEventHandler(
      std::shared_ptr<server::TServerEventHandler> eventHandler) {
    eventHandler_ = std::move(eventHandler);
  }

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
   * Set the ThreadFactory that will be used to create worker threads for the
   * service.  If not set, a default factory will be used.  Must be called
   * before the thread manager is started.
   */
  void setThreadFactory(
      std::shared_ptr<concurrency::ThreadFactory> threadFactory) {
    CHECK(configMutable());
    CHECK(!threadManager_);
    threadFactory_ = std::move(threadFactory);
  }

  /**
   * Get the prefix for naming the CPU (pool) threads.
   *
   * @return current setting.
   */
  std::string getCPUWorkerThreadName() const {
    return poolThreadName_.get();
  }

  /**
   * DEPRECATED: Get the prefix for naming the CPU (pool) threads.
   * Use getCPUWorkerThreadName instead.
   *
   * @return current setting.
   */
  std::string getPoolThreadName() const {
    return getCPUWorkerThreadName();
  }

  /**
   * Set the prefix for naming the CPU (pool) threads. Not set by default.
   * must be called before serve() for it to take effect
   * ignored if setThreadManager() is called.
   *
   * @param cpuWorkerThreadName thread name prefix
   */
  void setCPUWorkerThreadName(
      const std::string& cpuWorkerThreadName,
      AttributeSource source = AttributeSource::OVERRIDE) {
    poolThreadName_.set(cpuWorkerThreadName, source);
  }

  /**
   * DEPRECATED: Set the prefix for naming the CPU (pool) threads. Not set by
   * default. Must be called before serve() for it to take effect
   * ignored if setThreadManager() is called.
   * Use setCPUWorkerThreadName instead.
   *
   * @param poolThreadName thread name prefix
   */
  inline void setPoolThreadName(const std::string& poolThreadName) {
    setCPUWorkerThreadName(poolThreadName);
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
    return maxConnections_.get();
  }

  /**
   * Set the maximum # of connections allowed before overload.
   *
   * @param maxConnections new setting for maximum # of connections.
   */
  void setMaxConnections(
      uint32_t maxConnections,
      AttributeSource source = AttributeSource::OVERRIDE) {
    maxConnections_.set(maxConnections, source);
  }

  /**
   * Get the maximum # of requests being processed in handler before overload.
   *
   * @return current setting.
   */
  uint32_t getMaxRequests() const {
    return maxRequests_.get();
  }

  /**
   * Set the maximum # of requests being processed in handler before overload.
   *
   * @param maxRequests new setting for maximum # of active requests.
   */
  void setMaxRequests(
      uint32_t maxRequests,
      AttributeSource source = AttributeSource::OVERRIDE) {
    maxRequests_.set(maxRequests, source);
  }

  uint64_t getMaxResponseSize() const final {
    return maxResponseSize_.get();
  }

  void setMaxResponseSize(
      uint64_t size,
      AttributeSource source = AttributeSource::OVERRIDE) {
    maxResponseSize_.set(size, source);
  }

  bool getUseClientTimeout() const {
    return useClientTimeout_.get();
  }

  void setUseClientTimeout(
      bool useClientTimeout,
      AttributeSource source = AttributeSource::OVERRIDE) {
    useClientTimeout_.set(useClientTimeout, source);
  }

  // Get load of the server.
  int64_t getLoad(const std::string& counter = "", bool check_custom = true)
      const final;
  virtual std::string getLoadInfo(int64_t load) const;

  void setObserver(const std::shared_ptr<server::TServerObserver>& observer) {
    auto locked = observer_.wlock();
    if (*locked) {
      throw std::logic_error("Server already has an observer installed");
    }
    *locked = observer;
    observerPtr_.store(locked->get());
  }

  server::TServerObserver* getObserver() const final {
    return observerPtr_.load(std::memory_order_relaxed);
  }

  std::shared_ptr<server::TServerObserver> getObserverShared() const {
    return observer_.copy();
  }

  std::unique_ptr<apache::thrift::AsyncProcessor> getCpp2Processor() {
    return cpp2Pfac_->getProcessor();
  }

  /**
   * Set the address(es) to listen on.
   */
  void setAddress(const folly::SocketAddress& address) {
    setAddresses({address});
  }

  void setAddress(folly::SocketAddress&& address) {
    setAddresses({std::move(address)});
  }

  void setAddress(const char* ip, uint16_t port) {
    setAddresses({folly::SocketAddress(ip, port)});
  }

  void setAddress(const std::string& ip, uint16_t port) {
    setAddresses({folly::SocketAddress(ip, port)});
  }

  void setAddresses(std::vector<folly::SocketAddress> addresses) {
    CHECK(!addresses.empty());
    CHECK(configMutable());
    port_ = -1;
    addresses_ = std::move(addresses);
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
    return addresses_.at(0);
  }

  const std::vector<folly::SocketAddress>& getAddresses() const {
    return addresses_;
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
    return maxNumPendingConnectionsPerWorker_.get();
  }
  /**
   * Set the maximum number of pending connections each io worker thread can
   * hold. No new connections will be sent to that io worker thread if there
   * are more than such number of unprocessed connections in that queue. If
   * every io worker thread's queue is full the connection will be dropped.
   */
  void setMaxNumPendingConnectionsPerWorker(
      uint32_t num,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    maxNumPendingConnectionsPerWorker_.set(num, source);
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
    return timeout_.get();
  }

  /** Set maximum number of milliseconds we'll wait for data (0 = infinity).
   *  Note: existing connections are unaffected by this call.
   *
   *  @param timeout number of milliseconds, or 0 to disable timeouts.
   */
  void setIdleTimeout(
      std::chrono::milliseconds timeout,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    timeout_.set(timeout, source);
  }

  /**
   * Set the number of IO worker threads
   *
   * @param number of IO worker threads
   */
  void setNumIOWorkerThreads(
      size_t numIOWorkerThreads,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    nWorkers_.set(numIOWorkerThreads, source);
  }

  /**
   * DEPRECATED: Set the number of IO worker threads
   * Use setNumIOWorkerThreads instead.
   *
   * @param number of IO worker threads
   */
  inline void setNWorkerThreads(size_t nWorkers) {
    setNumIOWorkerThreads(nWorkers);
  }

  /**
   * Get the number of IO worker threads
   *
   * @return number of IO worker threads
   */
  size_t getNumIOWorkerThreads() const final {
    return nWorkers_.get();
  }

  /**
   * DEPRECATED: Get the number of IO worker threads
   * Use getNumIOWorkerThreads instead.
   *
   * @return number of IO worker threads
   */
  inline size_t getNWorkerThreads() {
    return getNumIOWorkerThreads();
  }

  /**
   * Set the number of CPU (pool) threads.
   * Only valid if you do not also set a threadmanager. This controls the number
   * of normal priority threads; the Thrift thread manager can create additional
   * threads for other priorities.
   * If set to 0, the number of normal priority threads will be the same as
   * number of CPU cores.
   *
   * @param number of CPU (pool) threads
   */
  void setNumCPUWorkerThreads(
      size_t numCPUWorkerThreads,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    CHECK(!threadManager_);

    nPoolThreads_.set(numCPUWorkerThreads, source);
  }

  /**
   * DEPRECATED: Set the number of CPU (pool) threads
   * Only valid if you do not also set a threadmanager.
   * Use setNumCPUWorkerThreads instead.
   *
   * @param number of CPU (pool) threads
   */
  inline void setNPoolThreads(
      size_t nPoolThreads,
      AttributeSource source = AttributeSource::OVERRIDE) {
    setNumCPUWorkerThreads(nPoolThreads, source);
  }

  /**
   * Get the number of CPU (pool) threads
   *
   * @return number of CPU (pool) threads
   */
  size_t getNumCPUWorkerThreads() const {
    auto nCPUWorkers = nPoolThreads_.get();
    return nCPUWorkers ? nCPUWorkers : T_ASYNC_DEFAULT_WORKER_THREADS;
  }

  /**
   * DEPRECATED: Get the number of CPU (pool) threads
   * Use getNumCPUWorkerThreads instead.
   *
   * @return number of CPU (pool) threads
   */
  inline size_t getNPoolThreads() {
    return getNumCPUWorkerThreads();
  }

  /**
   * Set the number of SSL handshake worker threads.
   */
  void setNumSSLHandshakeWorkerThreads(
      size_t nSSLHandshakeThreads,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    nSSLHandshakeWorkers_.set(nSSLHandshakeThreads, source);
  }

  /**
   * Get the number of threads used to perform SSL handshakes
   */
  size_t getNumSSLHandshakeWorkerThreads() const {
    return nSSLHandshakeWorkers_.get();
  }

  /**
   * Codel queuing timeout - limit queueing time before overload
   * http://en.wikipedia.org/wiki/CoDel
   */
  void setEnableCodel(
      bool enableCodel,
      AttributeSource source = AttributeSource::OVERRIDE) {
    enableCodel_.set(enableCodel, source);
  }

  bool getEnableCodel() {
    return enableCodel_.get();
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
  virtual void setProcessorFactory(
      std::shared_ptr<AsyncProcessorFactory> pFac) {
    CHECK(configMutable());
    cpp2Pfac_ = pFac;
  }

  std::shared_ptr<apache::thrift::AsyncProcessorFactory> getProcessorFactory()
      const {
    return cpp2Pfac_;
  }

  /**
   * Set the task expire time
   *
   */
  void setTaskExpireTime(
      std::chrono::milliseconds timeout,
      AttributeSource source = AttributeSource::OVERRIDE) {
    taskExpireTime_.set(timeout, source);
  }

  /**
   * Get the task expire time
   *
   * @return task expire time
   */
  std::chrono::milliseconds getTaskExpireTime() const {
    return taskExpireTime_.get();
  }

  /**
   * Set the stream starvation time
   *
   */
  void setStreamExpireTime(
      std::chrono::milliseconds timeout,
      AttributeSource source = AttributeSource::OVERRIDE) {
    streamExpireTime_.set(timeout, source);
  }

  /**
   * If there is no request for the stream for the given time period, then the
   * stream will create timeout error.
   */
  std::chrono::milliseconds getStreamExpireTime() const final {
    return streamExpireTime_.get();
  }

  /**
   * Set the time requests are allowed to stay on the queue.
   * Note, queuing is an indication that your server cannot keep
   * up with load, and realtime systems should not queue. Only
   * override this if you do heavily batched requests.
   *
   * @return queue timeout
   */
  void setQueueTimeout(
      std::chrono::milliseconds timeout,
      AttributeSource source = AttributeSource::OVERRIDE) {
    queueTimeout_.set(timeout, source);
  }

  /**
   * Get the time requests are allowed to stay on the queue
   *
   * @return queue timeout
   */
  std::chrono::milliseconds getQueueTimeout() const {
    return queueTimeout_.get();
  }

  /**
   * Calls the twin function getTaskExpireTimeForRequest with the
   * clientQueueTimeoutMs and clientTimeoutMs fields retrieved from the THeader.
   */
  bool getTaskExpireTimeForRequest(
      const apache::thrift::transport::THeader& header,
      std::chrono::milliseconds& queueTimeout,
      std::chrono::milliseconds& taskTimeout) const;

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
      std::chrono::milliseconds clientQueueTimeoutMs,
      std::chrono::milliseconds clientTimeoutMs,
      std::chrono::milliseconds& queueTimeout,
      std::chrono::milliseconds& taskTimeout) const final;

  /**
   * Set the listen backlog. Refer to the comment on listenBacklog_ member for
   * details.
   */
  void setListenBacklog(
      int listenBacklog,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    listenBacklog_.set(listenBacklog, source);
  }

  /**
   * Get the listen backlog.
   *
   * @return listen backlog.
   */
  int getListenBacklog() const {
    return listenBacklog_.get();
  }

  void setIsOverloaded(IsOverloadedFunc isOverloaded) {
    isOverloaded_ = std::move(isOverloaded);
  }

  void setPreprocess(PreprocessFunc preprocess) {
    preprocess_ = std::move(preprocess);
  }

  void setMethodsBypassMaxRequestsLimit(
      const std::vector<std::string>& methods,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    methodsBypassMaxRequestsLimit_.set(
        folly::sorted_vector_set<std::string>{methods.begin(), methods.end()},
        source);
  }

  const folly::sorted_vector_set<std::string>&
  getMethodsBypassMaxRequestsLimit() const {
    return methodsBypassMaxRequestsLimit_.get();
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
  virtual void setFailureInjection(FailureInjection fi) {
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

  /**
   * Set the client identity hook for the server, which will be called in
   * Cpp2ConnContext(). It can be used to cache client identities for each
   * connection. They can be retrieved with Cpp2ConnContext::getPeerIdentities.
   */
  void setClientIdentityHook(ClientIdentityHook func) {
    clientIdentityHook_ = func;
  }

  ClientIdentityHook getClientIdentityHook() {
    return clientIdentityHook_;
  }

  virtual void serve() = 0;

  virtual void stop() = 0;

  // This API is intended to stop listening on the server
  // socket and stop accepting new connection first while
  // still letting the established connections to be
  // processed on the server.
  virtual void stopListening() = 0;

  // Allows running the server as a Runnable thread
  void run() override {
    serve();
  }

  /**
   * Set the admission strategy used by the Thrift Server
   */
  void setAdmissionStrategy(
      std::shared_ptr<AdmissionStrategy> admissionStrategy,
      AttributeSource source = AttributeSource::OVERRIDE) {
    admissionStrategy_.set(std::move(admissionStrategy), source);
  }

  /**
   * Return the admission strategy associated with the Thrift Server
   */
  std::shared_ptr<AdmissionStrategy> getAdmissionStrategy() const {
    return admissionStrategy_.get();
  }

  /**
   * Return the maximum memory usage by each debug payload.
   */
  uint64_t getMaxDebugPayloadMemoryPerRequest() {
    return maxDebugPayloadMemoryPerRequest_.get();
  }

  /**
   * Set the maximum memory usage by each debug payload.
   */
  void setMaxDebugPayloadMemoryPerRequest(
      uint64_t limit,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    maxDebugPayloadMemoryPerRequest_.set(limit, source);
  }

  /**
   * Return the maximum memory usage by each worker to keep track of debug
   * payloads.
   */
  uint64_t getMaxDebugPayloadMemoryPerWorker() {
    return maxDebugPayloadMemoryPerWorker_.get();
  }

  /**
   * Set the maximum memory usage by each worker to keep track of debug
   * payloads.
   */
  void setMaxDebugPayloadMemoryPerWorker(
      uint64_t limit,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    maxDebugPayloadMemoryPerWorker_.set(limit, source);
  }

  /**
   * Return the maximum memory usage by each worker to keep track of debug
   * payloads.
   */
  uint16_t getMaxFinishedDebugPayloadsPerWorker() {
    return maxFinishedDebugPayloadsPerWorker_.get();
  }

  /**
   * Set the maximum memory usage by each worker to keep track of debug
   * payloads.
   */
  void setMaxFinishedDebugPayloadsPerWorker(
      uint16_t limit,
      AttributeSource source = AttributeSource::OVERRIDE) {
    CHECK(configMutable());
    maxFinishedDebugPayloadsPerWorker_.set(limit, source);
  }

  /**
   * Set write batching interval
   */
  void setWriteBatchingInterval(
      std::chrono::milliseconds interval,
      AttributeSource source = AttributeSource::OVERRIDE) {
    writeBatchingInterval_.set(interval, source);
  }

  /**
   * Get write batching interval
   */
  std::chrono::milliseconds getWriteBatchingInterval() const {
    return writeBatchingInterval_.get();
  }

  /**
   * Set write batching size. Ignored if write batching interval is not set.
   */
  void setWriteBatchingSize(
      size_t batchingSize,
      AttributeSource source = AttributeSource::OVERRIDE) {
    writeBatchingSize_.set(batchingSize, source);
  }

  /**
   * Get write batching size
   */
  size_t getWriteBatchingSize() const {
    return writeBatchingSize_.get();
  }
};
} // namespace thrift
} // namespace apache
