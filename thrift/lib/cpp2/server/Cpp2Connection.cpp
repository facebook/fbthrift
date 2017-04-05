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

#include <thrift/lib/cpp2/server/Cpp2Connection.h>

#include <thrift/lib/cpp/async/TEventConnection.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/security/SecurityKillSwitch.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>

namespace apache { namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::async;
using namespace std;
using apache::thrift::TApplicationException;

const std::string Cpp2Connection::loadHeader{"load"};

bool Cpp2Connection::isClientLocal(const folly::SocketAddress& clientAddr,
                                   const folly::SocketAddress& serverAddr) {
  if (clientAddr.isLoopbackAddress()) {
    return true;
  }
  if (clientAddr.empty() || serverAddr.empty()) {
    return false;
  }
  return clientAddr.getIPAddress() == serverAddr.getIPAddress();
}

Cpp2Connection::Cpp2Connection(
  const std::shared_ptr<TAsyncSocket>& asyncSocket,
  const folly::SocketAddress* address,
  std::shared_ptr<Cpp2Worker> worker,
  const std::shared_ptr<HeaderServerChannel>& serverChannel)
    : processor_(worker->getServer()->getCpp2Processor())
    , duplexChannel_(worker->getServer()->isDuplex() ?
        std::make_unique<DuplexChannel>(
            DuplexChannel::Who::SERVER, asyncSocket) :
        nullptr)
    , channel_(serverChannel ? serverChannel :  // used by client
               duplexChannel_ ? duplexChannel_->getServerChannel() : // server
               std::shared_ptr<HeaderServerChannel>(
                   new HeaderServerChannel(asyncSocket),
                   folly::DelayedDestruction::Destructor()))
    , worker_(std::move(worker))
    , context_(address,
               asyncSocket.get(),
               channel_->getSaslServer(),
               worker_->getServer()->getEventBaseManager(),
               duplexChannel_ ? duplexChannel_->getClientChannel() : nullptr,
               nullptr,
               worker_->getServer()->getClientIdentityHook())
    , socket_(asyncSocket)
    , threadManager_(worker_->getServer()->getThreadManager()) {

  channel_->setQueueSends(worker_->getServer()->getQueueSends());
  channel_->setMinCompressBytes(worker_->getServer()->getMinCompressBytes());
  channel_->setDefaultWriteTransforms(
    worker_->getServer()->getDefaultWriteTransforms()
  );

  auto observer = worker_->getServer()->getObserver();
  if (observer) {
    channel_->setSampleRate(observer->getSampleRate());
  }

  // Process the security kill switch.
  if (isSecurityKillSwitchEnabled() &&
      worker_->getServer()->getSaslPolicy() == "required") {
    // This means we downgrade only from "required" to "permitted, and don't
    // end up upgrading from "disabled" to "permitted"
    worker_->getServer()->setNonSaslEnabled(true);
  } else {
    // This is necessary for the server to revert back to old behavior once
    // kill switch has been removed after being turned on for some time.
    if (worker_->getServer()->getSaslPolicy() == "permitted") {
      worker_->getServer()->setNonSaslEnabled(true);
    } else {
      // sasl_policy is either "required" or "disabled"
      worker_->getServer()->setNonSaslEnabled(false);
    }
  }

  if (asyncSocket) {
    auto factory = worker_->getServer()->getSaslServerFactory();
    if (factory) {
      channel_->setSaslServer(
        unique_ptr<SaslServer>(factory(asyncSocket->getEventBase()))
      );
      // Refresh the saslServer_ pointer in context_
      context_.setSaslServer(channel_->getSaslServer());
    }
  }

  const bool downgradeSaslPolicy =
    worker_->getServer()->getAllowInsecureLoopback() &&
    isClientLocal(*address, *context_.getLocalAddress());

  if (worker_->getServer()->getSaslEnabled() &&
      (worker_->getServer()->getNonSaslEnabled() || downgradeSaslPolicy)) {
    channel_->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
  } else if (worker_->getServer()->getSaslEnabled()) {
    channel_->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
  } else {
    // If neither is set, act as if non-sasl was specified.
    channel_->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
  }

  auto handler = worker_->getServer()->getEventHandler();
  if (handler) {
    handler->newConnection(&context_);
  }
}

Cpp2Connection::~Cpp2Connection() {
  auto handler = worker_->getServer()->getEventHandler();
  if (handler) {
    handler->connectionDestroyed(&context_);
  }

  channel_.reset();
}

void Cpp2Connection::stop() {
  if (getConnectionManager()) {
    getConnectionManager()->removeConnection(this);
  }

  for (auto req : activeRequests_) {
    VLOG(1) << "Task killed due to channel close: " <<
      context_.getPeerAddress()->describe();
    req->cancelRequest();
    auto observer = worker_->getServer()->getObserver();
    if (observer) {
      observer->taskKilled();
    }
  }

  if (channel_) {
    channel_->setCallback(nullptr);

    // Release the socket to avoid long CLOSE_WAIT times
    channel_->closeNow();
  }

  socket_.reset();

  this_.reset();
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
    stop();
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
    observer->taskTimeout();
  }
}

