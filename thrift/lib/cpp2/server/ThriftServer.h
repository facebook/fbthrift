/*
 * Copyright 2014-present Facebook, Inc.
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
#include <folly/Singleton.h>
#include <folly/SocketAddress.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/io/ShutdownSocketSet.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp2/security/SecurityKillSwitchPoller.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/core/TransportRoutingHandler.h>
#include <wangle/acceptor/ServerSocketConfig.h>
#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/ssl/SSLContextConfig.h>
#include <wangle/ssl/TLSCredProcessor.h>

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

  folly::Optional<bool> reusePort_;
  folly::Optional<bool> enableTFO_;
  uint32_t fastOpenQueueSize_{10000};

  folly::Optional<wangle::SSLCacheOptions> sslCacheOptions_;

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
  size_t nSaslPoolThreads_ = 0;
  std::string saslThreadsNamePrefix_ = "thrift-sasl";

  std::weak_ptr<folly::ShutdownSocketSet> wShutdownSocketSet_;

  //! Listen socket
  folly::AsyncServerSocket::UniquePtr socket_;

  struct IdleServerAction:
    public folly::HHWheelTimer::Callback
  {
    IdleServerAction(
      ThriftServer &server,
      folly::HHWheelTimer &timer,
      std::chrono::milliseconds timeout
    );

    void timeoutExpired() noexcept override;

    ThriftServer &server_;
    folly::HHWheelTimer &timer_;
    std::chrono::milliseconds timeout_;
  };

  //! The folly::EventBase currently driving serve().  NULL when not serving.
  std::atomic<folly::EventBase*> serveEventBase_{nullptr};
  folly::HHWheelTimer::UniquePtr serverTimer_;
  folly::Optional<IdleServerAction> idleServer_;
  std::chrono::milliseconds idleServerTimeout_ = std::chrono::milliseconds(0);
  folly::Optional<std::chrono::milliseconds> sslHandshakeTimeout_;
  std::atomic<std::chrono::steady_clock::duration::rep> lastRequestTime_;

  std::chrono::steady_clock::time_point lastRequestTime() const noexcept;
  void touchRequestTimestamp() noexcept;

  //! Thread stack size in MB
  int threadStackSizeMB_ = 1;

  //! Manager of per-thread EventBase objects.
  folly::EventBaseManager* eventBaseManager_ = folly::EventBaseManager::get();

  //! IO thread pool. Drives Cpp2Workers.
  std::shared_ptr<folly::IOThreadPoolExecutor> ioThreadPool_ =
      std::make_shared<folly::IOThreadPoolExecutor>(0);

  //! Separate thread pool for handling SSL handshakes.
  std::shared_ptr<folly::IOThreadPoolExecutor> sslHandshakePool_ =
      std::make_shared<folly::IOThreadPoolExecutor>(
          0,
          std::make_shared<folly::NamedThreadFactory>("SSLHandshakePool"));

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

  void handleSetupFailure(void);

  void updateCertsToWatch();

  // Minimum size of response before it might be compressed
  // Prevents small responses from being compressed,
  // does not by itself turn on compression. Either client
  // must request or a default transform must be set
  uint32_t minCompressBytes_ = 0;

  std::vector<uint16_t> writeTrans_;

  bool queueSends_ = true;

  // Whether or not we should try to approximate pending IO events for
  // load balancing and load shedding
  bool trackPendingIO_ = false;

  bool stopWorkersOnStopListening_ = true;
  std::chrono::seconds workersJoinTimeout_{30};

  std::shared_ptr<folly::IOThreadPoolExecutor> acceptPool_;
  int nAcceptors_ = 1;

  // HeaderServerChannel and Cpp2Worker to use for a duplex server
  // (used by client). Both are nullptr for a regular server.
  std::shared_ptr<HeaderServerChannel> serverChannel_;
  std::shared_ptr<Cpp2Worker> duplexWorker_;

  bool isDuplex_ = false;   // is server in duplex mode? (used by server)

  mutable std::mutex ioGroupMutex_;

  std::shared_ptr<folly::IOThreadPoolExecutor> getIOGroupSafe() const {
    std::lock_guard<std::mutex> lock(ioGroupMutex_);
    return getIOGroup();
  }

  wangle::TLSCredProcessor& getCredProcessor();

  SecurityKillSwitchPoller securityKillSwitchPoller_;
  std::unique_ptr<wangle::TLSCredProcessor> tlsCredProcessor_;

  std::unique_ptr<ThriftProcessor> thriftProcessor_;
  std::vector<std::unique_ptr<TransportRoutingHandler>> routingHandlers_;

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
   * be set to this number. Use ThreadServer::setNumIOWorkerThreads() to set
   * it afterwards if you want to change the number of works.
   *
   * @param the new thread pool
   */
  void setIOThreadPool(
      std::shared_ptr<folly::IOThreadPoolExecutor> ioThreadPool) {
    CHECK(configMutable());
    ioThreadPool_ = ioThreadPool;

    if (ioThreadPool_->numThreads() > 0) {
      nWorkers_ = ioThreadPool_->numThreads();
    }
  }

  /**
   * Doing any blocking work on this executor will cause the server to
   * stall servicing requests. Be careful about using this executor for anything
   * other than its main purpose.
   */
  std::shared_ptr<folly::IOThreadPoolExecutor> getIOThreadPool() {
    return ioThreadPool_;
  }

  /**
   * Set the thread factory that will be used to create the server's IO
   * threads.
   *
   * @param the new thread factory
   */
  void setIOThreadFactory(
      std::shared_ptr<folly::NamedThreadFactory> threadFactory) {
    CHECK(configMutable());
    ioThreadPool_->setThreadFactory(threadFactory);
  }

  size_t getNumSaslThreadsToRun() const;

  /**
   * Set the thread pool used to handle TLS handshakes. Note that the pool's
   * thread factory will be overridden - if you'd like to use your own, set it
   * afterwards via ThriftServer::setSSLHandshakeThreadFactory(). If the given
   * thread pool has one or more allocated threads, the number of workers will
   * be set to this number. Use ThriftServer::setNumSSLHandshakeWorkerThreads()
   * to set it afterwards if you want to change the number of workers.
   *
   * @param the new thread pool
   */
  void setSSLHandshakeThreadPool(
      std::shared_ptr<folly::IOThreadPoolExecutor> sslHandshakePool) {
    CHECK(configMutable());
    sslHandshakePool_ = sslHandshakePool;

    if (sslHandshakePool_->numThreads() > 0) {
      nSSLHandshakeWorkers_ = sslHandshakePool_->numThreads();
    }
  }

  /**
   * Set the thread factory that will be used to create the server's SSL
   * handshake threads.
   *
   * @param the new thread factory
   */
  void setSSLHandshakeThreadFactory(
      std::shared_ptr<folly::NamedThreadFactory> threadFactory) {
    CHECK(configMutable());
    sslHandshakePool_->setThreadFactory(threadFactory);
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
        std::dynamic_pointer_cast<folly::NamedThreadFactory>(factory);
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

  bool isOverloaded(
      const apache::thrift::transport::THeader* header = nullptr) override;

  int64_t getRequestLoad() override;
  std::string getLoadInfo(int64_t load) override;

  void setAcceptExecutor(std::shared_ptr<folly::IOThreadPoolExecutor> pool) {
    acceptPool_ = pool;
  }

  /**
   * Generally the acceptor should not do any work other than
   * accepting connections, so use this with care.
   */
  std::shared_ptr<folly::IOThreadPoolExecutor> getAcceptExecutor() {
    return acceptPool_;
  }

  void setNumAcceptThreads(int numAcceptThreads) {
    CHECK(!acceptPool_);
    nAcceptors_ = numAcceptThreads;
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
    updateCertsToWatch();
  }

  void setSSLCacheOptions(wangle::SSLCacheOptions options) {
    sslCacheOptions_ = std::move(options);
  }

  void setTicketSeeds(wangle::TLSTicketKeySeeds seeds) {
    ticketSeeds_ = seeds;
  }

  /**
   * Set the ssl handshake timeout.
   */
  void setSSLHandshakeTimeout(
      folly::Optional<std::chrono::milliseconds> timeout) {
    sslHandshakeTimeout_ = timeout;
  }

  folly::Optional<std::chrono::milliseconds> getSSLHandshakeTimeout() const {
    return sslHandshakeTimeout_;
  }

  /**
   * Stops the Thrift server if it's idle for the given time.
   */
  void setIdleServerTimeout(std::chrono::milliseconds timeout) {
    idleServerTimeout_ = timeout;
  }

  void updateTicketSeeds(wangle::TLSTicketKeySeeds seeds);

  void updateTLSCert();

  /**
   * Tells the thrift server to update ticket seeds with the contents of the
   * ticket path when modified.  The seed file previously being watched will
   * no longer be watched.  This is not thread safe.
   * If initializeTickets is true, seeds are set on the thrift server.
   */
  void watchTicketPathForChanges(
      const std::string& ticketPath,
      bool initializeTickets);

  void setFastOpenOptions(bool enableTFO, uint32_t fastOpenQueueSize) {
    enableTFO_ = enableTFO;
    fastOpenQueueSize_ = fastOpenQueueSize;
  }

  folly::Optional<bool> getTFOEnabled() {
    return enableTFO_;
  }

  void setReusePort(bool reusePort) {
    reusePort_ = reusePort;
  }

  folly::Optional<bool> getReusePort() {
    return reusePort_;
  }

  std::shared_ptr<wangle::SSLContextConfig>
  getSSLConfig() const {
    return sslContext_;
  }

  folly::Optional<wangle::TLSTicketKeySeeds>
  getTicketSeeds() const {
    return ticketSeeds_;
  }

  folly::Optional<wangle::SSLCacheOptions> getSSLCacheOptions() const {
    return sslCacheOptions_;
  }

  wangle::ServerSocketConfig getServerSocketConfig() {
    wangle::ServerSocketConfig config;
    if (getSSLConfig()) {
      config.sslContextConfigs.push_back(*getSSLConfig());
    }
    if (sslCacheOptions_) {
      config.sslCacheOptions = *sslCacheOptions_;
    }
    config.connectionIdleTimeout = getIdleTimeout();
    config.acceptBacklog = getListenBacklog();
    if (ticketSeeds_) {
      config.initialTicketSeeds = *ticketSeeds_;
    }
    if (enableTFO_) {
      config.enableTCPFastOpen = *enableTFO_;
      config.fastOpenQueueSize = fastOpenQueueSize_;
    }
    if (sslHandshakeTimeout_) {
      config.sslHandshakeTimeout = *sslHandshakeTimeout_;
    } else if (getIdleTimeout() == std::chrono::milliseconds::zero()) {
      // make sure a handshake that takes too long doesn't kill the connection
      config.sslHandshakeTimeout = std::chrono::milliseconds::zero();
    }
    // We want the server to start even if cert/key is missing as it may become
    // available later
    config.strictSSL = false;
    return config;
  }

  /**
   * Use the provided socket rather than binding to address_.  The caller must
   * call ::bind on this socket, but should not call ::listen.
   *
   * NOTE: ThriftServer takes ownership of this 'socket' so if binding fails
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
   * Get the ThriftServer's main event base.
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
      if (securityKillSwitchPoller_.isKillSwitchEnabled()) {
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
  void setNSaslPoolThreads(size_t nSaslPoolThreads) {
    CHECK(configMutable());
    nSaslPoolThreads_ = nSaslPoolThreads;
  }

  /**
   * Sets the number of threads to use for SASL negotiation if it has been
   * enabled.
   */
  size_t getNSaslPoolThreads() {
    return nSaslPoolThreads_;
  }

  /**
   * Sets the prefix used for SASL threads.
   */
  void setSaslThreadsNamePrefix(std::string saslThreadsNamePrefix) {
    CHECK(configMutable());
    saslThreadsNamePrefix_ = std::move(saslThreadsNamePrefix);
  }

  /**
   * Get the prefix used for SASL threads.
   */
  const std::string& getSaslThreadsNamePrefix() {
    return saslThreadsNamePrefix_;
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
  uint64_t getNumDroppedConnections() const override;

  /**
   * Clear all the workers.
   */
  void clearWorkers() {
    ioThreadPool_->join();
  }

  /**
   * Stops workers.  Call this method after calling stopListening() if you
   * disabled the stopping of workers upon stopListening() via
   * setStopWorkersOnStopListening() and need to stop the workers explicitly
   * before server destruction.
   */
  void stopWorkers();

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
   * Sets the timeout for joining workers
   */
  void setWorkersJoinTimeout(std::chrono::seconds timeout) {
    workersJoinTimeout_ = timeout;
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
   * Set the default write transforms to be used on replies. If client
   * sets transforms, server will reflect them. Otherwise, these will
   * be used.
   */
  void setDefaultWriteTransforms(std::vector<uint16_t>& writeTrans) {
    writeTrans_ = writeTrans;
  }

  /**
   * Returns default write transforms to be used on replies.
   */
  std::vector<uint16_t>& getDefaultWriteTransforms() { return writeTrans_; }

  /**
   * Clears our default write transforms
   */
  void clearDefaultWriteTransforms() {
    writeTrans_.clear();
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
   * Preferably use this method in order to start ThriftServer created for
   * DuplexChannel instead of the serve() method.
   */
  void startDuplex();

  /**
   * This method should be used to cleanly stop a ThriftServer created for
   * DuplexChannel before disposing the ThriftServer. The caller should pass in
   * a shared_ptr to this ThriftServer since the ThriftServer does not have a
   * way of getting that (does not inherit from enable_shared_from_this)
   */
  void stopDuplex(std::shared_ptr<ThriftServer> thisServer);

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
   * Set whether or not we should try to approximate pending IO events for
   * load balancing and load shedding.
   */
  void setTrackPendingIO(bool trackPendingIO) {
    trackPendingIO_ = trackPendingIO;
  }

  /**
   * Get whether or not we should try to approximate pending IO events for
   * load balancing and load shedding.
   */
  bool getTrackPendingIO() const {
    return trackPendingIO_;
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

  std::vector<std::unique_ptr<TransportRoutingHandler>> const*
  getRoutingHandlers() const {
    return &routingHandlers_;
  }

  void addRoutingHandler(
      std::unique_ptr<TransportRoutingHandler> routingHandler) {
    routingHandlers_.push_back(std::move(routingHandler));
  }

  void setDuplex(bool duplex) {
    // setDuplex may only be called on the server side.
    // serverChannel_ must be nullptr in this case
    CHECK(serverChannel_ == nullptr);
    CHECK(configMutable());
    isDuplex_ = duplex;
  }

  /**
   * Returns a reference to the processor that is used by custom transports
   */
  apache::thrift::ThriftProcessor* getThriftProcessor() {
    return thriftProcessor_.get();
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

  /**
   * Sets an explicit AsyncProcessorFactory and sets the ThriftProcessor
   * to use for custom transports
   */
  virtual void setProcessorFactory(
      std::shared_ptr<AsyncProcessorFactory> pFac) override {
    CHECK(configMutable());
    BaseThriftServer::setProcessorFactory(pFac);
    thriftProcessor_.reset(new ThriftProcessor(getCpp2Processor(), *this));
  }

  // ThriftServer by defaults uses a global ShutdownSocketSet, so all socket's
  // FDs are registered there. But in some tests you might want to simulate 2
  // ThriftServer running in different processes, so their ShutdownSocketSet are
  // different. In that case server should have their own SSS each so shutting
  // down FD from one doesn't interfere with shutting down sockets for the
  // other.
  void replaceShutdownSocketSet(
      const std::shared_ptr<folly::ShutdownSocketSet>& newSSS);
};

}} // apache::thrift

#endif // #ifndef THRIFT_SERVER_H_
