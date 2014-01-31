/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#include "thrift/lib/cpp2/server/Cpp2Connection.h"

#include "thrift/lib/cpp/async/TEventConnection.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "thrift/lib/cpp2/server/Cpp2Worker.h"
#include "thrift/lib/cpp2/security/SecurityKillSwitch.h"
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <assert.h>

namespace apache { namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using apache::thrift::TApplicationException;

Cpp2Connection::Cpp2Connection(
  std::shared_ptr<TAsyncSocket> asyncSocket,
  const TSocketAddress* address,
  Cpp2Worker* worker)
    : processor_(worker->getServer()->getCpp2Processor())
    , channel_(new HeaderServerChannel(asyncSocket))
    , worker_(worker)
    , context_(address,
               asyncSocket.get(),
               channel_->getHeader(),
               channel_->getSaslServer(),
               worker->getServer()->getEventBaseManager())
    , socket_(asyncSocket) {
  channel_->setQueueSends(worker->getServer()->getQueueSends());
  channel_->getHeader()->setMinCompressBytes(
    worker_->getServer()->getMinCompressBytes());
  auto observer = worker->getServer()->getObserver();
  if (observer) {
    channel_->setSampleRate(observer->getSampleRate());
  }

  // If the security kill switch is present, make the security policy default
  // to "permitted" or "disabled".
  if (isSecurityKillSwitchEnabled()) {
    worker_->getServer()->setNonSaslEnabled(true);
  }

  if (worker_->getServer()->getSaslEnabled() &&
      worker_->getServer()->getNonSaslEnabled()) {
    // Check if we need to use a stub, otherwise set the kerberos principal
    auto factory = worker_->getServer()->getSaslServerFactory();
    if (factory) {
      channel_->setSaslServer(
        unique_ptr<SaslServer>(factory(asyncSocket->getEventBase()))
      );
    } else {
      channel_->getSaslServer()->setServiceIdentity(
        worker_->getServer()->getServicePrincipal());
    }
    channel_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
  } else if (worker_->getServer()->getSaslEnabled()) {
    // Check if we need to use a stub, otherwise set the kerberos principal
    auto factory = worker_->getServer()->getSaslServerFactory();
    if (factory) {
      channel_->setSaslServer(
        unique_ptr<SaslServer>(factory(asyncSocket->getEventBase()))
      );
    } else {
      channel_->getSaslServer()->setServiceIdentity(
        worker_->getServer()->getServicePrincipal());
    }
    channel_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
  } else {
    // If neither is set, act as if non-sasl was specified.
    channel_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
  }

  auto handler = worker->getServer()->getEventHandler();
  if (handler) {
    handler->newConnection(&context_);
  }
}

Cpp2Connection::~Cpp2Connection() {
  cancelTimeout();
  channel_.reset();
}

void Cpp2Connection::stop() {
  cancelTimeout();
  for (auto req : activeRequests_) {
    req->cancelRequest();
    auto observer = worker_->getServer()->getObserver();
    if (observer) {
      observer->taskKilled();
    }
  }

  if (channel_) {
    channel_->setCallback(nullptr);
  }

  auto handler = worker_->getServer()->getEventHandler();
  if (handler) {
    handler->connectionDestroyed(&context_);
  }
}

void Cpp2Connection::timeoutExpired() noexcept {
  // Only disconnect if there are no active requests. No need to set another
  // timeout here because it's going to be set when all the requests are
  // handled.
  if (activeRequests_.empty()) {
    disconnect("idle timeout");
  }
}

void Cpp2Connection::disconnect(const char* comment) noexcept {
  // This must be the last call, it may delete this.
  folly::ScopeGuard guard = folly::makeGuard([&]{
    worker_->closeConnection(shared_from_this());
  });

  VLOG(1) << "ERROR: Disconnect: " << comment << " on channel: " <<
    context_.getPeerAddress()->describe();
  auto observer = worker_->getServer()->getObserver();
  if (observer) {
    observer->connDropped();
  }
}

void Cpp2Connection::requestTimeoutExpired() {
  VLOG(1) << "ERROR: Task expired on channel: " <<
    context_.getPeerAddress()->describe();
  auto observer = worker_->getServer()->getObserver();
  if (observer) {
    observer->taskKilled();
  }
}

bool Cpp2Connection::pending() {
  return socket_->isPending();
}

void Cpp2Connection::killRequest(
    ResponseChannel::Request& req,
    TApplicationException::TApplicationExceptionType reason,
    const char* comment) {
  VLOG(1) << "ERROR: Task killed: " << comment
          << ": " << context_.getPeerAddress()->getAddressStr();

  auto server = worker_->getServer();
  auto observer = server->getObserver();
  if (observer) {
    observer->taskKilled();
  }

  // Nothing to do for Thrift oneway request.
  if (req.isOneway()) {
    return;
  }

  // Thrift1 oneway request doesn't use ONEWAY_REQUEST_ID and
  // may end up here. No need to send error back for such requests
  if (!processor_->isOnewayMethod(req.getBuf(),
      channel_->getHeader())) {
    apache::thrift::TApplicationException x(reason, comment);
    req.sendError(std::make_exception_ptr(x), kOverloadedErrorCode);
  } else {
    // Send an empty request so reqId will be handler properly
    req.sendReply(std::unique_ptr<folly::IOBuf>());
  }
}

// Response Channel callbacks
void Cpp2Connection::requestReceived(
  unique_ptr<ResponseChannel::Request>&& req) {

  auto server = worker_->getServer();
  auto observer = server->getObserver();

  auto injectedFailure = server->maybeInjectFailure();
  switch (injectedFailure) {
  case ThriftServer::InjectedFailure::NONE:
    break;
  case ThriftServer::InjectedFailure::ERROR:
    killRequest(*req,
        TApplicationException::TApplicationExceptionType::INJECTED_FAILURE,
        "injected failure");
    return;
  case ThriftServer::InjectedFailure::DROP:
    VLOG(1) << "ERROR: injected drop: "
            << context_.getPeerAddress()->getAddressStr();
    return;
  case ThriftServer::InjectedFailure::DISCONNECT:
    disconnect("injected failure");
    return;
  }

  int activeRequests = worker_->activeRequests_;
  activeRequests += worker_->getPendingCount();

  if (server->isOverloaded(activeRequests)) {
    killRequest(*req,
        TApplicationException::TApplicationExceptionType::LOADSHEDDING,
        "loadshedding request");
    return;
  }

  server->incGlobalActiveRequests();
  if (req->timestamps_.readBegin != 0) {
    // Expensive operations; this happens once every
    // TServerCounters.sampleRate
    req->timestamps_.processBegin =
      apache::thrift::concurrency::Util::currentTimeUsec();
    if (observer) {
      observer->queuedRequests(
        server->getThreadManager()->pendingTaskCount());
      if (server->getIsUnevenLoad()) {
        observer->activeRequests(server->getGlobalActiveRequests());
      }
    }
  }

  unique_ptr<folly::IOBuf> buf = req->getBuf()->clone();
  unique_ptr<Cpp2Request> t2r(
    new Cpp2Request(std::move(req), shared_from_this()));
  activeRequests_.insert(t2r.get());
  ++worker_->activeRequests_;

  if (observer) {
    observer->receivedRequest();
  }

  auto timeoutTime = server->getTaskExpireTime();
  if (server->useClientTimeout_) {
    auto clientTimeoutTime = channel_->getHeader()->getClientTimeout();
    if (clientTimeoutTime > std::chrono::milliseconds(0) &&
        clientTimeoutTime < timeoutTime) {
      timeoutTime = clientTimeoutTime;
    }
  }
  if (timeoutTime > std::chrono::milliseconds(0)) {
    worker_->scheduleTimeout(t2r.get(), timeoutTime);
    t2r->setStreamTimeout(timeoutTime);
  }
  auto reqContext = t2r->getContext();

  try {
    processor_->process(std::move(t2r),
                        std::move(buf),
                        static_cast<apache::thrift::protocol::PROTOCOL_TYPES>
                        (channel_->getHeader()->getProtocolId()),
                        reqContext,
                        worker_->getEventBase(),
                        server->getThreadManager().get());
  } catch (...) {
    LOG(WARNING) << "Process exception: " <<
      folly::exceptionStr(std::current_exception());
    throw;
  }
}

void Cpp2Connection::channelClosed(std::exception_ptr&& ex) {
  // This must be the last call, it may delete this.
  folly::ScopeGuard guard = folly::makeGuard([&]{
    worker_->closeConnection(shared_from_this());
  });

  VLOG(4) << "Channel " <<
    context_.getPeerAddress()->describe() << " closed";
}

void Cpp2Connection::removeRequest(Cpp2Request *req) {
  activeRequests_.erase(req);
  if (activeRequests_.empty()) {
    worker_->scheduleIdleConnectionTimeout(this);
  }
}

Cpp2Connection::Cpp2Request::Cpp2Request(
    std::unique_ptr<ResponseChannel::Request> req,
    std::shared_ptr<Cpp2Connection> con)
  : req_(std::move(req))
  , connection_(con)
  , reqContext_(&con->context_) {
  RequestContext::create();
}

MessageChannel::SendCallback*
Cpp2Connection::Cpp2Request::prepareSendCallback(
    MessageChannel::SendCallback* sendCallback,
    apache::thrift::server::TServerObserver* observer) {
  // If we are sampling this call, wrap it with a Cpp2Sample, which also
  // implements MessageChannel::SendCallback. Callers of sendReply/sendError
  // are responsible for cleaning up their own callbacks.
  MessageChannel::SendCallback* cb = sendCallback;
  if (req_->timestamps_.readBegin != 0) {
    req_->timestamps_.processEnd =
      apache::thrift::concurrency::Util::currentTimeUsec();
    // Cpp2Sample will delete itself when it's callback is called.
    cb = new Cpp2Sample(
      std::move(req_->timestamps_),
      observer,
      sendCallback);
  }
  return cb;
}


void Cpp2Connection::Cpp2Request::sendReply(
    std::unique_ptr<folly::IOBuf>&& buf,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    auto observer = connection_->getWorker()->getServer()->getObserver().get();
    req_->sendReply(
      std::move(buf),
      prepareSendCallback(sendCallback, observer));
    cancelTimeout();
    if (observer) {
      observer->sentReply();
    }
  }
}

