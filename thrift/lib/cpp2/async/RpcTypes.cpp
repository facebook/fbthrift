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

#include <thrift/lib/cpp2/async/RpcTypes.h>

#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache {
namespace thrift {

namespace {

template <typename ProtocolWriter>
std::unique_ptr<folly::IOBuf> serializeRequest(
    int32_t seqid,
    folly::StringPiece methodName,
    std::unique_ptr<folly::IOBuf> buf) {
  ProtocolWriter writer;
  auto messageBeginSizeUpperBound = writer.serializedMessageSize(methodName);
  folly::IOBufQueue queue;

  // If possible, serialize header into the headeroom of buf.
  if (!buf->isChained() && buf->headroom() >= messageBeginSizeUpperBound &&
      !buf->isSharedOne()) {
    // Store previous state of the buffer pointers and rewind it.
    auto startBuffer = buf->buffer();
    auto start = buf->data();
    auto origLen = buf->length();
    buf->trimEnd(origLen);
    buf->retreat(start - startBuffer);

    queue.append(std::move(buf), false);
    writer.setOutput(&queue);
    writer.writeMessageBegin(methodName, T_CALL, seqid);

    // Move the new data to come right before the old data and restore the
    // old tail pointer.
    buf = queue.move();
    buf->advance(start - buf->tail());
    buf->append(origLen);

    return buf;
  } else {
    auto messageBeginBuf = folly::IOBuf::create(messageBeginSizeUpperBound);
    queue.append(std::move(messageBeginBuf));
    writer.setOutput(&queue);
    writer.writeMessageBegin(methodName, T_CALL, seqid);
    queue.append(std::move(buf));
    return queue.move();
  }

  // We skip writeMessageEnd because for both Binary and Compact it's a noop.
}
} // namespace

LegacySerializedRequest::LegacySerializedRequest(
    uint16_t protocolId,
    int32_t seqid,
    folly::StringPiece methodName,
    SerializedRequest&& serializedRequest)
    : buffer([&] {
        switch (protocolId) {
          case protocol::T_BINARY_PROTOCOL:
            return serializeRequest<BinaryProtocolWriter>(
                seqid, methodName, std::move(serializedRequest.buffer));

          case protocol::T_COMPACT_PROTOCOL:
            return serializeRequest<CompactProtocolWriter>(
                seqid, methodName, std::move(serializedRequest.buffer));
          default:
            LOG(FATAL) << "Unsupported protocolId: " << protocolId;
        }
      }()) {}

LegacySerializedRequest::LegacySerializedRequest(
    uint16_t protocolId,
    folly::StringPiece methodName,
    SerializedRequest&& serializedRequest)
    : LegacySerializedRequest(
          protocolId,
          0,
          methodName,
          std::move(serializedRequest)) {}
} // namespace thrift
} // namespace apache
