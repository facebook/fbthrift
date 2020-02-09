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

#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Parser.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerConnection.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>
#include <thrift/lib/cpp2/transport/rocket/server/ThriftRocketServerHandler.h>

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

class FakeProcessor final : public apache::thrift::AsyncProcessor {
 public:
  // not used by rocket
  bool isOnewayMethod(
      const folly::IOBuf*,
      const apache::thrift::transport::THeader*) override {
    return false;
  }

  void process(
      apache::thrift::ResponseChannelRequest::UniquePtr req,
      std::unique_ptr<folly::IOBuf>,
      apache::thrift::protocol::PROTOCOL_TYPES,
      apache::thrift::Cpp2RequestContext*,
      folly::EventBase*,
      apache::thrift::concurrency::ThreadManager*) {
    req->sendErrorWrapped(
        folly::make_exception_wrapper<apache::thrift::TApplicationException>(
            apache::thrift::TApplicationException::TApplicationExceptionType::
                INTERNAL_ERROR,
            "place holder"),
        "1" /* doesnt matter */);
  }
};

class FakeProcessorFactory final
    : public apache::thrift::AsyncProcessorFactory {
 public:
  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return std::make_unique<FakeProcessor>();
  }
};

void testOneInput(
    const uint8_t* Data,
    size_t Size,
    folly::AsyncTransportWrapper::UniquePtr sock) {
  auto* const sockPtr = sock.get();
  apache::thrift::ThriftServer server;
  server.setProcessorFactory(std::make_shared<FakeProcessorFactory>());
  auto worker = apache::thrift::Cpp2Worker::create(&server);
  std::vector<std::unique_ptr<apache::thrift::rocket::SetupFrameHandler>> v;
  folly::SocketAddress address;
  auto connection = new apache::thrift::rocket::RocketServerConnection(
      std::move(sock),
      std::make_shared<apache::thrift::rocket::ThriftRocketServerHandler>(
          worker, address, sockPtr, v),
      std::chrono::milliseconds::zero());
  folly::DelayedDestruction::DestructorGuard dg(connection);
  apache::thrift::rocket::Parser<apache::thrift::rocket::RocketServerConnection>
      p(*connection);
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
  connection->close(folly::exception_wrapper());
}

} // namespace test
} // namespace rocket
} // namespace thrift
} // namespace apache
