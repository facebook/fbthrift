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

#include <folly/io/async/EventBase.h>
#include <stdint.h>
#include <thrift/lib/cpp2/transport/core/ThriftChannelIf.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/ClientChannel.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * The API to the transport layer.
 *
 * Concrete implementations support specific transports such as Proxygen
 * and RSocket.
 *
 * The primary method in this API is "getChannel()" which returns a
 * "ThriftChannelIf" object which is used to interface between the
 * Thrift layer and this layer.  A separate call to "getChannel()"
 * must be made for each RPC.
 *
 * "ClientConnectionIf" objects operate on an event base that must be
 * supplied when an object is constructed - see comments on each
 * subclass for more details.  The event base loop must be running.
 * This is the event base on which callbacks from the networking layer
 * take place.  This is also the event base on which calls to
 * "ThriftChannelIf" objects must be made from ThriftClient objects.
 *
 * Multiple "ClientConnectionIf" objects may be present at the same
 * time, each will manage a separate network connection and are
 * generally on different event bases (threads).
 */
class ClientConnectionIf {
 protected:
  ClientConnectionIf() = default;

 public:
  virtual ~ClientConnectionIf() = default;

  ClientConnectionIf(const ClientConnectionIf&) = delete;
  ClientConnectionIf& operator=(const ClientConnectionIf&) = delete;

  // Returns a channel object for use on a single RPC.  This can be
  // called from any thread.  Throws TTransportException if a channel
  // object cannot be returned.
  virtual std::shared_ptr<ThriftChannelIf> getChannel() = 0;

  // Sets the maximum pending outgoing requests allowed on this
  // connection.  Subject to negotiation with the server, which may
  // dictate a smaller maximum.
  virtual void setMaxPendingRequests(uint32_t num) = 0;

  // Returns the event base of the underlying transport.  This is also
  // the event base on which all calls to channel object (obtained via
  // "getChannel()") must be scheduled.
  virtual folly::EventBase* getEventBase() const = 0;

  // The following methods are proxies for ClientChannel methods.
  // They are called from corresponding "ThriftClient" methods, which
  // in turn can be used from any client side connection management
  // framework.
  virtual apache::thrift::async::TAsyncTransport* getTransport() = 0;
  virtual bool good() = 0;
  virtual ClientChannel::SaturationStatus getSaturationStatus() = 0;
  virtual void attachEventBase(folly::EventBase* evb) = 0;
  virtual void detachEventBase() = 0;
  virtual bool isDetachable() = 0;
  virtual bool isSecurityActive() = 0;
  virtual uint32_t getTimeout() = 0;
  virtual void setTimeout(uint32_t ms) = 0;
  virtual void closeNow() = 0;
  virtual CLIENT_TYPE getClientType() = 0;
};

} // namespace thrift
} // namespace apache
