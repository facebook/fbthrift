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

// Tests that are only about AlignedParser. The framing it shares with other
// parsers is tested in ParserContractTest.cpp.
//
// Every test here is DISABLED_. They spell out what the parser has to do, and
// the parser is still an empty skeleton, so all of them fail. Drop the prefix
// as the parser gets written.

#include <cstdint>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/portability/GMock.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/AlignedParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/ParserContractFixture.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/WireFrames.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/gen-cpp2/AlignedService.tcc>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/gen-cpp2/AlignedServiceAsyncClient.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameHeaders.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameWriter.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

namespace apache::thrift::fast_thrift::frame::read {

using namespace testing;

using channel_pipeline::BytesPtr;
using channel_pipeline::Result;

namespace {

BytesPtr blobOf(size_t size, char fill) {
  return folly::IOBuf::copyBuffer(std::string(size, fill));
}

// The struct Thrift generates around the arguments of AlignedService::put.
// The server reads a request with this type, so the test reads one with it too.
using PutArgs = AlignedService_put_pargs;

// The same for the return value of AlignedService::put.
using PutResult = AlignedService_put_presult;

// Serializes a response the way the server does, so Thrift lays out the bytes
// and the test does not.
BytesPtr serializeResponse(ReadChunkResponse& response) {
  PutResult result;
  result.get<0>().value = &response;
  result.setIsSet(0);

  folly::IOBufQueue queue;
  BinaryProtocolWriter writer;
  writer.setOutput(&queue);
  result.write(&writer);
  return queue.move();
}

// Sends a request the way a real client does, but keeps the bytes instead of
// putting them on a socket. Thrift lays out the request, not the test.
class CaptureChannel : public apache::thrift::RequestChannel {
 public:
  void sendRequestResponse(
      const apache::thrift::RpcOptions&,
      apache::thrift::MethodMetadata&&,
      apache::thrift::SerializedRequest&& request,
      std::shared_ptr<apache::thrift::transport::THeader>,
      apache::thrift::RequestClientCallback::Ptr cb,
      std::unique_ptr<folly::IOBuf>) override {
    request_ = std::move(request.buffer);

    // sync_put parses whatever comes back, so it needs a real response.
    ReadChunkResponse response;
    response.data() = folly::IOBuf::create(0);
    cb.release()->onResponse(
        apache::thrift::ClientReceiveState(
            apache::thrift::protocol::T_BINARY_PROTOCOL,
            apache::thrift::MessageType::T_REPLY,
            apache::thrift::SerializedResponse(serializeResponse(response)),
            nullptr,
            nullptr,
            apache::thrift::RpcTransportStats()));
  }

  void setCloseCallback(apache::thrift::CloseCallback*) override {}

  folly::EventBase* getEventBase() const override { return nullptr; }

  uint16_t getProtocolId() override {
    return apache::thrift::protocol::T_BINARY_PROTOCOL;
  }

  BytesPtr request_;
};

} // namespace

class AlignedParserTest : public Test {
 protected:
  // Collects the frames the parser emits. Returns whatever Result the test
  // asked for, so a test can make the sink refuse a frame.
  auto sink() noexcept {
    return [this](BytesPtr&& frame) noexcept {
      frames_.push_back(std::move(frame));
      return sinkResult_;
    };
  }

  // Feeds bytes the way a socket does: ask the parser for a buffer, fill it,
  // tell the parser how much arrived, repeat. ConsumeDriver runs that loop and
  // is shared with the contract tests.
  Result feed(const std::vector<uint8_t>& bytes) {
    return ConsumeDriver::feed(
        parser_, folly::ByteRange{bytes.data(), bytes.size()}, sink());
  }

  // Data is always the last buffer. The order is header, metadata, data.
  const folly::IOBuf& dataNode(const folly::IOBuf& frame) const {
    return *frame.prev();
  }