void Cpp2Connection::Cpp2Request::sendError(
    std::exception_ptr ex,
    std::string exCode,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    auto observer = connection_->getWorker()->getServer()->getObserver().get();
    req_->sendError(ex, exCode, prepareSendCallback(sendCallback, observer));
    cancelTimeout();
  }
}

void Cpp2Connection::Cpp2Request::sendReplyWithStreams(
    std::unique_ptr<folly::IOBuf>&& data,
    std::unique_ptr<StreamManager>&& streams,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    auto observer = connection_->getWorker()->getServer()->getObserver().get();
    req_->sendReplyWithStreams(
        std::move(data),
        std::move(streams),
        prepareSendCallback(sendCallback, observer));
    cancelTimeout();
    if (observer) {
      observer->sentReply();
    }
  }
}

void Cpp2Connection::Cpp2Request::setStreamTimeout(
    const std::chrono::milliseconds& timeout) {
  if (req_->isActive()) {
    req_->setStreamTimeout(timeout);
  }
}

void Cpp2Connection::Cpp2Request::timeoutExpired() noexcept {
  apache::thrift::TApplicationException x(
      TApplicationException::TApplicationExceptionType::TIMEOUT,
      "Task expired");
  sendError(std::make_exception_ptr(x), kTaskExpiredErrorCode);
  req_->cancel();
  connection_->requestTimeoutExpired();
}

