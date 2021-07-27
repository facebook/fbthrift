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

#ifndef THRIFT_SERVER_H_
#define THRIFT_SERVER_H_ 1

#include <array>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <map>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include <folly/Memory.h>
#include <folly/Singleton.h>
#include <folly/SocketAddress.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/experimental/coro/AsyncScope.h>
#include <folly/experimental/observer/Observer.h>
#include <folly/io/ShutdownSocketSet.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/server/PreprocessParams.h>
#include <thrift/lib/cpp2/server/RequestDebugLog.h>
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/server/TransportRoutingHandler.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_constants.h>
#include <wangle/acceptor/ServerSocketConfig.h>
#include <wangle/acceptor/SharedSSLContextManager.h>
#include <wangle/bootstrap/ServerBootstrap.h>
#include <wangle/ssl/SSLContextConfig.h>
#include <wangle/ssl/TLSCredProcessor.h>

DECLARE_bool(thrift_abort_if_exceeds_shutdown_deadline);
DECLARE_string(service_identity);

namespace apache {
namespace thrift {

// Forward declaration of classes
class Cpp2Connection;
class Cpp2Worker;
class ThriftServer;
class ThriftProcessor;
namespace rocket {
class ThriftRocketServerHandler;
}

enum class SSLPolicy { DISABLED, PERMITTED, REQUIRED };

typedef wangle::Pipeline<folly::IOBufQueue&, std::unique_ptr<folly::IOBuf>>
    Pipeline;

class ThriftTlsConfig : public wangle::CustomConfig {
 public:
  bool enableThriftParamsNegotiation{true};
  bool enableStopTLS{false};
};

class TLSCredentialWatcher {
 public:
  explicit TLSCredentialWatcher(ThriftServer* server);

  void setCertPathsToWatch(std::set<std::string> paths) {
    credProcessor_.setCertPathsToWatch(std::move(paths));
  }

  void setTicketPathToWatch(const std::string& path) {
    credProcessor_.setTicketPathToWatch(path);
  }

