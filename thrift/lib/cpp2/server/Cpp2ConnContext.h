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

#ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
#define THRIFT_ASYNC_CPP2CONNCONTEXT_H_ 1

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/SaslServer.h>

#include <folly/SocketAddress.h>

#include <memory>

using apache::thrift::concurrency::PriorityThreadManager;

namespace apache { namespace thrift {

class RequestChannel;
class TClientBase;

class Cpp2ConnContext : public apache::thrift::server::TConnectionContext {
 public:
  explicit Cpp2ConnContext(
    const folly::SocketAddress* address,
    const apache::thrift::async::TAsyncSocket* socket,
    apache::thrift::transport::THeader* header,
    const apache::thrift::SaslServer* sasl_server,
    apache::thrift::async::TEventBaseManager* manager,
    const std::shared_ptr<RequestChannel>& duplexChannel = nullptr)
    : header_(header),
      saslServer_(sasl_server),
      manager_(manager),
      duplexChannel_(duplexChannel) {
    if (address) {
      peerAddress_ = *address;
    }
    if (socket) {
      socket->getLocalAddress(&localAddress_);
    }
  }

  const folly::SocketAddress* getPeerAddress() const override {
    return &peerAddress_;
  }

  const folly::SocketAddress* getLocalAddress() const {
    return &localAddress_;
  }

  void reset() {
    peerAddress_.reset();
    localAddress_.reset();
    header_ = nullptr;
    cleanupUserData();
  }

  apache::thrift::transport::THeader* getHeader() override {
    return header_;
  }

  virtual void setSaslServer(const apache::thrift::SaslServer* sasl_server) {
    saslServer_ = sasl_server;
  }

  virtual const apache::thrift::SaslServer* getSaslServer() const {
    return saslServer_;
  }

  apache::thrift::async::TEventBaseManager* getEventBaseManager() override {
    return manager_;
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
 private:
  folly::SocketAddress peerAddress_;
  folly::SocketAddress localAddress_;
  apache::thrift::transport::THeader* header_;
  const apache::thrift::SaslServer* saslServer_;
  apache::thrift::async::TEventBaseManager* manager_;
  std::shared_ptr<RequestChannel> duplexChannel_;
  std::shared_ptr<TClientBase> duplexClient_;
};

// Request-specific context
class Cpp2RequestContext : public apache::thrift::server::TConnectionContext {

 public:
  explicit Cpp2RequestContext(Cpp2ConnContext* ctx)
      : ctx_(ctx), requestData_(nullptr, no_op_destructor) {
    setConnectionContext(ctx);
  }

  void setConnectionContext(Cpp2ConnContext* ctx) {
    ctx_ = ctx;
    if (ctx_) {
      auto header = ctx_->getHeader();
      if (header) {
        headers_ = header->getHeaders();
        transforms_ = header->getWriteTransforms();
        minCompressBytes_ = header->getMinCompressBytes();
        callPriority_ = header->getCallPriority();
      }
    }
  }

  // Forward all connection-specific information
  const folly::SocketAddress*
  getPeerAddress() const override {
    return ctx_->getPeerAddress();
  }

  const folly::SocketAddress* getLocalAddress() const {
    return ctx_->getLocalAddress();
  }

  void reset() {
    ctx_->reset();
  }

  // The following two header functions _are_ thread safe
  std::map<std::string, std::string> getHeaders() override {
    return headers_;
  }

  virtual std::map<std::string, std::string> getWriteHeaders() {
    return std::move(writeHeaders_);
  }

  std::map<std::string, std::string>* getHeadersPtr() override {
    return &headers_;
  }

  bool setHeader(const std::string& key, const std::string& value) override {
    writeHeaders_[key] = value;
    return true;
  }

  void setHeaders(std::map<std::string, std::string>&& headers) {
    writeHeaders_ = std::move(headers);
  }

  virtual std::vector<uint16_t>& getTransforms() {
    return transforms_;
  }

  virtual uint32_t getMinCompressBytes() {
    return minCompressBytes_;
  }

  PriorityThreadManager::PRIORITY getCallPriority() {
    return callPriority_;
  }

  CLIENT_TYPE getClientType() {
    return ctx_->getHeader()->getClientType();
  }

  std::map<std::string, std::string> releaseHeaders() {
    return ctx_->getHeader()->releaseHeaders();
  }

  virtual const apache::thrift::SaslServer* getSaslServer() const {
    return ctx_->getSaslServer();
  }

  apache::thrift::async::TEventBaseManager* getEventBaseManager() override {
    return ctx_->getEventBaseManager();
  }

  void* getUserData() const override {
    return ctx_->getUserData();
  }

  void* setUserData(void* data, void (*destructor)(void*) = nullptr) override {
    return ctx_->setUserData(data, destructor);
  }

  typedef void (*void_ptr_destructor)(void*);
  typedef std::unique_ptr<void, void_ptr_destructor> RequestData;

  // This data is set on a per request basis.
  void* getRequestData() const {
    return requestData_.get();
  }

  // Returns the old request data context so the caller can clean up
  RequestData setRequestData(
      void* data, void_ptr_destructor destructor = no_op_destructor) {

    RequestData oldData(data, destructor);
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

 protected:
  // Note:  Header is _not_ thread safe
  apache::thrift::transport::THeader* getHeader() override {
    return ctx_->getHeader();
  }

  static void no_op_destructor(void* /*ptr*/) {}

 private:
  Cpp2ConnContext* ctx_;

  RequestData requestData_;

  // Headers are per-request, not per-connection
  std::map<std::string, std::string> headers_;
  std::map<std::string, std::string> writeHeaders_;
  std::vector<uint16_t> transforms_;
  uint32_t minCompressBytes_;
  PriorityThreadManager::PRIORITY callPriority_;
  bool startedProcessing_ = false;
  std::chrono::milliseconds requestTimeout_{0};
};

} }

#endif // #ifndef THRIFT_ASYNC_CPP2CONNCONTEXT_H_
