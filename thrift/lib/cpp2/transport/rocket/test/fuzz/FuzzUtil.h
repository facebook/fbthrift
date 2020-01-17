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

#include <folly/ExceptionWrapper.h>

#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>

namespace apache {
namespace thrift {
namespace rocket {
namespace test {

class FakeTransport final : public folly::AsyncTransportWrapper {
 public:
  explicit FakeTransport(folly::EventBase* e) : eventBase_(e) {}
  void setReadCB(ReadCallback*) override {}
  ReadCallback* getReadCallback() const override {
    return nullptr;
  }
  void write(WriteCallback* cb, const void*, size_t, folly::WriteFlags)
      override {
    cb->writeSuccess();
  }
  void writev(WriteCallback* cb, const iovec*, size_t, folly::WriteFlags)
      override {
    cb->writeSuccess();
  }
  void writeChain(
      WriteCallback* cb,
      std::unique_ptr<folly::IOBuf>&&,
      folly::WriteFlags) override {
    cb->writeSuccess();
  }
  folly::EventBase* getEventBase() const override {
    return eventBase_;
  }
  void getAddress(folly::SocketAddress*) const override {}
  void close() override {}
  void closeNow() override {}
  void shutdownWrite() override {}
  void shutdownWriteNow() override {}
  bool good() const override {
    return true;
  }
  bool readable() const override {
    return true;
  }
  bool connecting() const override {
    return true;
  }
  bool error() const override {
    return true;
  }
  void attachEventBase(folly::EventBase*) override {}
  void detachEventBase() override {}
  bool isDetachable() const override {
    return true;
  }
  void setSendTimeout(uint32_t) override {}
  uint32_t getSendTimeout() const override {
    return 0u;
  }
  void getLocalAddress(folly::SocketAddress*) const override {}
  void getPeerAddress(folly::SocketAddress*) const override {}
  bool isEorTrackingEnabled() const override {
    return true;
  }
  void setEorTracking(bool) override {}
  size_t getAppBytesWritten() const override {
    return 0u;
  }
  size_t getRawBytesWritten() const override {
    return 0u;
  }
  size_t getAppBytesReceived() const override {
    return 0u;
  }
  size_t getRawBytesReceived() const override {
    return 0u;
  }

 private:
  folly::EventBase* eventBase_;
};

class FakeServerHandler final
    : public apache::thrift::rocket::RocketServerHandler {
 public:
  void handleRequestResponseFrame(
      apache::thrift::rocket::RequestResponseFrame&&,
      apache::thrift::rocket::RocketServerFrameContext&&) override {
    LOG(INFO) << "handleRequestResponseFrame";
  }

  void handleRequestFnfFrame(
      apache::thrift::rocket::RequestFnfFrame&&,
      apache::thrift::rocket::RocketServerFrameContext&&) override {}

  void handleRequestStreamFrame(
      apache::thrift::rocket::RequestStreamFrame&&,
      apache::thrift::rocket::RocketStreamClientCallback* cb) override {
    // avoid cb leak
    LOG(INFO) << "handleRequestStreamFrame";
    folly::exception_wrapper ex;
    cb->onFirstResponseError(std::move(ex));
  }

  void handleRequestChannelFrame(
      apache::thrift::rocket::RequestChannelFrame&&,
      apache::thrift::rocket::RocketSinkClientCallback* cb) override {
    // avoid cb leak
    LOG(INFO) << "handleRequestChannelFrame";
    folly::exception_wrapper ex;
    cb->onFirstResponseError(std::move(ex));
  }
};

void testOneInput(
    const uint8_t* Data,
    size_t Size,
    folly::AsyncTransportWrapper::UniquePtr sock) {
  auto connection = new RocketServerConnection(
      std::move(sock),
      std::make_shared<FakeServerHandler>(),
      std::chrono::milliseconds::zero());
  folly::DelayedDestruction::DestructorGuard dg(connection);
  Parser<RocketServerConnection> p(*connection);
  size_t left = Size;
  while (left != 0) {
    void* buffer;
    size_t length;
    p.getReadBuffer(&buffer, &length);
    size_t lenToRead = std::min(left, length);
    memcpy(buffer, Data, lenToRead);
    p.readDataAvailable(lenToRead);
    Data += lenToRead;
    left -= lenToRead;
  }
  connection->destroy();
}

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
