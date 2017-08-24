/*
 * Copyright 2017-present Facebook, Inc.
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

#pragma once

#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>

#include <folly/io/IOBuf.h>
#include <proxygen/httpserver/ResponseHandler.h>
#include <proxygen/lib/http/HTTPMessage.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>
#include <memory>

namespace apache {
namespace thrift {

class ThriftProcessor;

/**
 * Interface that specializes ThriftChannelIf for HTTP/2.  It supports
 * the translation between Thrift payloads and HTTP/2 streams.
 * Concrete implementations support different translation strategies.
 *
 * Objects of this class operate on a single HTTP/2 stream and run on
 * the thread managing the HTTP/2 network connection for that stream.
 *
 * Lifetime for objects of this class is managed via shared pointers:
 *
 * On the client side, [TO BE COMPLETED]
 *
 * On the server side, an H2RequestHandler object creates a shared
 * pointer of a channel object following which additional shared
 * pointers are passed to the single ThriftProcessor object using
 * shared_from_this().  When both the H2RequestHandler object and the
 * ThriftProcessor object are done with the channel object, all the
 * shared pointers are released and the channel object gets destroyed.
 *
 * Threading constraints:
 *
 * On the client side, [TO BE COMPLETED]
 *
 * On the server side, channel objects are created on a Proxygen
 * thread that manages an HTTP/2 connection.  This object calls the
 * ThriftProcessor object to perform the thrift call.  The thrift side
 * the calls back into this object via "sendThriftResponse()".  The
 * callback must be be scheduled on the event base of the Proxygen
 * thread.
 */
class H2ChannelIf : public ThriftChannelIf {
 public:
  // Constructor for server side that uses a ResponseHandler object
  // to write to the HTTP/2 stream.
  H2ChannelIf(ThriftProcessor* processor, proxygen::ResponseHandler* toHttp2)
      : processor_(processor),
        responseHandler_(toHttp2),
        httpTransaction_(nullptr) {}

  // Constructor for client side that uses a HTTPTransaction object
  // to write to the HTTP/2 stream.
  explicit H2ChannelIf(proxygen::HTTPTransaction* toHttp2)
      : responseHandler_(nullptr), httpTransaction_(toHttp2) {}

  virtual ~H2ChannelIf() = default;

  // Called from Proxygen at the beginning of the stream.
  virtual void onH2StreamBegin(
      std::unique_ptr<proxygen::HTTPMessage> headers) noexcept = 0;

  // Called from Proxygen whenever a body frame is available.
  virtual void onH2BodyFrame(
      std::unique_ptr<folly::IOBuf> contents) noexcept = 0;

  // Called from Proxygen at the end of the stream.  No more data will
  // be sent from Proxygen after this.
  virtual void onH2StreamEnd() noexcept = 0;

  // Called from Proxygen when the stream has closed.  Usually this is
  // called after all reading and writing to the stream have been
  // completed, but it could be called earlier in case of errors.  No
  // more writes to the stream should be performed after this point.
  // Also, after this call Proxygen will relinquish access to this
  // object.
  virtual void onH2StreamClosed() noexcept {
    responseHandler_ = nullptr;
    httpTransaction_ = nullptr;
  }

 protected:
  // The thrift processor used to execute RPCs (server side only).
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_;
  // Used to write messages to HTTP/2 on the server side.
  // Owned by H2RequestHandler.  Should not be used after
  // setStreamClosed() has been called.
  proxygen::ResponseHandler* responseHandler_;
  // Used to write messages to HTTP/2 on the client side.
  // [ADD MORE DETAILS]
  proxygen::HTTPTransaction* httpTransaction_;
};

} // namespace thrift
} // namespace apache
