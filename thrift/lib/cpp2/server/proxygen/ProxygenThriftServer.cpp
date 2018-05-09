/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp2/server/proxygen/ProxygenThriftServer.h>

#include <fcntl.h>
#include <signal.h>

#include <iostream>
#include <random>

#include <folly/Conv.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/portability/Sockets.h>
#include <glog/logging.h>
#include <proxygen/httpserver/ResponseBuilder.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/Cpp2Connection.h>

namespace apache {
namespace thrift {

using apache::thrift::concurrency::Runnable;
using folly::SocketAddress;
using proxygen::HTTPMessage;
using proxygen::HTTPServer;
using proxygen::HTTPServerOptions;
using proxygen::ProxygenError;
using proxygen::RequestHandlerChain;
using proxygen::ResponseBuilder;
using proxygen::ResponseHandler;
using proxygen::UpgradeProtocol;

void ProxygenThriftServer::ThriftRequestHandler::onRequest(
    std::unique_ptr<HTTPMessage> msg) noexcept {
  msg_ = std::move(msg);

  header_->setSequenceNumber(msg_->getSeqNo());

  apache::thrift::THeader::StringToStringMap thriftHeaders;
  msg_->getHeaders().forEach(
      [&](const std::string& key, const std::string& val) {
        thriftHeaders[key] = val;
      });

  header_->setReadHeaders(std::move(thriftHeaders));
}

void ProxygenThriftServer::ThriftRequestHandler::onBody(
    std::unique_ptr<folly::IOBuf> body) noexcept {
  if (body_) {
    body_->prependChain(std::move(body));
  } else {
    body_ = std::move(body);

    // This is a good time to detect the protocol, since if the header isn't
    // there we can sniff the body.
    auto& headers = msg_->getHeaders();
    auto proto = headers.getSingleOrEmpty(
        proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL);
    if (proto == "binary") {
      header_->setProtocolId(protocol::T_BINARY_PROTOCOL);
    } else if (proto == "compact") {
      header_->setProtocolId(protocol::T_COMPACT_PROTOCOL);
    } else if (proto == "json") {
      header_->setProtocolId(protocol::T_JSON_PROTOCOL);
    } else if (proto == "simplejson") {
      header_->setProtocolId(protocol::T_SIMPLE_JSON_PROTOCOL);
    } else {
      if (body_->length() == 0) {
        LOG(ERROR) << "Do not have a magic byte to sniff for the protocol";
        return;
      }

      auto protByte = body_->data()[0];
      switch (protByte) {
        case 0x80:
          header_->setProtocolId(protocol::T_BINARY_PROTOCOL);
          break;
        case 0x82:
          header_->setProtocolId(protocol::T_COMPACT_PROTOCOL);
          break;
        default:
          LOG(ERROR) << "Unable to determine protocol from magic byte "
                     << (int)protByte;
      }
    }
  }
}

void ProxygenThriftServer::ThriftRequestHandler::onEOM() noexcept {
  folly::RequestContext::create();
  connCtx_ = std::make_shared<apache::thrift::Cpp2ConnContext>(
      &msg_->getClientAddress(), nullptr, nullptr, nullptr, nullptr);

  buf_ = body_->clone(); // for apache::thrift::ResponseChannel::Request

  // Set up timeouts
  std::chrono::milliseconds queueTimeout;
  std::chrono::milliseconds taskTimeout;
  bool differentTimeouts = worker_->server_->getTaskExpireTimeForRequest(
      *header_, queueTimeout, taskTimeout);
  if (differentTimeouts) {
    if (queueTimeout > std::chrono::milliseconds(0)) {
      timer_->scheduleTimeout(&queueTimeout_, queueTimeout);
    }
  }
  if (taskTimeout > std::chrono::milliseconds(0)) {
    timer_->scheduleTimeout(&taskTimeout_, taskTimeout);
  }

  worker_->server_->incActiveRequests();

  reqCtx_ = std::make_shared<apache::thrift::Cpp2RequestContext>(
      connCtx_.get(), header_.get());

  request_ = new ProxygenRequest(this, header_, connCtx_, reqCtx_);
  auto req =
      std::unique_ptr<apache::thrift::ResponseChannel::Request>(request_);
  auto protoId = static_cast<apache::thrift::protocol::PROTOCOL_TYPES>(
      header_->getProtocolId());
  if (!apache::thrift::detail::ap::deserializeMessageBegin(
          protoId, req, body_.get(), reqCtx_.get(), worker_->evb_)) {
    return;
  }

  worker_->getProcessor()->process(
      std::move(req),
      std::move(body_),
      protoId,
      reqCtx_.get(),
      worker_->evb_,
      threadManager_);
}

void ProxygenThriftServer::ThriftRequestHandler::onUpgrade(
    UpgradeProtocol /*protocol*/) noexcept {
  LOG(WARNING) << "ProxygenThriftServer does not support upgrade requests";
}

void ProxygenThriftServer::ThriftRequestHandler::requestComplete() noexcept {
  if (cb_) {
    cb_->messageSent();
  }

  delete this;
}

void ProxygenThriftServer::ThriftRequestHandler::onError(
    ProxygenError err) noexcept {
  LOG(ERROR) << "Proxygen error=" << err;

  // TODO(ckwalsh) Expose proxygen errors as a counter somewhere
  delete this;
}

void ProxygenThriftServer::ThriftRequestHandler::sendReply(
    std::unique_ptr<folly::IOBuf>&& buf, // && from ResponseChannel.h
    apache::thrift::MessageChannel::SendCallback* cb) {
  queueTimeout_.cancelTimeout();
  taskTimeout_.cancelTimeout();

  if (request_) {
    request_->clearHandler();
    request_ = nullptr;
  }

  worker_->server_->decActiveRequests();

  auto response = ResponseBuilder(downstream_);

  response.status(200, "OK");

  for (auto itr : header_->releaseWriteHeaders()) {
    response.header(itr.first, itr.second);
  }

  response.header(
      proxygen::HTTPHeaderCode::HTTP_HEADER_CONTENT_TYPE,
      "application/x-thrift");

  switch (header_->getProtocolId()) {
    case protocol::T_BINARY_PROTOCOL:
      response.header(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "binary");
      break;
    case protocol::T_COMPACT_PROTOCOL:
      response.header(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "compact");
      break;
    case protocol::T_JSON_PROTOCOL:
      response.header(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL, "json");
      break;
    case protocol::T_SIMPLE_JSON_PROTOCOL:
      response.header(
          proxygen::HTTPHeaderCode::HTTP_HEADER_X_THRIFT_PROTOCOL,
          "simplejson");
      break;
    default:
      // Do nothing
      break;
  }

  auto& headers = msg_->getHeaders();

  if (headers.exists(Cpp2Connection::loadHeader)) {
    auto& loadHeader = headers.getSingleOrEmpty(Cpp2Connection::loadHeader);
    auto load = worker_->server_->getLoad(loadHeader);
    response.header(Cpp2Connection::loadHeader, folly::to<std::string>(load));
  }

  response.body(std::move(buf)).sendWithEOM();

  if (cb) {
    cb->sendQueued();
    cb_ = cb;
  }
}

void ProxygenThriftServer::ThriftRequestHandler::sendErrorWrapped(
    folly::exception_wrapper ew,
    std::string exCode,
    apache::thrift::MessageChannel::MessageChannel::SendCallback* cb) {
  // Other types are unimplemented.
  DCHECK(ew.is_compatible_with<apache::thrift::TApplicationException>());

  header_->setHeader("ex", exCode);

  ew.with_exception<apache::thrift::TApplicationException>(
      [&](apache::thrift::TApplicationException& tae) {
        std::unique_ptr<folly::IOBuf> exbuf;
        auto proto = header_->getProtocolId();
        try {
          exbuf = serializeError(proto, tae, getBuf());
        } catch (const apache::thrift::protocol::TProtocolException& pe) {
          LOG(ERROR) << "serializeError failed. type=" << pe.getType()
                     << " what()=" << pe.what();
          if (request_) {
            request_->clearHandler();
            request_ = nullptr;
          }
          worker_->server_->decActiveRequests();
          ResponseBuilder(downstream_)
              .status(500, "Internal Server Error")
              .closeConnection()
              .sendWithEOM();
          return;
        }

        sendReply(std::move(exbuf), cb);
      });
}

void ProxygenThriftServer::ThriftRequestHandler::TaskTimeout::
    timeoutExpired() noexcept {
  if (hard_ || !request_->reqCtx_->getStartedProcessing()) {
    request_->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(
            TApplicationException::TApplicationExceptionType::TIMEOUT,
            "Task expired"),
        kTaskExpiredErrorCode);
    request_->cancel();

    VLOG(1) << "ERROR: Task expired on channel: "
            << request_->msg_->getClientAddress().describe();

    auto observer = request_->worker_->server_->getObserver();
    if (observer) {
      observer->taskTimeout();
    }
  }
}

bool ProxygenThriftServer::isOverloaded(const THeader* header) {
  if (UNLIKELY(isOverloaded_(header))) {
    return true;
  }

  LOG(WARNING) << "isOverloaded is not implemented";

  return false;
}

uint64_t ProxygenThriftServer::getNumDroppedConnections() const {
  uint64_t droppedConns = 0;
  if (server_) {
    for (auto socket : server_->getSockets()) {
      auto serverSock = dynamic_cast<const folly::AsyncServerSocket*>(socket);
      if (serverSock) {
        droppedConns += serverSock->getNumDroppedConnections();
      }
    }
  }
  return droppedConns;
}

void ProxygenThriftServer::serve() {
  if (!observer_ && apache::thrift::observerFactory_) {
    observer_ = apache::thrift::observerFactory_->getObserver();
  }

  if (!threadManager_) {
    int numThreads = nPoolThreads_ > 0 ? nPoolThreads_ : nWorkers_;
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager(
        PriorityThreadManager::newPriorityThreadManager(
            numThreads,
            true /*stats*/,
            getMaxRequests() + numThreads /*maxQueueLen*/));
    threadManager->enableCodel(getEnableCodel());
    if (!poolThreadName_.empty()) {
      threadManager->setNamePrefix(poolThreadName_);
    }
    threadManager->start();
    setThreadManager(threadManager);
  }
  threadManager_->setExpireCallback([&](std::shared_ptr<Runnable> r) {
    EventTask* task = dynamic_cast<EventTask*>(r.get());
    if (task) {
      task->expired();
    }
  });
  threadManager_->setCodelCallback([&](std::shared_ptr<Runnable> /*r*/) {
    auto observer = getObserver();
    if (observer) {
      observer->queueTimeout();
    }
  });

  std::vector<HTTPServer::IPConfig> IPs;
  if (port_ != -1) {
    IPs.emplace_back(SocketAddress("::", port_, true), httpProtocol_);
  } else {
    IPs.emplace_back(address_, httpProtocol_);
  }

  HTTPServerOptions options;
  options.threads = static_cast<size_t>(nWorkers_);
  options.idleTimeout = timeout_;
  options.initialReceiveWindow = initialReceiveWindow_;
  options.shutdownOn = {SIGINT, SIGTERM};
  options.handlerFactories =
      RequestHandlerChain().addThen<ThriftRequestHandlerFactory>(this).build();

  configMutable_ = false;

  server_ = std::make_unique<HTTPServer>(std::move(options));
  server_->bind(IPs);
  server_->setSessionInfoCallback(this);

  server_->start([&]() {
    // Notify handler of the preServe event
    if (this->eventHandler_ != nullptr) {
      auto addrs = this->server_->addresses();
      if (!addrs.empty()) {
        this->eventHandler_->preServe(&addrs[0].address);
      }
    }
  });

  // Server stopped for some reason
  server_.reset();
}

void ProxygenThriftServer::stop() {
  server_->stop();
}

void ProxygenThriftServer::stopListening() {
  server_->stop();
}
} // namespace thrift
} // namespace apache
