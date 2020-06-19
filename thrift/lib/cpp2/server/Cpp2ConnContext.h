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

#ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
#define THRIFT_ASYNC_CPP2CONNCONTEXT_H_ 1

#include <memory>

#include <folly/CancellationToken.h>
#include <folly/Optional.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <wangle/ssl/SSLUtil.h>

using apache::thrift::concurrency::PriorityThreadManager;

namespace apache {
namespace thrift {

using ClientIdentityHook = std::function<std::unique_ptr<void, void (*)(void*)>(
    const folly::AsyncTransport* transport,
    X509* cert,
    const folly::SocketAddress& peerAddress)>;

class RequestChannel;
class TClientBase;

class Cpp2ConnContext : public apache::thrift::server::TConnectionContext {
 public:
  explicit Cpp2ConnContext(
      const folly::SocketAddress* address = nullptr,
      const folly::AsyncTransport* transport = nullptr,
      folly::EventBaseManager* manager = nullptr,
      const std::shared_ptr<RequestChannel>& duplexChannel = nullptr,
      const std::shared_ptr<X509> peerCert = nullptr /*overridden from socket*/,
      apache::thrift::ClientIdentityHook clientIdentityHook = nullptr)
      : userData_(nullptr, no_op_destructor),
        manager_(manager),
        duplexChannel_(duplexChannel),
        peerCert_(peerCert),
        peerIdentities_(nullptr, [](void*) {}),
        transport_(transport) {
    if (address) {
      peerAddress_ = *address;
    }
    if (transport) {
      transport->getLocalAddress(&localAddress_);
      auto cert = transport->getPeerCertificate();
      if (cert) {
        peerCert_ = cert->getX509();
      }
      securityProtocol_ = transport->getSecurityProtocol();

      if (localAddress_.getFamily() == AF_UNIX) {
        auto wrapper = transport_->getUnderlyingTransport<folly::AsyncSocket>();
        if (wrapper) {
          peerCred_ = PeerCred::queryFromSocket(wrapper->getNetworkSocket());
        }
      }
    }

    if (clientIdentityHook) {
      peerIdentities_ =
          clientIdentityHook(transport_, peerCert_.get(), peerAddress_);
    }
  }

  void reset() {
    peerAddress_.reset();
    localAddress_.reset();
    userData_.reset();
  }

  const folly::SocketAddress* getPeerAddress() const final {
    return &peerAddress_;
  }

  const folly::SocketAddress* getLocalAddress() const {
    return &localAddress_;
  }

  void setLocalAddress(const folly::SocketAddress& localAddress) {
    localAddress_ = localAddress;
  }

  void setRequestHeader(apache::thrift::transport::THeader* header) {
    header_ = header;
  }

  folly::EventBaseManager* getEventBaseManager() override {
    return manager_;
  }

  std::string getPeerCommonName() const {
    if (peerCert_) {
      if (auto cnPtr = wangle::SSLUtil::getCommonName(peerCert_.get())) {
        return std::move(*cnPtr);
      }
    }
    return std::string();
  }

  virtual std::shared_ptr<X509> getPeerCertificate() const {
    return peerCert_;
  }

  template <typename Client>
  std::shared_ptr<Client> getDuplexClient() {
    DCHECK(duplexChannel_);
    auto client = std::dynamic_pointer_cast<Client>(duplexClient_);
    if (!client) {
      duplexClient_.reset(new Client(duplexChannel_));
      client = std::dynamic_pointer_cast<Client>(duplexClient_);
    }
    return client;
  }

  virtual const std::string& getSecurityProtocol() const {
    return securityProtocol_;
  }

  virtual void* getPeerIdentities() const {
    return peerIdentities_.get();
  }

  const folly::AsyncTransport* getTransport() const {
    return transport_;
  }

  /**
   * Get the user data field.
   */
  virtual void* getUserData() const override {
    return userData_.get();
  }

