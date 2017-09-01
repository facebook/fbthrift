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
#ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
#define THRIFT_ASYNC_CPP2CONNCONTEXT_H_ 1

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/SaslServer.h>

#include <folly/SocketAddress.h>
#include <wangle/ssl/SSLUtil.h>

#include <memory>

using apache::thrift::concurrency::PriorityThreadManager;

namespace apache { namespace thrift {

using ClientIdentityHook = std::function<std::unique_ptr<void, void (*)(void*)>(
    X509* cert,
    const apache::thrift::SaslServer* sasl,
    const folly::SocketAddress& peerAddress)>;

class RequestChannel;
class TClientBase;

class Cpp2ConnContext : public apache::thrift::server::TConnectionContext {
 public:
  explicit Cpp2ConnContext(
      const folly::SocketAddress* address = nullptr,
      const apache::thrift::async::TAsyncTransport* transport = nullptr,
      const apache::thrift::SaslServer* sasl_server = nullptr,
      folly::EventBaseManager* manager = nullptr,
      const std::shared_ptr<RequestChannel>& duplexChannel = nullptr,
      const std::shared_ptr<X509> peerCert = nullptr /*overridden from socket*/,
      apache::thrift::ClientIdentityHook clientIdentityHook = nullptr)
      : saslServer_(sasl_server),
        manager_(manager),
        requestHeader_(nullptr),
        duplexChannel_(duplexChannel),
        peerCert_(peerCert),
        peerIdentities_(nullptr, [](void*) {}) {
    if (address) {
      peerAddress_ = *address;
    }
    if (transport) {
      transport->getLocalAddress(&localAddress_);
      peerCert_ = transport->getPeerCert();
      securityProtocol_ = transport->getSecurityProtocol();
    }

    if (clientIdentityHook) {
      peerIdentities_ = clientIdentityHook(
          peerCert_.get(), sasl_server, peerAddress_);
    }
  }

  const folly::SocketAddress* getLocalAddress() const {
    return &localAddress_;
  }

  apache::thrift::transport::THeader* getHeader() const override {
    return requestHeader_;
  }

  void setRequestHeader(apache::thrift::transport::THeader* header) {
    requestHeader_ = header;
  }

  virtual void setSaslServer(const apache::thrift::SaslServer* sasl_server) {
    saslServer_ = sasl_server;
  }

  virtual const apache::thrift::SaslServer* getSaslServer() const {
    return saslServer_;
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

  std::shared_ptr<X509> getPeerCertificate() const {
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

  const std::string& getSecurityProtocol() const {
    return securityProtocol_;
  }

  bool isTls() const {
    return securityProtocol_ == "TLS";
  }

  virtual void* getPeerIdentities() const {
    return peerIdentities_.get();
  }

 private:
  const apache::thrift::SaslServer* saslServer_;
  folly::EventBaseManager* manager_;
  transport::THeader* requestHeader_;
  std::shared_ptr<RequestChannel> duplexChannel_;
  std::shared_ptr<TClientBase> duplexClient_;
  std::shared_ptr<X509> peerCert_;
  std::unique_ptr<void, void (*)(void*)> peerIdentities_;
  std::string securityProtocol_;
};

// Request-specific context
class Cpp2RequestContext : public apache::thrift::server::TConnectionContext {
 public:
  explicit Cpp2RequestContext(
      Cpp2ConnContext* ctx,
      apache::thrift::transport::THeader* header = nullptr)
      : ctx_(ctx),
        requestData_(nullptr, no_op_destructor),
        header_(header),
        startedProcessing_(false) {}

  void setConnectionContext(Cpp2ConnContext* ctx) {
    ctx_= ctx;
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

  apache::thrift::transport::THeader* getHeader() const override {
      return header_;
  }

  virtual const apache::thrift::SaslServer* getSaslServer() const {
    return ctx_->getSaslServer();
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
      void* data, void_ptr_destructor destructor = no_op_destructor) {

    RequestDataPtr oldData(data, destructor);
    requestData_.swap(oldData);
    return oldData;
  }

  virtual Cpp2ConnContext* getConnectionContext() const {
    return ctx_;
  }

  bool getStartedProcessing() const {
    return startedProcessing_;
  }

  void setStartedProcessing() {
    startedProcessing_ = true;
  }

  std::chrono::milliseconds getRequestTimeout() const {
    return requestTimeout_;
  }

  void setRequestTimeout(std::chrono::milliseconds requestTimeout) {
    requestTimeout_ = requestTimeout;
  }

  void setProcessingStartTime(
      std::chrono::time_point<std::chrono::steady_clock> processingStartTime) {
    processingStartTime_ = processingStartTime;
  }

  std::chrono::time_point<std::chrono::steady_clock> getProcessingStartTime()
      const {
    return processingStartTime_;
  }

  void setMethodName(std::string methodName) {
    methodName_ = std::move(methodName);
  }

  const std::string& getMethodName() {
    return methodName_;
  }

  void setProtoSeqId(int32_t protoSeqId) {
    protoSeqId_ = protoSeqId;
  }

  int32_t getProtoSeqId() {
    return protoSeqId_;
  }

  void setMessageBeginSize(uint32_t messageBeginSize) {
    messageBeginSize_ = messageBeginSize;
  }

  uint32_t getMessageBeginSize() {
    return messageBeginSize_;
  }

 protected:
  static void no_op_destructor(void* /*ptr*/) {}

 private:
  Cpp2ConnContext* ctx_;
  RequestDataPtr requestData_;
  apache::thrift::transport::THeader* header_;
  bool startedProcessing_ = false;
  std::chrono::milliseconds requestTimeout_{0};
  std::chrono::steady_clock::time_point processingStartTime_;
  std::string methodName_;
  int32_t protoSeqId_{0};
  uint32_t messageBeginSize_{0};
};

} }

#endif // #ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