  // True when the two buffers are slices of one allocation.
  static bool sameAllocation(const folly::IOBuf& a, const folly::IOBuf& b) {
    return a.buffer() < b.buffer() + b.capacity() &&
        b.buffer() < a.buffer() + a.capacity();
  }

  // The body of factory_. A test that wants to see what the parser allocated
  // calls parser_.setIOBufFactory(&factory_) and then reads allocations_.
  BytesPtr recordAllocation(size_t capacity) {
    allocations_.push_back(capacity);
    return folly::IOBuf::create(capacity);
  }

  std::vector<BytesPtr> frames_;
  Result sinkResult_{Result::Success};
  std::vector<size_t> allocations_;

  // parser_ holds a raw pointer to factory_, so factory_ has to outlive it.
  // Members die in reverse order of declaration, so factory_ is declared
  // first and parser_ last.
  folly::IOBufFactory factory_{
      [this](size_t capacity) { return recordAllocation(capacity); }};
  AlignedParser parser_;
};

// Metadata and data must come back in separate buffers, not merged into one.
TEST_F(AlignedParserTest, DISABLED_EmitsMetadataAndDataInSeparateBuffers) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestResponseHeader{.streamId = 1},
          blobOf(40, 'm'),
          blobOf(100, 'd'))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  ASSERT_THAT(frames_[0]->countChainElements(), Eq(3));
  EXPECT_THAT(frames_[0]->length(), Eq(kBaseHeaderSize + kMetadataLengthSize));
  EXPECT_THAT(frames_[0]->next()->length(), Eq(40));
  EXPECT_THAT(frames_[0]->prev()->length(), Eq(100));
}

// A binary field of the response must end up in a buffer of its own. Code that
// keeps that field then keeps only those bytes. If the field pointed into a
// buffer shared with the header, it would keep the header alive too.
TEST_F(AlignedParserTest, DISABLED_ResponseBinaryFieldHasItsOwnBuffer) {
  constexpr size_t kDataSize = 4096;

  ReadChunkResponse response;
  response.data() = blobOf(kDataSize, 'd');
  response.checksum() = 7;

  EXPECT_THAT(
      feed(serializeFrame(
          write::PayloadHeader{.streamId = 1, .next = true},
          nullptr,
          serializeResponse(response))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));

  auto body = frames_[0]->clone();
  body->trimStart(kBaseHeaderSize);

  ReadChunkResponse parsed;
  PutResult parsedResult;
  parsedResult.get<0>().value = &parsed;
  BinaryProtocolReader protocolReader;
  protocolReader.setInput(body.get());
  parsedResult.read(&protocolReader);

  ASSERT_THAT(parsed.data().value(), NotNull());
  EXPECT_THAT(sameAllocation(*parsed.data().value(), *frames_[0]), IsFalse());
}

// The blob inside the request must end up at an address divisible by 16.
TEST_F(AlignedParserTest, DISABLED_BlobLandsOnAnAlignmentBoundary) {
  constexpr size_t kBlobSize = 4096;

  Payload payload;
  payload.blob() = blobOf(kBlobSize, 'd');

  auto channel = std::make_shared<CaptureChannel>();
  apache::thrift::Client<AlignedService> client(channel);
  ReadChunkResponse ignored;
  client.sync_put(ignored, payload);
  // Without this the test would feed an empty frame and prove nothing.
  ASSERT_THAT(channel->request_, NotNull());

  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestResponseHeader{.streamId = 1},
          nullptr,
          std::move(channel->request_))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));

  auto body = frames_[0]->clone();
  body->trimStart(kBaseHeaderSize);

  Payload parsed;
  PutArgs parsedArgs;
  parsedArgs.get<0>().value = &parsed;
  BinaryProtocolReader protocolReader;
  protocolReader.setInput(body.get());
  parsedArgs.read(&protocolReader);

  ASSERT_THAT(parsed.blob().value(), NotNull());
  const auto address =
      reinterpret_cast<uintptr_t>(parsed.blob().value()->data());
  EXPECT_THAT(address % 16, Eq(0));
}