  /**
   * Set the user data field.
   *
   * @param data         The new value for the user data field.
   * @param destructor   A function pointer to invoke when the connection
   *                     context is destroyed.  It will be invoked with the
   *                     contents of the user data field.
   *
   * @return Returns the old user data value.
   */
  virtual void* setUserData(void* data, void (*destructor)(void*) = nullptr)
      override {
    auto oldData = userData_.release();
    userData_ = {data, destructor ? destructor : no_op_destructor};
    return oldData;
  }

#ifndef _WIN32
  struct PeerEffectiveCreds {
    pid_t pid;
    uid_t uid;
    gid_t gid;
  };

  /**
   * Returns the connecting process ID, effective user ID, and effective user ID
   * of the unix socket peer. The connection may have terminated since that
   * time, so the PID may no longer map to a running process or the same process
   * that initially connected. Returns nullopt for TCP, on Windows, and if there
   * was an error retrieving the peer creds. In that case, call
   * `getPeerCredError` for the reason.
   *
   * On macOS, the pid field contains the effective pid. On Linux, there is no
   * distinction.
   */
  folly::Optional<PeerEffectiveCreds> getPeerEffectiveCreds() const {
    return peerCred_.getPeerEffectiveCreds();
  }
#endif

  /**
   * If the peer effective pid or uid are not available, it's possible
   * retrieving the information failed. Produce an error message with the
   * reason.
   */
  folly::Optional<std::string> getPeerCredError() const {
    return peerCred_.getError();
  }

  /**
   * Retrieve a new folly::CancellationToken that will be signaled when the
   * connection is closed.
   */
  folly::CancellationToken getCancellationToken() const {
    return cancellationSource_.getToken();
  }

  /**
   * Signal that the connection has been closed.
   *
   * This is intended to be called by the thrift server implementation code.
   *
   * Note that this will cause any CancellationCallback functions that have been
   * registered to run immediately in this thread.  If any of these callbacks
   * throw this will cause program termination.
   */
  void connectionClosed() {
    cancellationSource_.requestCancellation();
  }

 private:
  /**
   * Platform-independent representation of unix domain socket peer credentials,
   * e.g. ucred on Linux and xucred on macOS.
   *
   * Null implementation on Windows.
   */
  class PeerCred {
   public:
#ifndef _WIN32
    using StatusOrPid = pid_t;
#else
    // Even on Windows, differentiate between not initialized (not unix
    // domain socket), and unsupported platform.
    using StatusOrPid = int;
#endif

    /**
     * pid_t is guaranteed to be signed, so reserve non-positive values as
     * sentinels that indicate credential validity.
     * While negative pid_t values are possible, they are used to refer
     * to process groups and thus cannot occur in a process identifier.
     * Linux and macOS allow user IDs to span the entire range of a uint32_t,
     * so sentinal values must be stored in pid_t.
     */
    enum Validity : StatusOrPid {
      NotInitialized = -1,
      ErrorRetrieving = -2,
      UnsupportedPlatform = -3,
    };

    PeerCred() = default;
    PeerCred(const PeerCred&) = default;
    PeerCred& operator=(const PeerCred&) = default;

    /**
     * Query a socket for peer credentials.
     */
    static PeerCred queryFromSocket(folly::NetworkSocket socket);

#ifndef _WIN32
    folly::Optional<PeerEffectiveCreds> getPeerEffectiveCreds() const {
      return hasCredentials()
          ? folly::make_optional(PeerEffectiveCreds{pid_, uid_, gid_})
          : folly::none;
    }
#endif

    /**
     * If retrieving the effective credentials failed, return a string
     * containing the reason.
     */
    folly::Optional<std::string> getError() const;

   private:
    explicit PeerCred(Validity validity) : pid_{validity} {}

#ifndef _WIN32
    explicit PeerCred(pid_t pid, uid_t uid, gid_t gid)
        : pid_{pid}, uid_{uid}, gid_{gid} {}
#endif

    bool hasCredentials() const {
      return pid_ >= 0;
    }

    StatusOrPid pid_ = Validity::NotInitialized;
#ifndef _WIN32
    uid_t uid_ = 0;
    gid_t gid_ = 0;
#endif
  };

