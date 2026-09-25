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
// The remaining DISABLED_ test covers PAYLOAD, which still uses the plain path.

#include <cstdint>
#include <cstring>
#include <numeric>
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
    const size_t allocationSize = capacity + extraTailroom_;
    allocations_.push_back(allocationSize);
    return folly::IOBuf::create(allocationSize);
  }

  struct Run {
    Result result;
    size_t bytesAllocated;
  };

  // Ends with a getReadBuffer call, the way a socket asks for the next buffer
  // before it knows more bytes are coming. That call is where the parser
  // allocates, so a test that stops at the last consume sees nothing. frames_
  // and allocations_ hold only what this pass produced.
  Run drive(AlignedParser& parser, const std::vector<uint8_t>& bytes) {
    frames_.clear();
    allocations_.clear();
    parser.setIOBufFactory(&factory_);
    const Result result = ConsumeDriver::feed(
        parser, folly::ByteRange{bytes.data(), bytes.size()}, sink());
    void* buf = nullptr;
    size_t room = 0;
    parser.getReadBuffer(&buf, &room);
    return Run{
        result,
        std::accumulate(allocations_.begin(), allocations_.end(), size_t{0})};
  }

  std::vector<BytesPtr> frames_;
  Result sinkResult_{Result::Success};
  std::vector<size_t> allocations_;
  size_t extraTailroom_{0};

  // parser_ holds a raw pointer to factory_, so factory_ has to outlive it.
  // Members die in reverse order of declaration, so factory_ is declared
  // first and parser_ last.
  folly::IOBufFactory factory_{
      [this](size_t capacity) { return recordAllocation(capacity); }};
  AlignedParser parser_;
};

// Metadata and data must come back in separate buffers, not merged into one.
TEST_F(AlignedParserTest, EmitsMetadataAndDataInSeparateBuffers) {
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

  BytesPtr body = frames_[0]->clone();
  body->trimStart(kBaseHeaderSize);

  ReadChunkResponse parsed;
  PutResult parsedResult;
  parsedResult.get<0>().value = &parsed;
  BinaryProtocolReader protocolReader;
  protocolReader.setInput(body.get());
  parsedResult.read(&protocolReader);

  ASSERT_THAT(parsed.data().value(), NotNull());
  // One piece. More than one means the field was split across buffers.
  ASSERT_THAT(parsed.data().value()->countChainElements(), Eq(1));
  // That piece must not share an allocation with any other part of the
  // frame.
  const folly::IOBuf* node = frames_[0].get();
  do {
    const bool shared = sameAllocation(*parsed.data().value(), *node);
    EXPECT_THAT(shared, Eq(node == &dataNode(*frames_[0])));
    node = node->next();
  } while (node != frames_[0].get());
}

// The blob inside the request must end up at an address divisible by 16.
TEST_F(AlignedParserTest, BlobLandsOnAnAlignmentBoundary) {
  constexpr size_t kBlobSize = 4096;
  folly::IOBufFactory factory = [](size_t capacity) {
    BytesPtr buffer = folly::IOBuf::create(capacity + 1);
    buffer->advance(1);
    return buffer;
  };
  AlignedParser parser;
  parser.setIOBufFactory(&factory);

  Payload payload;
  payload.blob() = blobOf(kBlobSize, 'd');

  auto channel = std::make_shared<CaptureChannel>();
  apache::thrift::Client<AlignedService> client(channel);
  ReadChunkResponse ignored;
  client.sync_put(ignored, payload);
  // Without this the test would feed an empty frame and prove nothing.
  ASSERT_THAT(channel->request_, NotNull());

  const std::vector<uint8_t> wire = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      nullptr,
      std::move(channel->request_));
  EXPECT_THAT(
      ConsumeDriver::feed(
          parser, folly::ByteRange{wire.data(), wire.size()}, sink()),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));

  BytesPtr body = frames_[0]->clone();
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
TEST_F(AlignedParserTest, PayloadDataIsNotShifted) {
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
TEST_F(AlignedParserTest, UnalignedFrameTypesAreEmittedWhole) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestFnfHeader{.streamId = 1}, nullptr, blobOf(50, 'd'))),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize + 50));
}

// A CANCEL frame is a header and nothing else. It must still be emitted.
TEST_F(AlignedParserTest, CancelFrameIsEmitted) {
  EXPECT_THAT(
      feed(serializeFrame(write::CancelHeader{.streamId = 1})),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize));
}

// An aligned frame with an empty data field must be emitted.
TEST_F(AlignedParserTest, AlignedFrameWithNoDataIsEmitted) {
  EXPECT_THAT(
      feed(serializeFrame(
          write::RequestResponseHeader{.streamId = 1}, nullptr, nullptr)),
      Eq(Result::Success));
  ASSERT_THAT(frames_, SizeIs(1));
  EXPECT_THAT(frames_[0]->computeChainDataLength(), Eq(kBaseHeaderSize));
}

// A frame shorter than its own header must give Result::Error.
TEST_F(AlignedParserTest, RejectsFrameShorterThanItsHeader) {
  std::vector<uint8_t> bytes(kMetadataLengthSize + kBaseHeaderSize, 0);
  bytes[2] = static_cast<uint8_t>(kBaseHeaderSize - 1);

  EXPECT_THAT(feed(bytes), Eq(Result::Error));
  EXPECT_THAT(frames_, IsEmpty());
}