// PAYLOAD frame gets no shift, only REQUEST_RESPONSE needs one.
TEST_F(AlignedParserTest, DISABLED_PayloadDataIsNotShifted) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::PayloadHeader{.streamId = 1, .next = true},
          nullptr,
          blobOf(100, 'd'))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(dataNode(*frames_[0]).headroom(), Eq(0));
}

// Other frame types go down a different path inside the parser. They must
// still come out whole.
TEST_F(AlignedParserTest, DISABLED_UnalignedFrameTypesAreEmittedWhole) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestFnfHeader{.streamId = 1}, nullptr, blobOf(50, 'd'))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize + 50));
}

// A CANCEL frame is a header and nothing else. It must still be emitted.
TEST_F(AlignedParserTest, DISABLED_CancelFrameIsEmitted) {
  EXPECT_THAT(
      feed(serializeFrame(write::CancelHeader{.streamId = 1})),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize));
}

// An aligned frame with an empty data field must be emitted.
TEST_F(AlignedParserTest, DISABLED_AlignedFrameWithNoDataIsEmitted) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestResponseHeader{.streamId = 1}, nullptr, nullptr)),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize));
}

// A frame shorter than its own header must give Result::Error.
TEST_F(AlignedParserTest, DISABLED_RejectsFrameShorterThanItsHeader) {
  std::vector<uint8_t> bytes(kMetadataLengthSize + kBaseHeaderSize, 0);
  bytes[2] = static_cast<uint8_t>(kBaseHeaderSize - 1);

  EXPECT_THAT(feed(bytes), Eq(Result::Error));
  EXPECT_THAT(frames_, IsEmpty());
}

// A metadata length that does not fit the frame must give Result::Error too.
TEST_F(AlignedParserTest, DISABLED_RejectsMetadataLongerThanTheFrame) {
  auto bytes = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      blobOf(4, 'm'),
      blobOf(8, 'd'));

  // The metadata length comes after the frame length and the base header.
  const auto at = kMetadataLengthSize + kBaseHeaderSize;
  bytes[at] = 0xFF;
  bytes[at + 1] = 0xFF;
  bytes[at + 2] = 0xFF;

  EXPECT_THAT(feed(bytes), Eq(Result::Error));
  EXPECT_THAT(frames_, IsEmpty());
}

// The data buffer must come from the installed factory, not straight from
// folly. Zero copy needs it that way.
TEST_F(AlignedParserTest, DISABLED_DataBufferComesFromTheInstalledFactory) {
  constexpr size_t kDataSize = 4096;
  parser_.setIOBufFactory(&factory_);

  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestResponseHeader{.streamId = 1},
          nullptr,
          blobOf(kDataSize, 'd'))),
      Eq(Result::Success));
  EXPECT_THAT(allocations_, Contains(Ge(kDataSize)));
}

// getReadBuffer must offer only what the current field still needs. If it
// offers more, bytes of the next field land in the wrong buffer and the
// fields stop being separate.
TEST_F(AlignedParserTest, DISABLED_OffersRoomForOneFieldAtATime) {
  constexpr size_t kDataSize = 100;
  constexpr size_t kHeaderSize = kMetadataLengthSize + kBaseHeaderSize;

  void* buf = nullptr;
  size_t avail = 0;
  parser_.getReadBuffer(&buf, &avail);
  ASSERT_THAT(avail, Eq(kHeaderSize));

  const auto bytes = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      nullptr,
      blobOf(kDataSize, 'd'));
  std::memcpy(buf, bytes.data(), kHeaderSize);
  EXPECT_THAT(parser_.consume(kHeaderSize, sink()), Eq(Result::Success));

  parser_.getReadBuffer(&buf, &avail);
  EXPECT_THAT(avail, Eq(kDataSize));
}

} // namespace apache::thrift::fast_thrift::frame::read