  std::unique_ptr<void, void (*)(void*)> userData_;
  folly::SocketAddress peerAddress_;
  folly::SocketAddress localAddress_;
  folly::EventBaseManager* manager_;
  std::shared_ptr<RequestChannel> duplexChannel_;
  std::shared_ptr<TClientBase> duplexClient_;
  std::shared_ptr<X509> peerCert_;
  std::unique_ptr<void, void (*)(void*)> peerIdentities_;
  std::string securityProtocol_;
  const folly::AsyncTransport* transport_;
  PeerCred peerCred_;
  // A CancellationSource that will be signaled when the connection is closed.
  folly::CancellationSource cancellationSource_;

  static void no_op_destructor(void* /*ptr*/) {}
};

class Cpp2ClientRequestContext
    : public apache::thrift::server::TConnectionContext {
 public:
  Cpp2ClientRequestContext() = default;
  void setRequestHeader(transport::THeader* header) {
    header_ = header;
  }
};

// Request-specific context
class Cpp2RequestContext : public apache::thrift::server::TConnectionContext {
 public:
  explicit Cpp2RequestContext(
      Cpp2ConnContext* ctx,
      apache::thrift::transport::THeader* header = nullptr)
      : TConnectionContext(header),
        ctx_(ctx),
        requestData_(nullptr, no_op_destructor) {}

  void setConnectionContext(Cpp2ConnContext* ctx) {
    ctx_ = ctx;
  }

  // Forward all connection-specific information
  const folly::SocketAddress* getPeerAddress() const override {
    return ctx_->getPeerAddress();
  }

  const folly::SocketAddress* getLocalAddress() const {
    return ctx_->getLocalAddress();
  }

  void reset() {
    ctx_->reset();
  }

  PriorityThreadManager::PRIORITY getCallPriority() {
    return header_->getCallPriority();
  }

  virtual std::vector<uint16_t>& getTransforms() {
    return header_->getWriteTransforms();
  }

  folly::EventBaseManager* getEventBaseManager() override {
    return ctx_->getEventBaseManager();
  }

  void* getUserData() const override {
    return ctx_->getUserData();
  }

  void* setUserData(void* data, void (*destructor)(void*) = nullptr) override {
    return ctx_->setUserData(data, destructor);
  }

  typedef void (*void_ptr_destructor)(void*);
  typedef std::unique_ptr<void, void_ptr_destructor> RequestDataPtr;

  // This data is set on a per request basis.
  void* getRequestData() const {
    return requestData_.get();
  }

  // Returns the old request data context so the caller can clean up
  RequestDataPtr setRequestData(
      void* data,
      void_ptr_destructor destructor = no_op_destructor) {
    RequestDataPtr oldData(data, destructor);
    requestData_.swap(oldData);
    return oldData;
  }

  virtual Cpp2ConnContext* getConnectionContext() const {
    return ctx_;
  }

  std::chrono::milliseconds getRequestTimeout() const {
    return requestTimeout_;
  }

  void setRequestTimeout(std::chrono::milliseconds requestTimeout) {
    requestTimeout_ = requestTimeout;
  }

  void setMethodName(std::string methodName) {
    methodName_ = std::move(methodName);
  }

  const std::string& getMethodName() const {
    return methodName_;
  }

  std::string releaseMethodName() {
    return std::move(methodName_);
  }

  void setProtoSeqId(int32_t protoSeqId) {
    protoSeqId_ = protoSeqId;
  }

  int32_t getProtoSeqId() {
    return protoSeqId_;
  }

  void setInteractionId(int64_t id) {
    interactionId_ = id;
  }

  int64_t getInteractionId() {
    return interactionId_;
  }

  void setInteractionCreate(InteractionCreate interactionCreate) {
    interactionCreate_ = std::move(interactionCreate);
  }

  folly::Optional<InteractionCreate>& getInteractionCreate() {
    return interactionCreate_;
  }

  server::TServerObserver::PreHandlerTimestamps& getTimestamps() {
    return timestamps_;
  }

 protected:
  static void no_op_destructor(void* /*ptr*/) {}

  apache::thrift::server::TServerObserver::CallTimestamps timestamps_;

 private:
  Cpp2ConnContext* ctx_;
  RequestDataPtr requestData_;
  std::chrono::milliseconds requestTimeout_{0};
  std::string methodName_;
  int32_t protoSeqId_{0};
  int64_t interactionId_{0};
  folly::Optional<InteractionCreate> interactionCreate_;
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