// A frame bigger than the cap must be refused, and refused before any room is
// reserved for it.
TEST_F(AlignedParserTest, RejectsFrameOverMaxFrameSize) {
  constexpr size_t kMaxFrameSize = 1024;
  constexpr size_t kHeaderSize = kMetadataLengthSize + kBaseHeaderSize;

  const auto announce = [this](size_t frameLength) {
    AlignedParser parser{
        AlignedParser::kDefaultMinBufferSize,
        AlignedParser::kDefaultMaxBufferSize,
        kMaxFrameSize};
    std::vector<uint8_t> bytes(kHeaderSize, 0);
    write::writeFrameLength(bytes.data(), frameLength);
    return drive(parser, bytes);
  };

  // Control: a frame under the cap is taken, and taking it reserves room past
  // the header. Without this the check below would also pass on a parser that
  // never reserves anything.
  const Run accepted = announce(kMaxFrameSize / 2);
  ASSERT_THAT(accepted.result, Eq(Result::Success));
  ASSERT_THAT(accepted.bytesAllocated, Gt(kHeaderSize));

  // A parser that has read nothing has still allocated one header buffer. The
  // refused frame must cost that and nothing more.
  AlignedParser fresh;
  const size_t headerBufferOnly = drive(fresh, {}).bytesAllocated;

  const Run refused = announce(kMaxFrameSize + 1);
  EXPECT_THAT(refused.result, Eq(Result::Error));
  EXPECT_THAT(frames_, IsEmpty());
  EXPECT_THAT(refused.bytesAllocated, Eq(headerBufferOnly));
}

// On the plain path, a tail with less room than minBufferSize makes the next
// read ask for the whole wire frame size.
TEST_F(AlignedParserTest, PlainFrameReservesAnnouncedLengthWithShortTail) {
  constexpr size_t kBodySize = 1 << 20;

  // Leave one byte after the header, below minBufferSize.
  extraTailroom_ = 1;

  std::vector<uint8_t> headerOnly = serializeFrame(
      write::RequestFnfHeader{.streamId = 1}, nullptr, blobOf(kBodySize, 'd'));
  headerOnly.resize(kMetadataLengthSize + kBaseHeaderSize);

  AlignedParser parser;
  const Run run = drive(parser, headerOnly);
  EXPECT_THAT(run.result, Eq(Result::Success));
  EXPECT_THAT(frames_, IsEmpty());
  EXPECT_THAT(run.bytesAllocated, Ge(kBodySize));
}

// An aligned frame is the other way round. Its data has to arrive in one
// buffer, so the parser takes the whole announced size as soon as the header
// is in, before a single data byte has turned up.
TEST_F(AlignedParserTest, AlignedFrameReservesTheAnnouncedLength) {
  constexpr size_t kDataSize = 1 << 20;

  std::vector<uint8_t> headerOnly = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      nullptr,
      blobOf(kDataSize, 'd'));
  headerOnly.resize(kMetadataLengthSize + kBaseHeaderSize);

  AlignedParser parser;
  const Run run = drive(parser, headerOnly);
  EXPECT_THAT(run.result, Eq(Result::Success));
  EXPECT_THAT(frames_, IsEmpty());
  EXPECT_THAT(run.bytesAllocated, Ge(kDataSize));
}

// A metadata length that does not fit the frame must give Result::Error too.
TEST_F(AlignedParserTest, RejectsMetadataLongerThanTheFrame) {
  std::vector<uint8_t> bytes = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      blobOf(4, 'm'),
      blobOf(8, 'd'));

  // The metadata length comes after the frame length and the base header.
  const size_t at = kMetadataLengthSize + kBaseHeaderSize;
  bytes[at] = 0xFF;
  bytes[at + 1] = 0xFF;
  bytes[at + 2] = 0xFF;

  EXPECT_THAT(feed(bytes), Eq(Result::Error));
  EXPECT_THAT(frames_, IsEmpty());
}

// The data buffer must come from the installed factory, not straight from
// folly. Zero copy needs it that way.
TEST_F(AlignedParserTest, DataBufferComesFromTheInstalledFactory) {
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
TEST_F(AlignedParserTest, OffersRoomForOneFieldAtATime) {
  constexpr size_t kDataSize = 100;
  constexpr size_t kHeaderSize = kMetadataLengthSize + kBaseHeaderSize;

  void* buf = nullptr;
  size_t avail = 0;
  parser_.getReadBuffer(&buf, &avail);
  ASSERT_THAT(avail, Eq(kHeaderSize));

  const std::vector<uint8_t> bytes = serializeFrame(
      write::RequestResponseHeader{.streamId = 1},
      nullptr,
      blobOf(kDataSize, 'd'));
  std::memcpy(buf, bytes.data(), kHeaderSize);
  EXPECT_THAT(parser_.consume(kHeaderSize, sink()), Eq(Result::Success));

  parser_.getReadBuffer(&buf, &avail);
  EXPECT_THAT(avail, Eq(kDataSize));
}

} // namespace apache::thrift::fast_thrift::frame::read
