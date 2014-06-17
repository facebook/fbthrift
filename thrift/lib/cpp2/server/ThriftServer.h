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

#ifndef THRIFT_SERVER_H_
#define THRIFT_SERVER_H_ 1

#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

#include "folly/Memory.h"
#include "thrift/lib/cpp/ShutdownSocketSet.h"
#include "thrift/lib/cpp/async/TAsyncServerSocket.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TEventBaseManager.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/server/TServer.h"
#include "thrift/lib/cpp/server/TServerObserver.h"
#include "thrift/lib/cpp/transport/TSSLSocket.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/lib/cpp/transport/TTransportUtils.h"
#include "thrift/lib/cpp2/Thrift.h"
#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/async/SaslServer.h"
#include "thrift/lib/cpp2/async/HeaderServerChannel.h"

namespace apache { namespace thrift {

typedef std::function<void(
  apache::thrift::async::TEventBase*,
  std::shared_ptr<apache::thrift::async::TAsyncTransport>,
  std::unique_ptr<folly::IOBuf>)> getHandlerFunc;

// Forward declaration of classes
class Cpp2Connection;
class Cpp2Worker;

template <typename T>
class ThriftServerAsyncProcessorFactory : public AsyncProcessorFactory {
  public:
    explicit ThriftServerAsyncProcessorFactory(std::shared_ptr<T> t) {
      svIf_ = t;
    }
    std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() {
      return std::unique_ptr<apache::thrift::AsyncProcessor> (
        new typename T::ProcessorType(svIf_.get()));
    }
  private:
    std::shared_ptr<T> svIf_;
};

/**
 *   This is yet another thrift server.
 *   Uses cpp2 style generated code.
 */

class ThriftServer : public apache::thrift::server::TServer {
 public:
  struct FailureInjection {
    FailureInjection()
      : errorFraction(0),
        dropFraction(0),
        disconnectFraction(0) {
    }

    // Cause a fraction of requests to fail
    float errorFraction;

    // Cause a fraction of requests to be dropped (and presumably time out
    // on the client)
    float dropFraction;

    // Cause a fraction of requests to cause the channel to be disconnected,
    // possibly failing other requests as well.
    float disconnectFraction;
  };

 protected:

  //! Default number of worker threads (should be # of processor cores).
  static const int T_ASYNC_DEFAULT_WORKER_THREADS;

  static const uint32_t T_MAX_NUM_MESSAGES_IN_QUEUE = 0xffffffff;

  static const std::chrono::milliseconds DEFAULT_TIMEOUT;

  static const std::chrono::milliseconds DEFAULT_TASK_EXPIRE_TIME;

  /// Listen backlog
  static const int DEFAULT_LISTEN_BACKLOG = 1024;

 private:
  struct WorkerInfo {
    std::shared_ptr<Cpp2Worker> worker;
    std::shared_ptr<apache::thrift::concurrency::Thread> thread;
  };

  //! Prefix for worker thread names
  std::string cpp2WorkerThreadName_;

  //! Prefix for pool thread names
  std::string poolThreadName_;

  //! SSL context
  std::shared_ptr<apache::thrift::transport::SSLContext> sslContext_;

  // Cpp2 ProcessorFactory.
  std::shared_ptr<apache::thrift::AsyncProcessorFactory> cpp2Pfac_;

  //! The server's listening address
  apache::thrift::transport::TSocketAddress address_;

  //! The server's listening port
  int port_;

  // Security negotiation settings
  bool saslEnabled_;
  bool nonSaslEnabled_;
  std::function<std::unique_ptr<SaslServer> (
    apache::thrift::async::TEventBase*)> saslServerFactory_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    saslThreadManager_;

  std::unique_ptr<apache::thrift::ShutdownSocketSet> shutdownSocketSet_;

  //! Listen socket
  apache::thrift::async::TAsyncServerSocket::UniquePtr socket_;

  //! The TEventBase currently driving serve().  NULL when not serving.
  std::atomic<apache::thrift::async::TEventBase*> serveEventBase_;

  //! Number of io worker threads (may be set) (should be # of CPU cores)
  int nWorkers_;

  //! Number of sync pool threads (may be set) (should be set to expected
  //  sync load)
  int nPoolThreads_;

  //! Milliseconds we'll wait for data to appear (0 = infinity)
  std::chrono::milliseconds timeout_;

  //! Manager of per-thread TEventBase objects.
  std::unique_ptr<apache::thrift::async::TEventBaseManager>
    eventBaseManagerHolder_;
  apache::thrift::async::TEventBaseManager* eventBaseManager_;
  std::mutex ebmMutex_;

  //! Last worker chosen -- used to select workers in round-robin sequence.
  uint32_t workerChoice_;

  //! List of workers.
  typedef std::vector<WorkerInfo> WorkerVector;
  WorkerVector workers_;