void Cpp2Connection::queueTimeoutExpired() {
  VLOG(1) << "ERROR: Queue timeout on channel: " <<
    context_.getPeerAddress()->describe();
  auto observer = worker_->getServer()->getObserver();
  if (observer) {
    observer->queueTimeout();
  }
}

bool Cpp2Connection::pending() {
  return socket_ ? socket_->isPending() : false;
}

void Cpp2Connection::killRequest(
    ResponseChannel::Request& req,
    TApplicationException::TApplicationExceptionType reason,
    const std::string& errorCode,
    const char* comment) {
  VLOG(1) << "ERROR: Task killed: " << comment
          << ": " << context_.getPeerAddress()->getAddressStr();

  auto server = worker_->getServer();
  auto observer = server->getObserver();
  if (observer) {
    if (reason ==
         TApplicationException::TApplicationExceptionType::LOADSHEDDING) {
      observer->serverOverloaded();
    } else {
      observer->taskKilled();
    }
  }

  // Nothing to do for Thrift oneway request.
  if (req.isOneway()) {
    return;
  }

  auto header_req = static_cast<HeaderServerChannel::HeaderRequest*>(&req);

  // Thrift1 oneway request doesn't use ONEWAY_REQUEST_ID and
  // may end up here. No need to send error back for such requests
  if (!processor_->isOnewayMethod(req.getBuf(), header_req->getHeader())) {
    header_req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(reason,
                                                             comment),
        errorCode,
        nullptr);
  } else {
    // Send an empty response so reqId will be handled properly
    req.sendReply(std::unique_ptr<folly::IOBuf>());
  }
}

