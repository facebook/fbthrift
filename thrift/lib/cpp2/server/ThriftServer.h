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

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <vector>

#include <folly/Memory.h>
#include <folly/io/ShutdownSocketSet.h>
#include <folly/io/async/EventBaseManager.h>
#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/concurrent/IOThreadPoolExecutor.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/transport/TTransportUtils.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/security/SecurityKillSwitchPoller.h>
#include <folly/Singleton.h>

#include <wangle/ssl/SSLContextConfig.h>
#include <wangle/acceptor/ServerSocketConfig.h>

namespace apache { namespace thrift {

// Forward declaration of classes
class Cpp2Connection;
class Cpp2Worker;

enum class SSLPolicy {
  DISABLED, PERMITTED, REQUIRED
};

typedef wangle::Pipeline<
  folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>> Pipeline;

/**
 *   This is yet another thrift server.
 *   Uses cpp2 style generated code.
 */

class ThriftServer : public apache::thrift::BaseThriftServer
                   , public wangle::ServerBootstrap<Pipeline> {
 private:
  //! SSL context
  std::shared_ptr<wangle::SSLContextConfig> sslContext_;
  folly::Optional<wangle::TLSTicketKeySeeds> ticketSeeds_;

  // Security negotiation settings
  bool saslEnabled_ = false;
  bool nonSaslEnabled_ = true;
  const std::string saslPolicy_;
  SSLPolicy sslPolicy_ = SSLPolicy::PERMITTED;
  const bool allowInsecureLoopback_;
  std::function<std::unique_ptr<SaslServer> (
    folly::EventBase*)> saslServerFactory_;
  std::shared_ptr<apache::thrift::concurrency::ThreadManager>
    saslThreadManager_;
  int nSaslPoolThreads_ = 0;

  std::unique_ptr<folly::ShutdownSocketSet> shutdownSocketSet_ =
      folly::make_unique<folly::ShutdownSocketSet>();

  //! Listen socket
  folly::AsyncServerSocket::UniquePtr socket_;

  //! The folly::EventBase currently driving serve().  NULL when not serving.
  std::atomic<folly::EventBase*> serveEventBase_{nullptr};

  //! Thread stack size in MB
  int threadStackSizeMB_ = 1;

  //! Manager of per-thread EventBase objects.
  folly::EventBaseManager* eventBaseManager_ = folly::EventBaseManager::get();

  //! IO thread pool. Drives Cpp2Workers.
  std::shared_ptr<wangle::IOThreadPoolExecutor> ioThreadPool_ =
      std::make_shared<wangle::IOThreadPoolExecutor>(0);

  /**
   * The speed for adjusting connection accept rate.
   * 0 for disabling auto adjusting connection accept rate.
   */
  double acceptRateAdjustSpeed_ = 0.0;

  /**
   * Acceptors accept and process incoming connections.  The acceptor factory
   * helps create acceptors.
   */
  std::shared_ptr<wangle::AcceptorFactory> acceptorFactory_;

  /**
   * ThreadFactory used to create worker threads
   */
  std::shared_ptr<apache::thrift::concurrency::ThreadFactory> threadFactory_;

  void addWorker();

  void stopWorkers();

  void handleSetupFailure(void);

  // Minimum size of response before it might be compressed
  // Prevents small responses from being compressed,
  // does not by itself turn on compression.  Client must
  // request compression.
  uint32_t minCompressBytes_ = 0;

  bool queueSends_ = true;

  bool stopWorkersOnStopListening_ = true;

  std::shared_ptr<wangle::IOThreadPoolExecutor> acceptPool_;

  // HeaderServerChannel and Cpp2Worker to use for a duplex server
  // (used by client). Both are nullptr for a regular server.
  std::shared_ptr<HeaderServerChannel> serverChannel_;
  std::unique_ptr<Cpp2Worker> duplexWorker_;

  bool isDuplex_ = false;   // is server in duplex mode? (used by server)

  mutable std::mutex ioGroupMutex_;

  std::shared_ptr<wangle::IOThreadPoolExecutor> getIOGroupSafe() const {
    std::lock_guard<std::mutex> lock(ioGroupMutex_);
    return getIOGroup();
  }

  friend class Cpp2Connection;
  friend class Cpp2Worker;

 public:
  ThriftServer();

  // If sasl_policy is set. FLAGS_sasl_policy will be ignored for this server
  ThriftServer(const std::string& sasl_policy,
               bool allow_insecure_loopback);


  // NOTE: Don't use this constructor to create a regular Thrift server. This
  // constructor is used by the client to create a duplex server on an existing
  // connection.
  // Don't create a listening server. Instead use the channel to run incoming
  // requests.
  explicit ThriftServer(
      const std::shared_ptr<HeaderServerChannel>& serverChannel);

  ~ThriftServer() override;

  /**
   * Set the thread pool used to drive the server's IO threads. Note that the
   * pool's thread factory will be overridden - if you'd like to use your own,
   * set it afterwards via ThriftServer::setIOThreadFactory(). If the given
   * thread pool has one or more allocated threads, the number of workers will
   * be set to this number. Use ThreadServer::setNWorkerThreads() to set
   * it afterwards if you want to change the number of works.
   *
   * @param the new thread pool
   */
  void setIOThreadPool(
      std::shared_ptr<wangle::IOThreadPoolExecutor> ioThreadPool) {
    CHECK(configMutable());
    ioThreadPool_ = ioThreadPool;

    if (ioThreadPool_->numThreads() > 0) {
      nWorkers_ = ioThreadPool_->numThreads();
    }
  }

  /**
   * Set the thread factory that will be used to create the server's IO threads.
   *
   * @param the new thread factory
   */
  void setIOThreadFactory(
      std::shared_ptr<wangle::NamedThreadFactory> threadFactory) {
    CHECK(configMutable());
    ioThreadPool_->setThreadFactory(threadFactory);
  }

  /**
   * Set the prefix for naming the worker threads. "Cpp2Worker" by default.
   * must be called before serve() for it to take effect
   *
   * @param cpp2WorkerThreadName net thread name prefix
   */
  void setCpp2WorkerThreadName(const std::string& cpp2WorkerThreadName) {
    CHECK(configMutable());
    auto factory = ioThreadPool_->getThreadFactory();
    CHECK(factory);
    auto namedFactory =
      std::dynamic_pointer_cast<wangle::NamedThreadFactory>(factory);
    CHECK(namedFactory);
    namedFactory->setNamePrefix(cpp2WorkerThreadName);
  }

  /**
   * Number of connections that epoll says need attention but ThriftServer
   * didn't have a chance to "ack" yet. A rough proxy for a number of pending
   * requests that are waiting to be processed (though it's an imperfect proxy
   * as there may be more than one request sent through a single connection).
   */
  int32_t getPendingCount() const;

  virtual bool isOverloaded(uint32_t workerActiveRequests = 0,
                    const apache::thrift::transport::THeader* header = nullptr) override;

  // Get load percent of the server.  Must be a number between 0 and 100:
  // 0 - no load, 100-fully loaded.
  int64_t getRequestLoad() override;
  int64_t getConnectionLoad() override;
  std::string getLoadInfo(int64_t reqload, int64_t connload, int64_t queueload) override;

  void setIOThreadPoolExecutor(
    std::shared_ptr<wangle::IOThreadPoolExecutor> pool) {
    acceptPool_ = pool;
  }

  std::shared_ptr<wangle::IOThreadPoolExecutor>
  getIOThreadPoolExecutor_() const {
    return acceptPool_;
  }

  /**
   *
   */
  void setSSLConfig(
    std::shared_ptr<wangle::SSLContextConfig> context) {
    CHECK(configMutable());
    if (context) {
      context->isDefault = true;
    }
    sslContext_ = context;
  }

  void setTicketSeeds(
      wangle::TLSTicketKeySeeds seeds) {
    ticketSeeds_ = seeds;
  }

  void updateTicketSeeds(wangle::TLSTicketKeySeeds seeds);

  std::shared_ptr<wangle::SSLContextConfig>
  getSSLConfig() const {
    return sslContext_;
  }

  folly::Optional<wangle::TLSTicketKeySeeds>
  getTicketSeeds() const {
    return ticketSeeds_;
  }

  wangle::ServerSocketConfig getServerSocketConfig() {
    wangle::ServerSocketConfig config;
    if (getSSLConfig()) {
      config.sslContextConfigs.push_back(*getSSLConfig());
    }
    config.connectionIdleTimeout = getIdleTimeout();
    config.acceptBacklog = getListenBacklog();
    if (ticketSeeds_) {
      config.initialTicketSeeds = *ticketSeeds_;
    }
    return config;
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
    folly::AsyncServerSocket::UniquePtr socket);

  /**
   * Return the file descriptor(s) associated with the listening socket
   */
  int getListenSocket() const;
  std::vector<int> getListenSockets() const;

  /**
   * Get the TEventServer's main event base.
   *
   * @return a pointer to the EventBase.
   */
  folly::EventBase* getServeEventBase() const {
    return serveEventBase_;
  }

  /**
   * Get the EventBaseManager used by this server.  This can be used to find
   * or create the EventBase associated with any given thread, including any
   * new threads created by clients.  This may be called from any thread.
   *
   * @return a pointer to the EventBaseManager.
   */
  folly::EventBaseManager* getEventBaseManager();
  const folly::EventBaseManager* getEventBaseManager() const {
    return const_cast<ThriftServer*>(this)->getEventBaseManager();
  }

  SSLPolicy getSSLPolicy() const {
    auto policy = sslPolicy_;
    if (policy == SSLPolicy::REQUIRED) {
      auto ksPoller = folly::Singleton<SecurityKillSwitchPoller>::try_get();
      if (ksPoller && ksPoller->isKillSwitchEnabled()) {
        policy = SSLPolicy::PERMITTED;
      }
    }
    return policy;
  }

  void setSSLPolicy(SSLPolicy policy) {
    sslPolicy_ = policy;
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
  bool getAllowInsecureLoopback() const {
    return allowInsecureLoopback_;
  }
  std::string getSaslPolicy() const {
    return saslPolicy_;
  }

  // The default SASL implementation can be overridden for testing or
  // other purposes.  Most users will never need to call this.
  void setSaslServerFactory(std::function<std::unique_ptr<SaslServer> (
      folly::EventBase*)> func) {
    saslServerFactory_ = func;
  }

  std::function<std::unique_ptr<SaslServer> (
      folly::EventBase*)> getSaslServerFactory() {
    return saslServerFactory_;
  }

  void setAcceptorFactory(
      const std::shared_ptr<wangle::AcceptorFactory>& acceptorFactory) {
    acceptorFactory_ = acceptorFactory;
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
   * Sets the number of threads to use for SASL negotiation if it has been
   * enabled.
   */
  void setNSaslPoolThreads(int nSaslPoolThreads) {
    CHECK(configMutable());
    nSaslPoolThreads_ = nSaslPoolThreads;
  }

  /**
   * Sets the number of threads to use for SASL negotiation if it has been
   * enabled.
   */
  int getNSaslPoolThreads() {
    return nSaslPoolThreads_;
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
    CHECK(configMutable());
    acceptRateAdjustSpeed_ = speed;
  }

  /**
   * Get the number of connections dropped by the AsyncServerSocket
   */
  virtual uint64_t getNumDroppedConnections() const override;

  /**
   * Clear all the workers.
   */
  void clearWorkers() {
    ioThreadPool_->join();
  }

  /**
   * Set the thread stack size in MB
   * Only valid if you do not also set a threadmanager.
   *
   * @param stack size in MB
   */
  void setThreadStackSizeMB(int stackSize) {
    assert(!threadFactory_);

    threadStackSizeMB_ = stackSize;
  }

  /**
   * Get the thread stack size
   *
   * @return thread stack size
   */
  int getThreadStackSizeMB() {
    return threadStackSizeMB_;
  }

  /**
   * Set whether to stop io workers when stopListening() is called (we do stop
   * them by default).
   */
  void setStopWorkersOnStopListening(bool stopWorkers) {
    CHECK(configMutable());
    stopWorkersOnStopListening_ = stopWorkers;
  }

  /**
   * Get whether to stop io workers when stopListening() is called.
   */
  bool getStopWorkersOnStopListening() const {
    return stopWorkersOnStopListening_;
  }

  /**
   * Set the thread factory used to create new worker threads
   */
  void setThreadFactory(
      std::shared_ptr<apache::thrift::concurrency::ThreadFactory> tf) {
    CHECK(configMutable());
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
  void serve() override;

  /**
   * Call this to stop the server, if started by serve()
   *
   * This causes the main serve() function to stop listening for new
   * connections, close existing connections, shut down the worker threads,
   * and then return.
   */
  void stop() override;

  /**
   * Call this to stop listening on the server port.
   *
   * This causes the main serve() function to stop listening for new
   * connections while still allows the worker threads to process
   * existing connections. stop() still needs to be called to clear
   * up the worker threads.
   */
  void stopListening() override;

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

  // client side duplex
  std::shared_ptr<HeaderServerChannel> getDuplexServerChannel() {
    return serverChannel_;
  }

  // server side duplex
  bool isDuplex() {
    return isDuplex_;
  }

  void setDuplex(bool duplex) {
    // setDuplex may only be called on the server side.
    // serverChannel_ must be nullptr in this case
    CHECK(serverChannel_ == nullptr);
    CHECK(configMutable());
    isDuplex_ = duplex;
  }

  const std::vector<std::shared_ptr<folly::AsyncServerSocket>> getSockets()
      const {
    std::vector<std::shared_ptr<folly::AsyncServerSocket>> serverSockets;
    auto sockets = ServerBootstrap::getSockets();
    for (auto& socket : sockets) {
      serverSockets.push_back(
          std::dynamic_pointer_cast<folly::AsyncServerSocket>(socket));
    }
    return serverSockets;
  }
};

}} // apache::thrift

#endif // #ifndef THRIFT_SERVER_H_
