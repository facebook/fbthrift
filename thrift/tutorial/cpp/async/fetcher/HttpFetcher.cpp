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
#include <thrift/tutorial/cpp/async/fetcher/HttpFetcher.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>

#include <thrift/tutorial/cpp/async/fetcher/gen-cpp2/fetcher_types.h>

using namespace std;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::transport::TTransportException;

namespace apache { namespace thrift { namespace tutorial { namespace fetcher {

/**
 * Start the fetch operation.
 *
 * Returns immediately.  When the operation completes, the cob parameter
 * passed in to the constructor will be invoked.  If the operation fails,
 * the error_cob will be invoked instead.
 */
void HttpFetcher::fetch() {
  // Create the socket
  socket_ = TAsyncSocket::newSocket(eventBase_, addr_, port_);
  if (!socket_->good()) {
    fail("error creating socket");
    return;
  }

  // Create the message to send to the server.
  // Just use plain old HTTP/1.0.
  //
  // Note: TAsyncSocket::write() does not make a copy of the write buffer,
  // so we are responsible for ensuring that it exists until the write cob is
  // invoked.  We just store it as a member variable.
  httpRequestLength_ = snprintf(buffer_, sizeof(buffer_),
                                "GET %s HTTP/1.0\r\n\r\n", path_.c_str());

  // Ask the socket to write the request, and invoke
  // this->writeFinished() when it has finished sending the request
  socket_->write(this, reinterpret_cast<const uint8_t*>(buffer_),
                 httpRequestLength_);
}

void HttpFetcher::writeSuccess() noexcept {
  // The write has succeeded.  Start reading the response
  socket_->setReadCallback(this);
}

void HttpFetcher::writeError(size_t bytesWritten,
                             const TTransportException& ex) noexcept {
  fail(string("failed to write HTTP request: ") + ex.what());
}

void HttpFetcher::getReadBuffer(void** bufReturn, size_t* lenReturn) {
  *bufReturn = buffer_;
  *lenReturn = sizeof(buffer_);
}

void HttpFetcher::readDataAvailable(size_t len) noexcept {
  // We read some data.  Copy the data out of buffer_ into response_,
  // so we can re-use buffer_ for the next read.
  response_.append(buffer_, len);
}

void HttpFetcher::readEOF() noexcept {
  // The remote end closed the connection.  This means we have the full
  // request.  (Since we're using HTTP/1.0, the server will immediately close
  // the connection after sending the response.)
  //
  // Invoke the callback object with the full response string
  cob_(response_);
}

void HttpFetcher::readError(const TTransportException& ex) noexcept {
  fail(string("failed to read from socket: ") + ex.what());
}

void HttpFetcher::fail(string const& msg) {
  // Invoke errorCob_ to inform our user that we failed
  HttpError e;
  e.message = msg;
  errorCob_(e);
}

}}}}