// Response Channel callbacks
void Cpp2Connection::requestReceived(
  unique_ptr<ResponseChannel::Request>&& req) {
  auto reqCtx = std::make_shared<folly::RequestContext>();
  auto handler = worker_->getServer()->getEventHandler();
  if (handler) {
    handler->connectionNewRequest(&context_, reqCtx.get());
  }
  folly::RequestContextScopeGuard rctx(reqCtx);

  auto server = worker_->getServer();
  auto observer = server->getObserver();

  server->touchRequestTimestamp();

  auto injectedFailure = server->maybeInjectFailure();
  switch (injectedFailure) {
  case ThriftServer::InjectedFailure::NONE:
    break;
  case ThriftServer::InjectedFailure::ERROR:
    killRequest(*req,
        TApplicationException::TApplicationExceptionType::INJECTED_FAILURE,
        kInjectedFailureErrorCode,
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

  auto* hreq = static_cast<HeaderServerChannel::HeaderRequest*>(req.get());
  bool useHttpHandler = false;
  // Any POST not for / should go to the status handler
  if (hreq->getHeader()->getClientType() == THRIFT_HTTP_SERVER_TYPE) {
    auto buf = req->getBuf();
    // 7 == length of "POST / " - we are matching on the path
    if (buf->length() >= 7 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()),
                     "POST",
                     4) &&
        buf->data()[6] != ' ') {
      useHttpHandler = true;
    }

    // Any GET should use the handler
    if (buf->length() >= 3 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()),
                     "GET",
                     3)) {
      useHttpHandler = true;
    }

    // Any HEAD should use the handler
    if (buf->length() >= 4 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()),
                     "HEAD",
                     4)) {
      useHttpHandler = true;
    }
  }

  if (useHttpHandler && worker_->getServer()->getGetHandler()) {
    worker_->getServer()->getGetHandler()(
        worker_->getEventBase(), socket_, req->extractBuf());

    // Close the channel, since the handler now owns the socket.
    channel_->setCallback(nullptr);
    channel_->setTransport(nullptr);
    stop();
    return;
  }

  if (worker_->getServer()->getGetHeaderHandler()) {
    worker_->getServer()->getGetHeaderHandler()(
        hreq->getHeader(), context_.getPeerAddress());
  }

  if (server->getTrackPendingIO()) {
    worker_->computePendingCount();
  }
  if (server->isOverloaded(hreq->getHeader())) {
    killRequest(*req,
        TApplicationException::TApplicationExceptionType::LOADSHEDDING,
        kOverloadedErrorCode,
        "loadshedding request");
    return;
  }

  server->incActiveRequests();
  if (req->timestamps_.readBegin != 0) {
    // Expensive operations; this happens once every
    // TServerCounters.sampleRate
    req->timestamps_.processBegin =
      apache::thrift::concurrency::Util::currentTimeUsec();
    if (observer) {
      observer->queuedRequests(threadManager_->pendingTaskCount());
      observer->activeRequests(
        server->getActiveRequests() +
        server->getPendingCount());
    }
  }

  // After this, the request buffer is no longer owned by the request
  // and will be released after deserializeRequest.
  unique_ptr<folly::IOBuf> buf = hreq->extractBuf();

  Cpp2Request* t2r = new Cpp2Request(std::move(req), this_);
  auto up2r = std::unique_ptr<ResponseChannel::Request>(t2r);
  activeRequests_.insert(t2r);
  ++worker_->activeRequests_;

  if (observer) {
    observer->receivedRequest();
  }

  std::chrono::milliseconds queueTimeout;
  std::chrono::milliseconds taskTimeout;
  auto differentTimeouts = server->getTaskExpireTimeForRequest(
    *(hreq->getHeader()),
    queueTimeout,
    taskTimeout
  );
  if (differentTimeouts) {
    if (queueTimeout > std::chrono::milliseconds(0)) {
      scheduleTimeout(&t2r->queueTimeout_, queueTimeout);
    }
  }
  if (taskTimeout > std::chrono::milliseconds(0)) {
    scheduleTimeout(&t2r->taskTimeout_, taskTimeout);
  }

  auto reqContext = t2r->getContext();
  reqContext->setRequestTimeout(taskTimeout);

  try {
    auto protoId = static_cast<apache::thrift::protocol::PROTOCOL_TYPES>
      (hreq->getHeader()->getProtocolId());

    if (!apache::thrift::detail::ap::deserializeMessageBegin(
          protoId,
          up2r,
          buf.get(),
          reqContext,
          worker_->getEventBase())) {
      return;
    }

    processor_->process(std::move(up2r),
                        std::move(buf),
                        protoId,
                        reqContext,
                        worker_->getEventBase(),
                        threadManager_.get());
  } catch (...) {
    LOG(WARNING) << "Process exception: " <<
      folly::exceptionStr(std::current_exception());
    throw;
  }
}

void Cpp2Connection::channelClosed(folly::exception_wrapper&& ex) {
  // This must be the last call, it may delete this.
  folly::ScopeGuard guard = folly::makeGuard([&]{
    stop();
  });

  VLOG(4) << "Channel " <<
    context_.getPeerAddress()->describe() << " closed: " << ex.what();
}

void Cpp2Connection::removeRequest(Cpp2Request *req) {
  activeRequests_.erase(req);
  if (activeRequests_.empty()) {
    resetTimeout();
  }
}

