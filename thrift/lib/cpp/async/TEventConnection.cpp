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

#include <thrift/lib/cpp/async/TEventConnection.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TEventWorker.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/async/TBinaryAsyncChannel.h>
#include <thrift/lib/cpp/async/TQueuingAsyncProcessor.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>

#include <assert.h>

namespace {
const uint32_t kMaxReadsPerLoop = 32;
}

namespace apache { namespace thrift { namespace async {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace std;
using namespace std::placeholders;
using namespace folly;


  // Constructor
TEventConnection::TEventConnection(shared_ptr<TAsyncSocket> asyncSocket,
                                   const SocketAddress* address,
                                   TEventWorker* worker,
                                   TEventServer::TransportType transport)
  : worker_(nullptr),
    asyncSocket_(asyncSocket),
    largestReadBufferSize_(0),
    largestWriteBufferSize_(0),
    callsForResize_(0),
    asyncChannel_(nullptr),
    processorActive_(0),
    readersActive_(0),
    shutdown_(false),
    closing_(false) {
  // Allocate input and output transports (they last the lifetime of the
  // connection and are reused between requests)
  inputTransport_.reset(new TMemoryBuffer());
  outputTransport_.reset(
    new TMemoryBuffer(worker->getServer()->getWriteBufferDefaultSize()));

  T_DEBUG_T("fd=%d; TEventConnection: constructed new connection.",
            asyncSocket->getFd());

  init(asyncSocket, address, worker, transport);
}

TEventConnection::~TEventConnection() {
  if (asyncChannel_) {
    asyncChannel_->destroy();
  }
}

void TEventConnection::init(shared_ptr<TAsyncSocket> asyncSocket,
                            const SocketAddress* address,
                            TEventWorker* worker,
                            TEventServer::TransportType transport) {
  T_DEBUG_T("fd=%d; TEventConnection::init()", asyncSocket->getFd());

  asyncSocket_ = asyncSocket;
  worker_ = worker;
  TEventServer* server = worker_->getServer();

  inputTransport_->resetBuffer();
  outputTransport_->resetBuffer();

  // Create protocol
  shared_ptr<TDuplexProtocolFactory> protFactory =
    worker->getDuplexProtocolFactory();
  TTransportPair transports(inputTransport_, outputTransport_);
  TProtocolPair protocols = protFactory->getProtocol(transports);
  inputProtocol_ = protocols.first;
  outputProtocol_ = protocols.second;
  context_.init(address, inputProtocol_, outputProtocol_);
  // If we're using a non-async processor, construct a task queue adaptor
  if (server->queuingMode()) {
    processor_ = server->getProcessor(&context_);
    asyncProcessor_.reset(
      new TQueuingAsyncProcessor(processor_,
                                 server->getThreadManager(),
                                 server->getTaskExpireTime().count(),
                                 this));
  } else {
    asyncProcessor_ = worker_->getAsyncProcessorFactory()->getProcessor(
      &context_);
  }

  // create async socket transport and async channel
  assert(asyncChannel_ == nullptr);
  switch (transport) {
    case TEventServer::TransportType::FRAMED:
    {
      TFramedAsyncChannel* framedAsyncChannel(
        new TFramedAsyncChannel(asyncSocket_));
      framedAsyncChannel->setMaxFrameSize(server->getMaxFrameSize());
      asyncChannel_ = framedAsyncChannel;
      break;
    }
    case TEventServer::TransportType::HEADER:
    {
      THeaderAsyncChannel* headerAsyncChannel(
        new THeaderAsyncChannel(asyncSocket_));
      headerAsyncChannel->setMaxMessageSize(server->getMaxFrameSize());
      asyncChannel_ = headerAsyncChannel;
      break;
    }
    case TEventServer::TransportType::UNFRAMED_BINARY:
    {
      TBinaryAsyncChannel *binaryAsyncChannel =
        new TBinaryAsyncChannel(asyncSocket);
      // minor abuse of 'frame' size to set the maximum message length
      binaryAsyncChannel->setMaxMessageSize(server->getMaxFrameSize());
      asyncChannel_ = binaryAsyncChannel;
      break;
    }
    default:
      assert(false);
  }
  asyncChannel_->setRecvTimeout(worker_->getServer()->getRecvTimeout().count());

  serverEventHandler_ = server->getEventHandler();
  if (serverEventHandler_ != nullptr) {
    serverEventHandler_->newConnection(&context_);
  }

  processorActive_ = 0;
  shutdown_ = false;
  closing_ = false;
}

void TEventConnection::start() {
  readNextRequest();
}

bool TEventConnection::notifyCompletion(TaskCompletionMessage &&msg) {
  return worker_->notifyCompletion(std::move(msg));
}

void TEventConnection::runLoopCallback() noexcept {
  this->readNextRequest();
}

void TEventConnection::readNextRequest() {
  T_DEBUG_T("fd=%d; TEventConnection::readNextRequest()",
            asyncSocket_->getFd());
  if (closing_) {
    T_DEBUG_T("fd=%d; TEventConnection::readNextRequest is CLOSING",
              asyncSocket_->getFd());
    return;
  }
  // If we're being shut down we stop here, right before the next read
  if (shutdown_) {
    cleanup();
    return;
  }

  if (readersActive_ >= kMaxReadsPerLoop) {
    worker_->getEventBase()->runInLoop(this);
    return;
  }
  // See if we're due for a buffer size check
  if (worker_->getServer()->getResizeBufferEveryN() > 0 &&
      ++callsForResize_ > worker_->getServer()->getResizeBufferEveryN()) {
    checkBufferMemoryUsage();
    callsForResize_ = 1;
  }

  // Set up the input transport buffer for getting 4 bytes
  inputTransport_->resetBuffer();
  T_DEBUG_T("fd=%d; TEventConnection::readNextRequest() calling recvMessage()",
            asyncSocket_->getFd());
  asyncChannel_->recvMessage(
      std::bind(&TEventConnection::handleReadSuccess, this),
      std::bind(&TEventConnection::handleReadFailure, this),
      inputTransport_.get());
  // handleReadSuccess() or handleReadFailure() will be invoked
  // when the read is finished.  It may have already been invoked before
  // recvMessage() returned, in which case the TEventConnection may have
  // already been destroyed.  Do not access any member variables after this
  // point.
}

void TEventConnection::handleReadSuccess() {
  T_DEBUG_T("fd=%d; TEventConnection::handleReadSuccess()",
            asyncSocket_->getFd());

  // handleReadFailure() should have been invoked on error.
  assert(!asyncChannel_->error());

  // EOF is treated as success, so check for it now.
  if (!asyncChannel_->good()) {
    T_DEBUG_T("fd=%d; handleReadSuccess() sees EOF", asyncSocket_->getFd());
    handleEOF();
    return;
  }

  T_DEBUG_T("fd=%d; handleReadSuccess(): read frame of size %d",
            asyncSocket_->getFd(), inputTransport_->available_read());
  if (inputTransport_->available_read() > largestReadBufferSize_) {
    largestReadBufferSize_ = inputTransport_->available_read();
  }

  TEventServer* server = worker_->getServer();
  ++processorActive_;
  ++readersActive_;
  server->setCurrentConnection(this);
  RequestContext::create();
  try {
    // Invoke the processor
    T_DEBUG_T("fd=%d; handleReadSuccess(): calling asyncProcessor",
              asyncSocket_->getFd());
    asyncProcessor_->process(
        std::bind(&TEventConnection::handleAsyncTaskComplete, this,
                       std::placeholders::_1),
        inputProtocol_, outputProtocol_, this->getConnectionContext());

    // Clear the current connection context after process() returns.
    //
    // Note: process() is an asynchronous call that will call
    // handleAsyncTaskComplete() when it is done.  It is generally not safe to
    // perform any other operations after calling an async function, since it
    // may have invoked the callback before it returns.  However, in our case
    // it should still be safe to access the TEventServer after process()
    // returns.  The TEventServer cannot be destroyed inside the call to
    // process() or the service's handler.
    server->clearCurrentConnection();
  } catch (TException &x) {
    server->clearCurrentConnection();
    --processorActive_;
    --readersActive_;
    handleFailure("process() failed (TException)");
    return;
  }
  // Note: process() may have invoked the callback before returning.
  // The TEventConnection may have been destroyed already, so we should
  // not attempt to access any member variables after this point.
  --readersActive_;
  if (closing_ && readersActive_ <= 0) {
    return cleanup();
  }
}

void TEventConnection::handleReadFailure() {
  handleFailure("read failure");
}

void TEventConnection::handleAsyncTaskComplete(bool success) {
  T_DEBUG_T("fd=%d; TEventConnection::handleAsyncTaskComplete(%d) active=%d",
            asyncSocket_->getFd(), success, processorActive_);
  --processorActive_;

  // The success parameter was previously ignored in this function.
  // The generated code and the dispatcher can pass false on parsing
  // errors.  Close the connection.
  if (!success) {
    // clearCurrentConnection gets called when the stack unwinds.
    handleFailure("process() failed (success=false)");
    return;
  } else if (closing_ && processorActive_ <= 0) {
    return cleanup();
  }

  // Because unframed channels can read-ahead, it's possible to end up
  // in handleReadSuccess again before we return from process and clear the
  // current connection.  Technically we were still processing the same
  // connection, but it triggered an assert failure in setCurrentConnection.
  worker_->getServer()->clearCurrentConnection();

  // figure out how big our response is
  int32_t responseBytes = outputTransport_->available_read();
  if (responseBytes == 0) {
    // if we have no response to send, start again -- let's see the new
    // request!
    readNextRequest();
    return;
  }
  if (responseBytes > largestWriteBufferSize_) {
    largestWriteBufferSize_ = responseBytes;
  }

  asyncChannel_->sendMessage(
      std::bind(&TEventConnection::handleSendSuccess, this),
      std::bind(&TEventConnection::handleSendFailure, this),
      outputTransport_.get());
  // sendMessage() may invoke the callback before returning,
  // so don't access any member variables after this point.
  // The TEventConnection may have already been destroyed.
}

void TEventConnection::handleSendSuccess() {
  T_DEBUG_T("fd=%d; TEventConnection::handleSendSuccess()",
            asyncSocket_->getFd());

  // we've finished writing -- clean up
  outputTransport_->writeEnd();
  outputTransport_->resetBuffer();

  // read the next request!
  readNextRequest();
}

void TEventConnection::handleSendFailure() {
  handleFailure("sendMessage() failed");
}

void TEventConnection::handleEOF() {
  cleanup();
}

void TEventConnection::handleFailure(const char* msg) {
  T_DEBUG_T("worker=%d fd=%d; TEventConnection::handleFailure(): %s",
            worker_->getID(), asyncSocket_->getFd(), msg);
  cleanup();
}

/**
 * Close the connection, and return it to the TEventWorker.
 *
 * The TEventWorker may retain this connection, and call init() on it later
 * with a new socket.
 */
void TEventConnection::cleanup() {
  T_DEBUG_T("fd=%d; TEventConnection::cleanup(), active=%d",
            asyncSocket_->getFd(), processorActive_);
  if (processorActive_ > 0 || readersActive_ > 0) {
    closing_ = true;
    return;
  }
  closing_ = false;

  if (serverEventHandler_ != nullptr) {
    serverEventHandler_->connectionDestroyed(&context_);
  }

  // Cancel callbacks first, so cleanup isn't re-entered
  // while closing the channel
  asyncChannel_->cancelCallbacks();
  asyncChannel_->getTransport()->close();
  asyncChannel_->destroy();
  asyncChannel_ = nullptr;

  // Reset context_
  context_.reset();

  // Give this object back to the worker that owns it
  worker_->returnConnection(this);
}

/**
 * Check our buffer memory usage and reset if necessary.
 */
void TEventConnection::checkBufferMemoryUsage() {
  const TEventServer* server = worker_->getServer();
  if (server->getIdleReadBufferLimit() != 0 &&
      largestReadBufferSize_ > server->getIdleReadBufferLimit()) {
    inputTransport_->resetBuffer(server->getReadBufferDefaultSize());
    T_DEBUG_T("TEventConnection: read buffer reset from %zd to %zd",
              largestReadBufferSize_, server->getReadBufferDefaultSize());
    largestReadBufferSize_ = 0;
  }
  if (server->getIdleWriteBufferLimit() != 0 &&
      largestWriteBufferSize_ > server->getIdleWriteBufferLimit()) {
    outputTransport_->resetBuffer(server->getWriteBufferDefaultSize());
    T_DEBUG_T("TEventConnection: write buffer reset from %zd to %zd",
              largestWriteBufferSize_, server->getWriteBufferDefaultSize());
    largestWriteBufferSize_ = 0;
  }
}

}}} // apache::thrift
