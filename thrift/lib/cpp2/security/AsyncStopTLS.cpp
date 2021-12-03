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

#include <thrift/lib/cpp2/security/AsyncStopTLS.h>

namespace apache {
namespace thrift {

void AsyncStopTLS::start(fizz::AsyncFizzBase* transport) {
  DestructorGuard guard(this);

  DCHECK(!transport_);
  transport_ = transport;

  transport->setEndOfTLSCallback(this);

  // Attaching a read callback to the transport may synchronously trigger
  // read callbacks to fire.
  transport->setReadCB(this);

  // Because attaching a read callback above may have synchronously
  // transitioned us to a different state, we must explicitly check that we
  // have not invoked our awaiter before starting the tlsShutdown()
  if (awaiter_) {
    transport->tlsShutdown();
  }
}

void AsyncStopTLS::endOfTLS(
    fizz::AsyncFizzBase*, std::unique_ptr<folly::IOBuf> postTLSData) {
  transport_->setReadCB(nullptr);

  auto awaiter = std::exchange(awaiter_, nullptr);
  awaiter->stopTLSSuccess(std::move(postTLSData));
}

// This should never be called because we are using `isBufferMovable` and
// `AsyncFizzBase` guarantees that if the read callback supports movable
// buffers, that it will invoke the readBufferAvailable() API.
void AsyncStopTLS::getReadBuffer(void**, size_t* lenReturn) {
  LOG(DFATAL)
      << "AsyncStopTLS::getReadBuffer is being invoked, when it clearly supports movable buffer API";
  *lenReturn = 0;
}

// This should never be called because we are using `isBufferMovable` and
// `AsyncFizzBase` guarantees that if the read callback supports movable
// buffers, that it will invoke the readBufferAvailable() API.
void AsyncStopTLS::readDataAvailable(size_t) noexcept {
  LOG(DFATAL)
      << "AsyncStopTLS::readDataAvailable is being invoked, when it clearly supports movable buffer API";
}

// The underlying transport closed *before* we received an endOfTLS callback.
//
// This is an error with respect to the StopTLS handshake negotiation.
void AsyncStopTLS::readEOF() noexcept {
  transport_->setReadCB(nullptr);

  auto awaiter = std::exchange(awaiter_, nullptr);
  awaiter->stopTLSError(folly::make_exception_wrapper<std::runtime_error>(
      "stoptls protocol error: readEOF() before negotiation completion"));
}

// The underlying transport is giving us application data *before* we received
// an endOfTLS callback.
//
// This is an error with respect to the StopTLS handshake negotiation.
void AsyncStopTLS::readBufferAvailable(std::unique_ptr<folly::IOBuf>) noexcept {
  transport_->setReadCB(nullptr);

  auto awaiter = std::exchange(awaiter_, nullptr);
  awaiter->stopTLSError(folly::make_exception_wrapper<std::runtime_error>(
      "stoptls protocol error: received unexpected application data, expecting close_notify"));
}

// The underlying transport has closed with an error *before* we received
// an endOfTLS callback.
//
// This is an error with respect ot the StopTLS handshake negotiation.
void AsyncStopTLS::readErr(const folly::AsyncSocketException& ex) noexcept {
  transport_->setReadCB(nullptr);

  auto awaiter = std::exchange(awaiter_, nullptr);
  awaiter->stopTLSError(ex);
}
} // namespace thrift
} // namespace apache