Cpp2Connection::Cpp2Request::~Cpp2Request() {
  connection_->removeRequest(this);
  cancelTimeout();
  connection_->getWorker()->activeRequests_--;
  connection_->getWorker()->getServer()->decGlobalActiveRequests();
}

// Cancel request is usually called from a different thread than sendReply.
void Cpp2Connection::Cpp2Request::cancelRequest() {
  cancelTimeout();
  req_->cancel();
}

Cpp2Connection::Cpp2Sample::Cpp2Sample(
    apache::thrift::server::TServerObserver::CallTimestamps&& timestamps,
    apache::thrift::server::TServerObserver* observer,
    MessageChannel::SendCallback* chainedCallback)
  : timestamps_(timestamps)
  , observer_(observer)
  , chainedCallback_(chainedCallback) {
  DCHECK(observer != nullptr);
}

void Cpp2Connection::Cpp2Sample::sendQueued() {
  if (chainedCallback_ != nullptr) {
    chainedCallback_->sendQueued();
  }
  timestamps_.writeBegin =
    apache::thrift::concurrency::Util::currentTimeUsec();
}

void Cpp2Connection::Cpp2Sample::messageSent() {
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSent();
  }
  timestamps_.writeEnd =
    apache::thrift::concurrency::Util::currentTimeUsec();
  delete this;
}

void Cpp2Connection::Cpp2Sample::messageSendError(std::exception_ptr&& e){
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSendError(std::move(e));
  }
  timestamps_.writeEnd =
    apache::thrift::concurrency::Util::currentTimeUsec();
  delete this;
}

Cpp2Connection::Cpp2Sample::~Cpp2Sample() {
  if (observer_) {
    observer_->callCompleted(timestamps_);
  }
}

}} // apache::thrift