 private:
  wangle::TLSCredProcessor credProcessor_;
};

/**
 *   This is yet another thrift server.
 *   Uses cpp2 style generated code.
 */

class ThriftServer : public apache::thrift::BaseThriftServer,
                     public wangle::ServerBootstrap<Pipeline> {
 private:
  //! SSL context
  std::optional<folly::observer::Observer<wangle::SSLContextConfig>>
      sslContextObserver_;
  std::optional<wangle::TLSTicketKeySeeds> ticketSeeds_;
  folly::observer::CallbackHandle getSSLCallbackHandle();

  std::optional<bool> reusePort_;
  std::optional<bool> enableTFO_;
  uint32_t fastOpenQueueSize_{10000};

  std::optional<wangle::SSLCacheOptions> sslCacheOptions_;
  wangle::FizzConfig fizzConfig_;
  ThriftTlsConfig thriftConfig_;

  // Security negotiation settings
  std::optional<SSLPolicy> sslPolicy_;
  bool strictSSL_ = false;
  // whether we allow plaintext connections from loopback in REQUIRED mode
  bool allowPlaintextOnLoopback_ = false;

  std::weak_ptr<folly::ShutdownSocketSet> wShutdownSocketSet_;

  //! Listen socket
  folly::AsyncServerSocket::UniquePtr socket_;

  struct IdleServerAction : public folly::HHWheelTimer::Callback {
    IdleServerAction(
        ThriftServer& server,
        folly::HHWheelTimer& timer,
        std::chrono::milliseconds timeout);

    void timeoutExpired() noexcept override;

    ThriftServer& server_;
    folly::HHWheelTimer& timer_;
    std::chrono::milliseconds timeout_;
  };

  //! The folly::EventBase currently driving serve().  NULL when not serving.
  std::atomic<folly::EventBase*> serveEventBase_{nullptr};
  std::optional<IdleServerAction> idleServer_;
  std::chrono::milliseconds idleServerTimeout_ = std::chrono::milliseconds(0);
  std::optional<std::chrono::milliseconds> sslHandshakeTimeout_;
  std::atomic<std::chrono::steady_clock::duration::rep> lastRequestTime_;

  // Includes non-request events in Rocket. Only bumped if idleTimeout set.
  std::chrono::steady_clock::time_point lastRequestTime() const noexcept;
  void touchRequestTimestamp() noexcept;

  //! Manager of per-thread EventBase objects.
  folly::EventBaseManager* eventBaseManager_ = folly::EventBaseManager::get();

  //! IO thread pool. Drives Cpp2Workers.
  std::shared_ptr<folly::IOThreadPoolExecutor> ioThreadPool_ =
      std::make_shared<folly::IOThreadPoolExecutor>(
          0, std::make_shared<folly::NamedThreadFactory>("ThriftIO"));

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
  std::shared_ptr<wangle::SharedSSLContextManager> sharedSSLContextManager_;

  void handleSetupFailure(void);

  void updateCertsToWatch();

  bool queueSends_ = true;

  bool stopWorkersOnStopListening_ = true;
  bool joinRequestsWhenServerStops_{true};

  folly::AsyncWriter::ZeroCopyEnableFunc zeroCopyEnableFunc_;

  std::shared_ptr<folly::IOThreadPoolExecutor> acceptPool_;
  int nAcceptors_ = 1;
  uint16_t socketMaxReadsPerEvent_{16};

  // HeaderServerChannel and Cpp2Worker to use for a duplex server
  // (used by client). Both are nullptr for a regular server.
  std::shared_ptr<HeaderServerChannel> serverChannel_;
  std::shared_ptr<Cpp2Worker> duplexWorker_;

  bool isDuplex_ = false; // is server in duplex mode? (used by server)

  mutable std::mutex ioGroupMutex_;

  std::shared_ptr<folly::IOThreadPoolExecutor> getIOGroupSafe() const {
    std::lock_guard<std::mutex> lock(ioGroupMutex_);
    return getIOGroup();
  }

  void stopWorkers();
  void stopCPUWorkers();
  void stopAcceptingAndJoinOutstandingRequests();

  void callOnStartServing();
  void callOnStopServing();

  std::atomic<bool> stoppedListening_{false};

#if FOLLY_HAS_COROUTINES
  std::unique_ptr<folly::coro::CancellableAsyncScope> asyncScope_;
#endif

  bool stopAcceptingAndJoinOutstandingRequestsDone_{false};

  folly::Synchronized<std::optional<TLSCredentialWatcher>> tlsCredWatcher_{};

  std::unique_ptr<ThriftProcessor> thriftProcessor_;
  std::vector<std::unique_ptr<TransportRoutingHandler>> routingHandlers_;

  friend class Cpp2Connection;
  friend class Cpp2Worker;
  friend class rocket::ThriftRocketServerHandler;

  bool tosReflect_{false};
  uint32_t listenerTos_{0};

  std::optional<instrumentation::ServerTracker> tracker_;

  bool quickExitOnShutdownTimeout_ = false;

  std::atomic<bool> started_{false};

 public:
  ThriftServer();

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
      setNumIOWorkerThreads(ioThreadPool_->numThreads());
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

  // if overloaded, returns applicable overloaded exception code.
  folly::Optional<std::string> checkOverload(
      const transport::THeader::StringToStringMap* readHeaders = nullptr,
      const std::string* = nullptr) const final;

  // returns descriptive error if application is unable to process request
  PreprocessResult preprocess(
      const server::PreprocessParams& params) const final;

  std::string getLoadInfo(int64_t load) const override;

  /*
   * Use a ZeroCopyEnableFunc to decide when to use zerocopy mode
   * Ex: use zerocopy when the IOBuf chain exceeds a certain thresold
   * setZeroCopyEnableFunc([threshold](const std::unique_ptr<folly::IOBuf>& buf)
   * { return (buf->computeChainDataLength() > threshold);});
   */
  void setZeroCopyEnableFunc(folly::AsyncWriter::ZeroCopyEnableFunc func) {
    zeroCopyEnableFunc_ = std::move(func);
  }

  const folly::AsyncWriter::ZeroCopyEnableFunc& getZeroCopyEnableFunc() const {
    return zeroCopyEnableFunc_;
  }

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
   * Set the SSLContextConfig on the thrift server.
   */
  void setSSLConfig(std::shared_ptr<wangle::SSLContextConfig> context) {
    CHECK(configMutable());
    if (context) {
      setSSLConfig(folly::observer::makeObserver(
          [context = std::move(context)]() { return *context; }));
    }
    updateCertsToWatch();
  }

  /**
   * Set the SSLContextConfig on the thrift server. Note that the thrift server
   * keeps an observer on the SSLContextConfig. Whenever the SSLContextConfig
   * has an update, the observer callback would reset SSLContextConfig on all
   * acceptors.
   */
  void setSSLConfig(
      folly::observer::Observer<wangle::SSLContextConfig> contextObserver) {
    sslContextObserver_ = folly::observer::makeObserver(
        [observer = std::move(contextObserver)]() {
          auto context = **observer;
          context.isDefault = true;
          return context;
        });
  }

  void setFizzConfig(wangle::FizzConfig config) { fizzConfig_ = config; }

  void setThriftConfig(ThriftTlsConfig thriftConfig) {
    thriftConfig_ = thriftConfig;
  }

  void setSSLCacheOptions(wangle::SSLCacheOptions options) {
    sslCacheOptions_ = std::move(options);
  }

  void setTicketSeeds(wangle::TLSTicketKeySeeds seeds) { ticketSeeds_ = seeds; }

  /**
   * Set the ssl handshake timeout.
   */
  void setSSLHandshakeTimeout(
      std::optional<std::chrono::milliseconds> timeout) {
    sslHandshakeTimeout_ = timeout;
  }

  const std::optional<std::chrono::milliseconds>& getSSLHandshakeTimeout()
      const {
    return sslHandshakeTimeout_;
  }

  /**
   * Stops the Thrift server if it's idle for the given time.
   */
  void setIdleServerTimeout(std::chrono::milliseconds timeout) {
    idleServerTimeout_ = timeout;
  }

  /**
   * Configures maxReadsPerEvent for accepted connections, see
   * `folly::AsyncSocket::setMaxReadsPerEvent` for more details.
   */
  void setSocketMaxReadsPerEvent(uint16_t socketMaxReadsPerEvent) {
    socketMaxReadsPerEvent_ = socketMaxReadsPerEvent;
  }

  void updateTicketSeeds(wangle::TLSTicketKeySeeds seeds);

  void updateTLSCert();

  /**
   * Tells the thrift server to update ticket seeds with the contents of the
   * file ticketPath when modified and initialized the seeds with the contents
   * of the file ticketPath. The seed file previously being watched will no
   * longer be watched.  This is not thread safe.
   */
  void watchTicketPathForChanges(const std::string& ticketPath);

  void setFastOpenOptions(bool enableTFO, uint32_t fastOpenQueueSize) {
    enableTFO_ = enableTFO;
    fastOpenQueueSize_ = fastOpenQueueSize;
  }

  std::optional<bool> getTFOEnabled() { return enableTFO_; }

  void setReusePort(bool reusePort) { reusePort_ = reusePort; }

  std::optional<bool> getReusePort() { return reusePort_; }

  const std::optional<folly::observer::Observer<wangle::SSLContextConfig>>&
  getSSLConfig() const {
    return sslContextObserver_;
  }

  const std::optional<wangle::TLSTicketKeySeeds>& getTicketSeeds() const {
    return ticketSeeds_;
  }

  const std::optional<wangle::SSLCacheOptions>& getSSLCacheOptions() const {
    return sslCacheOptions_;
  }

  wangle::ServerSocketConfig getServerSocketConfig() {
    wangle::ServerSocketConfig config;
    if (sslContextObserver_.has_value()) {
      config.sslContextConfigs.push_back(*sslContextObserver_->getSnapshot());
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
    // By default, we set strictSSL to false. This means the server will start
    // even if cert/key is missing as it may become available later
    config.strictSSL = getStrictSSL() || getSSLPolicy() == SSLPolicy::REQUIRED;
    config.fizzConfig = fizzConfig_;
    config.customConfigMap["thrift_tls_config"] =
        std::make_shared<ThriftTlsConfig>(thriftConfig_);
    config.socketMaxReadsPerEvent = socketMaxReadsPerEvent_;

    config.useZeroCopy = !!zeroCopyEnableFunc_;
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
  void useExistingSocket(folly::AsyncServerSocket::UniquePtr socket);

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
  folly::EventBase* getServeEventBase() const { return serveEventBase_; }

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

  SSLPolicy getSSLPolicy() const;

  // Convenience method to check if SSLPolicy is explicitly set
  bool isSSLPolicySet() const { return sslPolicy_.has_value(); }

  void setSSLPolicy(SSLPolicy policy) { sslPolicy_ = policy; }

  void setStrictSSL(bool strictSSL) { strictSSL_ = strictSSL; }

  bool getStrictSSL() { return strictSSL_; }

  void setAllowPlaintextOnLoopback(bool allow) {
    allowPlaintextOnLoopback_ = allow;
  }

  bool isPlaintextAllowedOnLoopback() const {
    return allowPlaintextOnLoopback_;
  }

  static folly::observer::Observer<bool> enableStopTLS();

#if FOLLY_HAS_COROUTINES
  /**
   * Get CancellableAsyncScope that will be maintained by the Thrift Server.
   * Cancellation is requested when the server is stopping.
   */
  folly::coro::CancellableAsyncScope& getAsyncScope() { return *asyncScope_; }
#endif

  void setAcceptorFactory(
      const std::shared_ptr<wangle::AcceptorFactory>& acceptorFactory) {
    acceptorFactory_ = acceptorFactory;
  }

  /**
   * Get the speed of adjusting connection accept rate.
   */
  double getAcceptRateAdjustSpeed() const { return acceptRateAdjustSpeed_; }

  /**
   * Set the speed of adjusting connection accept rate.
   */
  void setAcceptRateAdjustSpeed(double speed) {
    CHECK(configMutable());
    acceptRateAdjustSpeed_ = speed;
  }

  /**
   * Enable/Disable TOS reflection on the server socket
   */
  void setTosReflect(bool enable) { tosReflect_ = enable; }

  /**
   * Get TOS reflection setting for the server socket
   */
  bool getTosReflect() const override { return tosReflect_; }

  /**
   * Set default TOS for listener/accepted connections
   */
  void setListenerTos(uint32_t tos) { listenerTos_ = tos; }

  /**
   * Get default TOS for listener socket
   */
  uint32_t getListenerTos() const override { return listenerTos_; }

  /**
   * Get the number of connections dropped by the AsyncServerSocket
   */
  uint64_t getNumDroppedConnections() const override;

  /**
   * Check if the server and all the handlers have started.
   */
  bool getStarted() const override {
    return started_.load(std::memory_order_acquire);
  }

  /**
   * Clear all the workers.
   */
  void clearWorkers() { ioThreadPool_->join(); }

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
   * If stopWorkersOnStopListening is disabled, then enabling
   * leakOutstandingRequestsWhenServerStops permits thriftServer->serve() to
   * return before all outstanding requests are joined.
   */
  void leakOutstandingRequestsWhenServerStops(bool leak) {
    CHECK(configMutable());
    joinRequestsWhenServerStops_ = !leak;
  }

  /**
   * Call this to complete initialization
   */
  void setup();

  /**
   * Create and start the default thread manager unless it already exists.
   */
  void setupThreadManager();

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
  void setQueueSends(bool queueSends) { queueSends_ = queueSends; }

  bool getQueueSends() { return queueSends_; }

  // client side duplex
  std::shared_ptr<HeaderServerChannel> getDuplexServerChannel() {
    return serverChannel_;
  }

  // server side duplex
  bool isDuplex() { return isDuplex_; }

  std::vector<std::unique_ptr<TransportRoutingHandler>> const*
  getRoutingHandlers() const {
    return &routingHandlers_;
  }

  void addRoutingHandler(
      std::unique_ptr<TransportRoutingHandler> routingHandler) {
    routingHandlers_.push_back(std::move(routingHandler));
  }

  void clearRoutingHandlers() { routingHandlers_.clear(); }

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
    for (auto& socket : ServerBootstrap::getSockets()) {
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
      std::shared_ptr<AsyncProcessorFactory> pFac) override;

  // ThriftServer by defaults uses a global ShutdownSocketSet, so all socket's
  // FDs are registered there. But in some tests you might want to simulate 2
  // ThriftServer running in different processes, so their ShutdownSocketSet are
  // different. In that case server should have their own SSS each so shutting
  // down FD from one doesn't interfere with shutting down sockets for the
  // other.
  void replaceShutdownSocketSet(
      const std::shared_ptr<folly::ShutdownSocketSet>& newSSS);

  static folly::observer::Observer<std::list<std::string>>
  defaultNextProtocols();

  bool getQuickExitOnShutdownTimeout() const {
    return quickExitOnShutdownTimeout_;
  }

  void setQuickExitOnShutdownTimeout(bool quickExitOnShutdownTimeout) {
    quickExitOnShutdownTimeout_ = quickExitOnShutdownTimeout;
  }

  /**
   * For each request debug stub, a snapshot information can be constructed to
   * persist some transitent states about the corresponding request.
   */
  class RequestSnapshot {
   public:
    explicit RequestSnapshot(const RequestsRegistry::DebugStub& stub)
        : methodName_(stub.getMethodName()),
          creationTimestamp_(stub.getTimestamp()),
          finishedTimestamp_(stub.getFinished()),
          protoId_(stub.getProtoId()),
          peerAddress_(*stub.getPeerAddress()),
          localAddress_(*stub.getLocalAddress()),
          rootRequestContextId_(stub.getRootRequestContextId()),
          reqId_(RequestsRegistry::getRequestId(rootRequestContextId_)),
          reqDebugLog_(collectRequestDebugLog(stub)) {
      auto requestPayload = rocket::unpack<RequestPayload>(stub.clonePayload());
      payload_ = std::move(*requestPayload->payload);
      auto& metadata = requestPayload->metadata;
      if (metadata.otherMetadata_ref()) {
        headers_ = std::move(*requestPayload->metadata.otherMetadata_ref());
      }
      clientId_ = metadata.clientId_ref().to_optional();
      serviceTraceMeta_ = metadata.serviceTraceMeta_ref().to_optional();
      auto req = stub.getRequest();
      DCHECK(
          req != nullptr || finishedTimestamp_.time_since_epoch().count() != 0);
      startedProcessing_ = req == nullptr ? true : stub.getStartedProcessing();
    }

    const std::string& getMethodName() const { return methodName_; }

    std::chrono::steady_clock::time_point getCreationTimestamp() const {
      return creationTimestamp_;
    }

    std::chrono::steady_clock::time_point getFinishedTimestamp() const {
      return finishedTimestamp_;
    }

    intptr_t getRootRequestContextId() const { return rootRequestContextId_; }

    const std::string& getRequestId() const { return reqId_; }

    bool getStartedProcessing() const { return startedProcessing_; }

    /**
     * Returns empty IOBuff if payload is not present.
     */
    const folly::IOBuf& getPayload() const { return payload_; }

    const transport::THeader::StringToStringMap& getHeaders() const {
      return headers_;
    }

    protocol::PROTOCOL_TYPES getProtoId() const { return protoId_; }

    const folly::SocketAddress& getLocalAddress() const {
      return localAddress_;
    }
    const folly::SocketAddress& getPeerAddress() const { return peerAddress_; }

    const std::vector<std::string>& getDebugLog() const { return reqDebugLog_; }

    const auto& clientId() const { return clientId_; }
    auto& clientId() { return clientId_; }

    const auto& serviceTraceMeta() const { return serviceTraceMeta_; }
    auto& serviceTraceMeta() { return serviceTraceMeta_; }

   private:
    const std::string methodName_;
    const std::chrono::steady_clock::time_point creationTimestamp_;
    const std::chrono::steady_clock::time_point finishedTimestamp_;
    const protocol::PROTOCOL_TYPES protoId_;
    folly::IOBuf payload_;
    transport::THeader::StringToStringMap headers_;
    std::optional<std::string> clientId_;
    std::optional<std::string> serviceTraceMeta_;
    folly::SocketAddress peerAddress_;
    folly::SocketAddress localAddress_;
    intptr_t rootRequestContextId_;
    const std::string reqId_;
    const std::vector<std::string> reqDebugLog_;
    bool startedProcessing_;
  };

  struct ConnectionSnapshot {
    size_t numActiveRequests{0};
    size_t numPendingWrites{0};
    std::chrono::steady_clock::time_point creationTime;
  };
  using RequestSnapshots = std::vector<RequestSnapshot>;
  using ConnectionSnapshots =
      std::unordered_map<folly::SocketAddress, ConnectionSnapshot>;
  struct ServerSnapshot {
    RecentRequestCounter::Values recentCounters;
    RequestSnapshots requests;
    ConnectionSnapshots connections;
  };
  struct SnapshotOptions {
    std::chrono::microseconds connectionsAgeMax;
  };
  folly::SemiFuture<ServerSnapshot> getServerSnapshot() {
    return getServerSnapshot(SnapshotOptions{});
  }
  folly::SemiFuture<ServerSnapshot> getServerSnapshot(
      const SnapshotOptions& options);

  /**
   * If shutdown does not complete within the configured worker join timeout,
   * then we schedule a task to dump the server's state to disk for
   * investigation.
   *
   * The implementor of the dumping logic should provide the the task as well
   * as an appropriate timeout -- we do not want to indefinitely block shutdown
   * in case the task deadlocks.
   */
  struct DumpSnapshotOnLongShutdownResult {
    folly::SemiFuture<folly::Unit> task;
    std::chrono::milliseconds timeout;
  };
};

template <typename AcceptorClass, typename SharedSSLContextManagerClass>
class ThriftAcceptorFactory : public wangle::AcceptorFactorySharedSSLContext {
 public:
  ThriftAcceptorFactory<AcceptorClass, SharedSSLContextManagerClass>(
      ThriftServer* server)
      : server_(server) {}

  std::shared_ptr<wangle::SharedSSLContextManager>
  initSharedSSLContextManager() {
    if constexpr (!std::is_same<SharedSSLContextManagerClass, void>::value) {
      sharedSSLContextManager_ = std::make_shared<SharedSSLContextManagerClass>(
          server_->getServerSocketConfig());
    }
    return sharedSSLContextManager_;
  }

  std::shared_ptr<wangle::Acceptor> newAcceptor(folly::EventBase* eventBase) {
    if (!sharedSSLContextManager_) {
      return AcceptorClass::create(server_, nullptr, eventBase);
    }
    auto acceptor = AcceptorClass::create(
        server_,
        nullptr,
        eventBase,
        sharedSSLContextManager_->getCertManager(),
        sharedSSLContextManager_->getContextManager(),
        sharedSSLContextManager_->getFizzContext());
    sharedSSLContextManager_->addAcceptor(acceptor);
    return acceptor;
  }

 protected:
  ThriftServer* server_;
};
using DefaultThriftAcceptorFactory = ThriftAcceptorFactory<Cpp2Worker, void>;

using DefaultThriftAcceptorFactorySharedSSLContext = ThriftAcceptorFactory<
    Cpp2Worker,
    wangle::SharedSSLContextManagerImpl<wangle::FizzConfigUtil>>;
} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_SERVER_H_