  /**
   * The thread manager used for sync calls.
   */
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;

  /**
   * The time in milliseconds before an unperformed task expires
   * (0 == infinite)
   */
  std::chrono::milliseconds taskExpireTime_;

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
  int listenBacklog_;

  /**
   * The speed for adjusting connection accept rate.
   * 0 for disabling auto adjusting connection accept rate.
   */
  double acceptRateAdjustSpeed_;

  /**
   * The maximum number of unprocessed messages which a NotificationQueue
   * can hold.
   */
  uint32_t maxNumMsgsInQueue_;

  /**
   * ThreadFactory used to create worker threads
   */
  std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory_;

  void addWorker();

  void stopWorkers();

  // Notification of various server events
  std::shared_ptr<apache::thrift::server::TServerObserver> observer_;

  // Max number of active connections
  uint32_t maxConnections_;

  // Max active requests
  uint32_t maxRequests_;

  // If it is set true, # of global active requests is tracked
  bool isUnevenLoad_;

  // If it is set true, server will check and use client timeout header
  bool useClientTimeout_;

  // Track # of global active requests
  static std::atomic<int32_t> globalActiveRequests_;

  // Minimum size of response before it might be compressed
  // Prevents small responses from being compressed,
  // does not by itself turn on compression.  Client must
  // request compression.
  uint32_t minCompressBytes_;

  std::function<bool(void)> isOverloaded_;

  bool queueSends_;

  bool enableCodel_;

  bool stopWorkersOnStopListening_;

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

  friend class Cpp2Connection;
  friend class Cpp2Worker;

  InjectedFailure maybeInjectFailure() const {
    return failureInjection_.test();
  }

  getHandlerFunc getHandler_;

 public:
  ThriftServer();

  virtual ~ThriftServer();

