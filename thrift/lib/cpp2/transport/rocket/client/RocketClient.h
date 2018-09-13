/*
 * Copyright 2018-present Facebook, Inc.
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

#include <chrono>
#include <memory>
#include <string>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/SocketAddress.h>
#include <folly/Try.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/AsyncTransport.h>
#include <folly/io/async/DelayedDestruction.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>
#include <thrift/lib/cpp2/transport/rocket/client/RequestContextQueue.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>

namespace folly {
class IOBuf;
} // namespace folly

namespace apache {
namespace thrift {
namespace rocket {

class RocketClient : public folly::DelayedDestruction,
                     private folly::AsyncTransportWrapper::WriteCallback,
                     public std::enable_shared_from_this<RocketClient> {
 public:
  RocketClient(const RocketClient&) = delete;
  RocketClient(RocketClient&&) = delete;
  RocketClient& operator=(const RocketClient&) = delete;
  RocketClient& operator=(RocketClient&&) = delete;

  ~RocketClient() final;

  static std::shared_ptr<RocketClient> create(
      folly::EventBase& evb,
      folly::AsyncTransportWrapper::UniquePtr socket);

  // Main send*Sync() API. Must be called on the EventBase's FiberManager.
  Payload sendRequestResponseSync(
      Payload&& request,
      std::chrono::milliseconds timeout);

  void sendRequestFnfSync(Payload&& request);

  void scheduleWrite(RequestContext& ctx);

  // WriteCallback implementation
  void writeSuccess() noexcept final;
  void writeErr(
      size_t bytesWritten,
      const folly::AsyncSocketException& e) noexcept final;

  // Hard close: stop reading from socket and abort all in-progress writes
  // immediately.
  void closeNow() noexcept;

  // Initiate shutdown, but not as hard as closeNow(). Pending writes buffered
  // up within AsyncSocket will still have a chance to complete (all the way to
  // writeSuccess() or writeErr()). Other in-progress requests will be failed
  // with the exception specified by ew.
  void close(folly::exception_wrapper ew) noexcept;

  const folly::AsyncTransportWrapper* getTransportWrapper() const {
    return socket_.get();
  }

  folly::AsyncTransportWrapper* getTransportWrapper() {
    return socket_.get();
  }

 private:
  folly::EventBase* evb_;
  folly::AsyncTransportWrapper::UniquePtr socket_;
  StreamId nextStreamId_{1};
  bool setupFrameSent_{false};
  enum class ConnectionState : uint8_t {
    CONNECTED,
    CLOSED,
    ERROR,
  };
  // Client must be constructed with an already open socket
  ConnectionState state_{ConnectionState::CONNECTED};

  RequestContextQueue queue_;

  Parser<RocketClient> parser_{*this};

  class WriteLoopCallback : public folly::EventBase::LoopCallback {
   public:
    explicit WriteLoopCallback(RocketClient& client) : client_(client) {}
    ~WriteLoopCallback() final = default;
    void runLoopCallback() noexcept final;

   private:
    RocketClient& client_;
    bool rescheduled_{false};
  };
  WriteLoopCallback writeLoopCallback_;

  std::unique_ptr<folly::EventBase::LoopCallback> eventBaseDestructionCallback_;

  RocketClient(
      folly::EventBase& evb,
      folly::AsyncTransportWrapper::UniquePtr socket);

  StreamId makeStreamId() {
    const StreamId rid = nextStreamId_;
    // rsocket protocol specifies that clients must generate odd stream IDs
    nextStreamId_ += 2;
    return rid;
  }

  void handleFrame(folly::IOBuf&& frame);
  void handleRequestResponseFrame(
      RequestContext& ctx,
      FrameType frameType,
      folly::IOBuf&& frame);

  void writeScheduledRequestsToSocket() noexcept;

  template <class T>
  friend class Parser;
};

} // namespace rocket
} // namespace thrift
} // namespace apache
