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

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <stdint.h>
#include <thrift/lib/cpp2/transport/core/FunctionInfo.h>
//#include <yarpl/flowable/Subscriber.h>
#include <map>
#include <memory>
#include <string>

namespace apache {
namespace thrift {

class ThriftClientCallback;

/**
 * Interface used by the Thrift layer to communicate with the
 * Transport layer.  The same interface is used both on the client
 * side and the server side.
 *
 * On the client side, this interface is used to send a request to the
 * server, while on the server side it is used to send a response to
 * the client.
 *
 * Implementations are specialized for different kinds of transports
 * (e.g., HTTP/2, RSocket) and using different strategies (e.g., one
 * RPC per HTTP/2 stream, or multiple RPCs per HTTP/2 stream).  The
 * same implementation is used on both the client and the server side.
 *
 * The lifetime of a channel object depends on the implementation - on
 * the one extreme, a new object may be created for each RPC, while on
 * the other extreme a single object may be use for the lifetime of a
 * connection.
 *
 * The channel object supports streaming as follows:
 * - Server and client keeps an instance of the channel as a twin of
 *   each other. The input in one side is perceived as output in the other
 *   side, and vice versa.
 *
 * - For RPCs with streaming requests, the channel provides the
 *   "getOutput()" method to obtain the stream into which the client
 *   streams messages.
 *   The channel also provides "setInput(stream)" method for the server
 *   to register a sink that is to receive these messages.
 *
 * - For RPCs with streaming responses, the operations are reversed.
 *   The client registers a sink for incoming messages via the channel's
 *   "setInput(stream)" method. While the server uses the channel's
 *   "getOutput()" method to obtain a stream and to use it for streaming
 *   messages.
 */
class ThriftChannelIf : public std::enable_shared_from_this<ThriftChannelIf> {
 public:
  // yarpl::Reference is a reference counted object, similar to shared_ptr.
  // Subscriber is an interface with methods: onNext, onComplete and onError.
  // @see <a
  // href="https://github.com/reactive-streams/reactive-streams-jvm/blob/v1.0.0/README.md#2-subscriber-code">Reactive
  // streams' Subscriber specification</a>
  //using SubscriberRef = yarpl::Reference<
  //    yarpl::flowable::Subscriber<std::unique_ptr<folly::IOBuf>>>;

  ThriftChannelIf() {}

  virtual ~ThriftChannelIf() = default;

  ThriftChannelIf(const ThriftChannelIf&) = delete;
  ThriftChannelIf& operator=(const ThriftChannelIf&) = delete;

  // Called on the server at the end of a single response RPC.  This
  // is not called for streaming response RPCs.  "seqId" is used to
  // match the request that this channel object sent to
  // ThriftProcessor.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  virtual void sendThriftResponse(
      uint32_t seqId,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload) noexcept = 0;

  // Can be called on the server to cancel a RPC (instead of calling
  // "sendThriftResponse()").  Once this is called, the server does
  // not have to perform any additional calls such as
  // "sendThriftResponse()", or closing of streams.
  //
  // Calls must be scheduled on the event base obtained from
  // "getEventBase()".
  //
  // TODO: cancel() has been added at a few places and will be used
  // to handle various error conditions.  The details of how this is
  // used is TBD.  We may add more parameters to the various
  // cancel() methods as necessary going forward.
  virtual void cancel(uint32_t seqId) noexcept = 0;

  // Called from the client to initiate an RPC with a server.
  // "callback" is used to call back with the response for single
  // (non-streaming) responses.
  //
  // Calls to the channel must be scheduled "this->getEventBase()".
  // Callbacks to "callback" must be scheduled on
  // "client->getEventBase()".
  //
  // "callback" must not be destroyed until it has received the call
  // back to "onThriftResponse()" or "cancel()".
  virtual void sendThriftRequest(
      std::unique_ptr<FunctionInfo> functionInfo,
      std::unique_ptr<std::map<std::string, std::string>> headers,
      std::unique_ptr<folly::IOBuf> payload,
      std::unique_ptr<ThriftClientCallback> callback) noexcept = 0;

  // Can be called from the client to cancel a previous call to
  // "sendThriftRequest()".  "callback" is used to identify the call
  // that is being cancelled.  The channel will not interact with
  // "callback" after this call.
  //
  // This may be called from any thread.
  virtual void cancel(ThriftClientCallback* callback) noexcept = 0;

  // Returns the event base on which calls to "sendThriftRequest()",
  // "sendThriftResponse()", and cancel must be scheduled.
  virtual folly::EventBase* getEventBase() noexcept = 0;

  // Used to manage streams on both the client and server side.

  // Caller provides a Subscriber to the Channel, meaning that, any data, that
  // is received, to be forwarded to the given subscriber.
  // virtual void setInput(uint32_t seqId, SubscriberRef sink) noexcept = 0;

  // Channel provides a Subscriber object to the caller. Caller can use this
  // object to send data to the receiving side.
  // virtual SubscriberRef getOutput(uint32_t seqId) noexcept = 0;
};

} // namespace thrift
} // namespace apache