  /**
   * Set the prefix for naming the worker threads. "Cpp2Worker" by default.
   * must be called before serve() for it to take effect
   *
   * @param cpp2WorkerThreadName net thread name prefix
   */
  void setCpp2WorkerThreadName(const std::string& cpp2WorkerThreadName) {
    assert(workers_.size() == 0);
    cpp2WorkerThreadName_ = cpp2WorkerThreadName;
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

  /**
   * Get if the server expects uneven load among workers.
   *
   * @return current setting.
   */
  bool getIsUnevenLoad() const {
    return isUnevenLoad_;
  }

  /**
   * Set if the server expects uneven load among workers.
   *
   * @param isUnevenLoad new setting for the expected load.
   */
  void setIsUnevenLoad(bool isUnevenLoad) {
    isUnevenLoad_ = isUnevenLoad;
  }

  bool getUseClientTimeout() const {
    return useClientTimeout_;
  }

  void setUseClientTimeout(bool useClientTimeout) {
    useClientTimeout_ = useClientTimeout;
  }

  void incGlobalActiveRequests(int32_t numRequests = 1) {
    if (isUnevenLoad_) {
      globalActiveRequests_ += numRequests;
    }
  }

  void decGlobalActiveRequests(int32_t numRequests = 1) {
    if (isUnevenLoad_) {
      globalActiveRequests_ -= numRequests;
    }
  }

  int32_t getGlobalActiveRequests() const {
    return globalActiveRequests_;
  }

  /**
   * Number of connections that epoll says need attention but ThriftServer
   * didn't have a chance to "ack" yet. A rough proxy for a number of pending
   * requests that are waiting to be processed (though it's an imperfect proxy
   * as there may be more than one request sent through a single connection).
   */
  int32_t getPendingCount() const;

  bool isOverloaded(uint32_t workerActiveRequests = 0);

  void setObserver(
    const std::shared_ptr<apache::thrift::server::TServerObserver>& observer) {
    observer_ = observer;
  }

  const std::shared_ptr<apache::thrift::server::TServerObserver>&
  getObserver() const {
    return observer_;
  }

  /**
   *
   */
  void setSSLContext(
    std::shared_ptr<apache::thrift::transport::SSLContext> context) {
    assert(workers_.size() == 0);
    sslContext_ = context;
  }

  std::shared_ptr<apache::thrift::transport::SSLContext>
  getSSLContext() const {
    return sslContext_;
  }

  /**
   * Use the provided socket rather than binding to address_.  The caller must
   * call ::bind on this socket, but should not call ::listen.
   *
   * NOTE: TEventServe takes ownership of this 'socket' so if binding fails
   *       we destroy this socket, while cleaning itself up. So, 'accept' better
   *       work the first time :)
   */
  void useExistingSocket(int socket);
  void useExistingSockets(const std::vector<int>& sockets);
  void useExistingSocket(
    apache::thrift::async::TAsyncServerSocket::UniquePtr socket);

  /**
   * Return the file descriptor(s) associated with the listening socket
   */
  int getListenSocket() const;
  std::vector<int> getListenSockets() const;

  std::unique_ptr<apache::thrift::AsyncProcessor> getCpp2Processor() {
    return cpp2Pfac_->getProcessor();
  }

  /**
   * Get the TEventServer's main event base.
   *
   * @return a pointer to the TEventBase.
   */
  apache::thrift::async::TEventBase* getServeEventBase() const {
    return serveEventBase_;
  }

  /**
   * Get the TEventBaseManager used by this server.  This can be used to find
   * or create the TEventBase associated with any given thread, including any
   * new threads created by clients.  This may be called from any thread.
   *
   * @return a pointer to the TEventBaseManager.
   */
  apache::thrift::async::TEventBaseManager* getEventBaseManager();
  const apache::thrift::async::TEventBaseManager* getEventBaseManager() const {
    return const_cast<ThriftServer*>(this)->getEventBaseManager();
  }

  /**
   * Set the TEventBaseManager used by this server.  Must be called before
   * the first call to getEventBaseManager(), setup(), or serve().  The
   * caller retains ownership of the EBM.
   */
  void setEventBaseManager(apache::thrift::async::TEventBaseManager* ebm);

  /**
   * Set the address to listen on.
   */
  void setAddress(const apache::thrift::transport::TSocketAddress& address) {
    DCHECK(socket_ == nullptr);
    port_ = -1;
    address_ = address;
  }

  void setAddress(apache::thrift::transport::TSocketAddress&& address) {
    DCHECK(socket_ == nullptr);
    port_ = -1;
    address_ = std::move(address);
  }

  void setAddress(const char* ip, uint16_t port) {
    DCHECK(socket_ == nullptr);
    port_ = -1;
    address_.setFromIpPort(ip, port);
  }
  void setAddress(const std::string& ip, uint16_t port) {
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
  const apache::thrift::transport::TSocketAddress& getAddress() const {
    return address_;
  }

  /**
   * Set the port to listen on.
   */
  void setPort(uint16_t port) {
    DCHECK(socket_ == nullptr);
    port_ = port;
  }

  /**
   * Enable negotiation of SASL on received connections.  This
   * defaults to false.
   */
  void setSaslEnabled(bool enabled) {
    saslEnabled_ = enabled;
  }
  bool getSaslEnabled() {
    return saslEnabled_;
  }

  // The default SASL implementation can be overridden for testing or
  // other purposes.  Most users will never need to call this.
  void setSaslServerFactory(std::function<std::unique_ptr<SaslServer> (
      apache::thrift::async::TEventBase*)> func) {
    saslServerFactory_ = func;
  }

  std::function<std::unique_ptr<SaslServer> (
      apache::thrift::async::TEventBase*)> getSaslServerFactory() {
    return saslServerFactory_;
  }

  /**
   * Enable negotiation of insecure (non-SASL) on received
   * connections.  This defaults to true.
   */
  void setNonSaslEnabled(bool enabled) {
    nonSaslEnabled_ = enabled;
  }
  bool getNonSaslEnabled() {
    return nonSaslEnabled_;
  }

  /**
   * Get the maximum number of unprocessed messages which a NotificationQueue
   * can hold.
   */
  uint32_t getMaxNumMessagesInQueue() const {
    return maxNumMsgsInQueue_;
  }
  /**
   * Set the maximum number of unprocessed messages in NotificationQueue.
   * No new message will be sent to that NotificationQueue if there are more
   * than such number of unprocessed messages in that queue.
   */
  void setMaxNumMessagesInQueue(uint32_t num) {
    assert(workers_.size() == 0);
    maxNumMsgsInQueue_ = num;
  }

  /**
   * Get the speed of adjusting connection accept rate.
   */
  double getAcceptRateAdjustSpeed() const {
    return acceptRateAdjustSpeed_;
  }

  /**
   * Set the speed of adjusting connection accept rate.
   */
  void setAcceptRateAdjustSpeed(double speed) {
    assert(workers_.size() == 0);
    acceptRateAdjustSpeed_ = speed;
  }

  /**
   * Get the number of connections dropped by the TAsyncServerSocket
   */
  uint64_t getNumDroppedConnections() const;

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
    assert(workers_.size() == 0);
    timeout_ = timeout;
  }

  /**
   * Set the number of worker threads
   *
   * @param number of worker threads
   */
  void setNWorkerThreads(int nWorkers) {
    assert(workers_.size() == 0);
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
   * Get workers
   *
   * @return the workers
   */
  WorkerVector getWorkers() {
    return workers_;
  }

  /**
   * Clear all the workers.
   *
   * NOTE: We keep a shared_ptr to Thread in the workers_ vector. Therefore
   *       this just removes that one particular reference to the thread.
   *       If you have separate reference to those thread object, they will
   *       keep running
   */
  void clearWorkers() {
    workers_.clear();
  }

  /**
   * Set the number of pool threads
   * Only valid if you do not also set a threadmanager.
   *
   * @param number of pool threads
   */
  void setNPoolThreads(int nPoolThreads) {
    assert(workers_.size() == 0);
    assert(!threadManager_);

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
   * Set the processor factory as the one built into the
   * ServerInterface.
   *
   * setInterface() can take both unique_ptr and shared_ptr to handler
   * interface.
   *
   * @param handler interface shared_ptr
   */
  void setInterface(std::shared_ptr<ServerInterface> iface) {
    assert(workers_.size() == 0);
    cpp2Pfac_ = iface;
  }

  /**
   * Sets an explicit AsyncProcessorFactory
   *
   */
  void setProcessorFactory(
      std::unique_ptr<apache::thrift::AsyncProcessorFactory> pFac) {
    cpp2Pfac_ = std::shared_ptr<AsyncProcessorFactory>(std::move(pFac));
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
    assert(workers_.size() == 0);
    threadManager_ = threadManager;
  }

  /**
   * Get Thread Manager (for queuing mode).
   *
   * @return a shared pointer to the thread manager
   */
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
  getThreadManager() {
    return threadManager_;
  }

  /**
   * Set whether to stop io workers when stopListening() is called (we do stop
   * them by default).
   */
  void setStopWorkersOnStopListening(bool stopWorkers) {
    assert(workers_.size() == 0);
    stopWorkersOnStopListening_ = stopWorkers;
  }

  /**
   * Get whether to stop io workers when stopListening() is called.
   */
  bool getStopWorkersOnStopListening() const {
    return stopWorkersOnStopListening_;
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

  /**
   * Set the thread factory used to create new worker threads
   */
  void setThreadFactory(
      std::shared_ptr<apache::thrift::concurrency::ThreadFactory> tf) {
    assert(workers_.size() == 0);
    threadFactory_ = tf;
  }

  /**
   * Get the minimum response compression size
   *
   * @return minimum response compression size
   */
  uint32_t getMinCompressBytes() const {
    return minCompressBytes_;
  }

  /**
   * Set the minimum compressioin size
   *
   */
  void setMinCompressBytes(uint32_t bytes) {
    minCompressBytes_ = bytes;
  }

  /**
   * Call this to complete initialization
   */
  void setup();

  /**
   * Kill the workers and wait for listeners to quit
   */
  void cleanUp();

  /**
   * One stop solution:
   *
   * starts worker threads, enters accept loop; when
   * the accept loop exits, shuts down and joins workers.
   */
  void serve();

  /**
   * Call this to stop the server, if started by serve()
   *
   * This causes the main serve() function to stop listening for new
   * connections, close existing connections, shut down the worker threads,
   * and then return.
   */
  void stop();

  /**
   * Call this to stop listening on the server port.
   *
   * This causes the main serve() function to stop listening for new
   * connections while still allows the worker threads to process
   * existing connections. stop() still needs to be called to clear
   * up the worker threads.
   */
  void stopListening();

  /**
   * Set a function which determines whether we are currently overloaded. If
   * we are, the server will return a load-shed response.
   */
  void setIsOverloaded(std::function<bool(void)> isOverloaded) {
    isOverloaded_ = isOverloaded;
  }

  /**
   * Immediate shutdown of all connections. This is a hard-hitting hammer;
   * all reads and writes will return errors and no new connections will
   * be accepted.
   *
   * To be used only in dire situations. We're using it from the failure
   * signal handler to close all connections quickly, even though the server
   * might take multiple seconds to finish crashing.
   *
   * The optional bool parameter indicates whether to set the active
   * connections in the ShutdownSocketSet to not linger.  The effect of that
   * includes RST packets being immediately sent to clients which will result
   * in errors (and not normal EOF) on the client side.  This also causes
   * the local (ip, tcp port number) tuple to be reusable immediately, instead
   * of having to wait the standard amount of time.  For full details see
   * the `shutdown` method of `ShutdownSocketSet` (incl. notes about the
   * `abortive` parameter).
   */
  void immediateShutdown(bool abortConnections = false);

  /**
   * Queue sends - better throughput by avoiding syscalls, but can increase
   * latency for low-QPS servers.  Defaults to true
   */
  void setQueueSends(bool queueSends) {
    queueSends_ = queueSends;
  }

  bool getQueueSends() {
    return queueSends_;
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
   * Set failure injection parameters.
   */
  void setFailureInjection(FailureInjection fi) { failureInjection_.set(fi); }

  void setGetHandler(getHandlerFunc func) {
    getHandler_ = func;
  }

  getHandlerFunc getGetHandler() {
    return getHandler_;
  }
};

}} // apache::thrift

#endif // #ifndef THRIFT_SERVER_H_
