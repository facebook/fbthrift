/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

// How the in-memory transports deliver IOBuf chains through both read callback
// APIs.

#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/portability/GMock.h>

#include <thrift/lib/cpp2/fast_thrift/transport/test/TestAsyncTransport.h>

namespace apache::thrift::fast_thrift::transport::test {

using namespace testing;

namespace {

// Offers a fixed amount of room at a time and keeps everything it is given, so
// a test can check that nothing was dropped and the order held.
class RecordingReadCallback : public folly::AsyncTransport::ReadCallback {
 public:
  RecordingReadCallback(size_t roomPerCall, TestAsyncTransport* transport)
      : scratch_(roomPerCall), transport_(transport) {}

  void getReadBuffer(void** bufReturn, size_t* lenReturn) override {
    *bufReturn = scratch_.data();
    *lenReturn = scratch_.size();
  }

  void readDataAvailable(size_t len) noexcept override {
    received_.append(reinterpret_cast<const char*>(scratch_.data()), len);
    if (detachAfter_ > 0 && received_.size() >= detachAfter_) {
      transport_->setReadCB(nullptr);
    }
  }

  void readEOF() noexcept override {}
  void readErr(const folly::AsyncSocketException&) noexcept override {}

  // Detaches once this many bytes have arrived, which is how the transport
  // handler asks for backpressure.
  void detachAfter(size_t bytes) { detachAfter_ = bytes; }

  std::string received_;

 private:
  std::vector<uint8_t> scratch_;
  TestAsyncTransport* transport_;
  size_t detachAfter_{0};
};

class MovableRecordingReadCallback
    : public folly::AsyncTransport::ReadCallback {
 public:
  void getReadBuffer(void** bufReturn, size_t* lenReturn) override {
    ADD_FAILURE() << "getReadBuffer called for a movable callback";
    *bufReturn = nullptr;
    *lenReturn = 0;
  }

  void readDataAvailable(size_t) noexcept override {
    ADD_FAILURE() << "readDataAvailable called for a movable callback";
  }

  bool isBufferMovable() noexcept override { return true; }

  void readBufferAvailable(
      std::unique_ptr<folly::IOBuf> data) noexcept override {
    received_ = std::move(data);
  }

  void readEOF() noexcept override {}
  void readErr(const folly::AsyncSocketException&) noexcept override {}

  std::unique_ptr<folly::IOBuf> received_;
};

} // namespace

class ReadDataDeliveryTest : public Test {
 protected:
  folly::EventBase evb_;
  TestAsyncTransport transport_{&evb_};
};

TEST_F(ReadDataDeliveryTest, GivesTheWholeChainToAMovableCallback) {
  MovableRecordingReadCallback recorder;
  transport_.setReadCB(&recorder);

  std::unique_ptr<folly::IOBuf> chain =
      folly::IOBuf::copyBuffer(std::string(100, 'a'));
  chain->appendToChain(folly::IOBuf::copyBuffer(std::string(100, 'b')));
  const folly::IOBuf* const original = chain.get();
  transport_.injectReadData(std::move(chain));

  EXPECT_THAT(recorder.received_.get(), Eq(original));
  if (!recorder.received_) {
    return;
  }
  EXPECT_THAT(recorder.received_->countChainElements(), Eq(2));
}

// A chain longer than the room on offer must arrive whole and in order. One
// round of getReadBuffer is not enough, and flattening the chain first would
// copy every byte a second time.
TEST_F(ReadDataDeliveryTest, DeliversAChainThatOutgrowsTheOfferedRoom) {
  RecordingReadCallback recorder{/*roomPerCall=*/64, &transport_};
  transport_.setReadCB(&recorder);

  auto chain = folly::IOBuf::copyBuffer(std::string(100, 'a'));
  chain->appendToChain(folly::IOBuf::copyBuffer(std::string(100, 'b')));
  chain->appendToChain(folly::IOBuf::copyBuffer(std::string(100, 'c')));
  transport_.injectReadData(std::move(chain));

  EXPECT_THAT(
      recorder.received_,
      Eq(std::string(100, 'a') + std::string(100, 'b') +
         std::string(100, 'c')));
}

TEST_F(ReadDataDeliveryTest, SupportsRoomLargerThanFourKilobytes) {
  RecordingReadCallback recorder{/*roomPerCall=*/8192, &transport_};
  transport_.setReadCB(&recorder);

  const std::string data(5000, 'a');
  transport_.injectReadData(folly::IOBuf::copyBuffer(data));

  EXPECT_THAT(recorder.received_, Eq(data));
}

// Taking bytes can make the handler detach itself, which is how it asks for
// backpressure. Nothing may be pushed at it after that.
TEST_F(ReadDataDeliveryTest, StopsWhenTheCallbackDetaches) {
  RecordingReadCallback recorder{/*roomPerCall=*/64, &transport_};
  recorder.detachAfter(64);
  transport_.setReadCB(&recorder);

  transport_.injectReadData(folly::IOBuf::copyBuffer(std::string(300, 'a')));

  EXPECT_THAT(recorder.received_, SizeIs(64));
}

} // namespace apache::thrift::fast_thrift::transport::test