Cpp2Connection::Cpp2Request::Cpp2Request(
    std::unique_ptr<ResponseChannel::Request> req,
    std::shared_ptr<Cpp2Connection> con)
  : req_(static_cast<HeaderServerChannel::HeaderRequest*>(req.release()))
  , connection_(con)
  , reqContext_(&con->context_, req_->getHeader()) {
  queueTimeout_.request_ = this;
  taskTimeout_.request_ = this;

  const auto& headers = req_->getHeader()->getHeaders();
  auto it = headers.find(Cpp2Connection::loadHeader);
  if (it != headers.end()) {
    loadHeader_ = it->second;
  }
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

void Cpp2Connection::Cpp2Request::setLoadHeader() {
  // Set load header, based on the received load header
  if (!loadHeader_) {
    return;
  }

  auto load =
      connection_->getWorker()->getServer()->getLoad(*loadHeader_);
  req_->getHeader()->setHeader(Cpp2Connection::loadHeader,
                               folly::to<std::string>(load));
}

void Cpp2Connection::Cpp2Request::sendReply(
    std::unique_ptr<folly::IOBuf>&& buf,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    setLoadHeader();
    auto observer = connection_->getWorker()->getServer()->getObserver().get();
    auto maxResponseSize =
      connection_->getWorker()->getServer()->getMaxResponseSize();
    if (maxResponseSize != 0 &&
        buf->computeChainDataLength() > maxResponseSize) {
      req_->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::INTERNAL_ERROR,
            "Response size too big"),
          kResponseTooBigErrorCode,
          reqContext_.getMethodName(),
          reqContext_.getProtoSeqId(),
          prepareSendCallback(sendCallback, observer));
    } else {
      req_->sendReply(
          std::move(buf),
          prepareSendCallback(sendCallback, observer));
    }
    cancelTimeout();
    if (observer) {
      observer->sentReply();
    }
  }
}

void Cpp2Connection::Cpp2Request::sendErrorWrapped(
    folly::exception_wrapper ew,
    std::string exCode,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    setLoadHeader();
    auto observer = connection_->getWorker()->getServer()->getObserver().get();
    req_->sendErrorWrapped(std::move(ew),
                           std::move(exCode),
                           reqContext_.getMethodName(),
                           reqContext_.getProtoSeqId(),
                           prepareSendCallback(sendCallback, observer));
    cancelTimeout();
  }
}

void Cpp2Connection::Cpp2Request::sendTimeoutResponse(
  HeaderServerChannel::HeaderRequest::TimeoutResponseType responseType) {
  auto observer = connection_->getWorker()->getServer()->getObserver().get();
  std::map<std::string, std::string> headers;
  if (loadHeader_) {
    auto load =
      connection_->getWorker()->getServer()->getLoad(*loadHeader_);
    headers[Cpp2Connection::loadHeader] = folly::to<std::string>(load);
  }
  req_->sendTimeoutResponse(reqContext_.getMethodName(),
                            reqContext_.getProtoSeqId(),
                            prepareSendCallback(nullptr, observer),
                            headers,
                            responseType);
  cancelTimeout();
}

void Cpp2Connection::Cpp2Request::TaskTimeout::timeoutExpired() noexcept {
  request_->req_->cancel();
  request_->sendTimeoutResponse(
    HeaderServerChannel::HeaderRequest::TimeoutResponseType::TASK);
  request_->connection_->requestTimeoutExpired();
}

void Cpp2Connection::Cpp2Request::QueueTimeout::timeoutExpired() noexcept {
  if (!request_->reqContext_.getStartedProcessing()) {
    request_->req_->cancel();
    request_->sendTimeoutResponse(
      HeaderServerChannel::HeaderRequest::TimeoutResponseType::QUEUE);
    request_->connection_->queueTimeoutExpired();
  }
}

Cpp2Connection::Cpp2Request::~Cpp2Request() {
  connection_->removeRequest(this);
  cancelTimeout();
  connection_->getWorker()->activeRequests_--;
  connection_->getWorker()->getServer()->decActiveRequests();
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

void Cpp2Connection::Cpp2Sample::messageSendError(
    folly::exception_wrapper&& e) {
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
