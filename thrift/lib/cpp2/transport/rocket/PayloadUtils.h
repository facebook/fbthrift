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

#pragma once

#include <fmt/core.h>
#include <folly/Expected.h>
#include <folly/Try.h>
#include <folly/compression/Compression.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class RequestRpcMetadata;
class ResponseRpcMetadata;

namespace rocket {

template <class Metadata>
Payload makePayload(
    const Metadata& metadata,
    std::unique_ptr<folly::IOBuf> data);

extern template Payload makePayload<>(
    const RequestRpcMetadata&,
    std::unique_ptr<folly::IOBuf> data);
extern template Payload makePayload<>(
    const ResponseRpcMetadata&,
    std::unique_ptr<folly::IOBuf> data);

template <typename T>
size_t unpackCompact(T& output, const folly::IOBuf* buffer) {
  CompactProtocolReader reader;
  reader.setInput(buffer);
  output.read(&reader);
  return reader.getCursorPosition();
}

template <typename T>
size_t unpackCompact(T& output, std::unique_ptr<folly::IOBuf> buffer) {
  return unpackCompact(output, buffer.get());
}

template <>
inline size_t unpackCompact(
    std::unique_ptr<folly::IOBuf>& output,
    std::unique_ptr<folly::IOBuf> buffer) {
  output = std::move(buffer);
  return 0;
}

/**
 * Helper method to uncompress the request on server side.
 */
folly::Expected<std::unique_ptr<folly::IOBuf>, std::string> uncompressPayload(
    CompressionAlgorithm compression,
    std::unique_ptr<folly::IOBuf> data);

template <class T>
folly::Try<T> unpack(rocket::Payload&& payload) {
  return folly::makeTryWith([&] {
    T t{{}, {}};
    if (payload.hasNonemptyMetadata()) {
      if (unpackCompact(t.metadata, payload.buffer()) >
          payload.metadataSize()) {
        folly::throw_exception<std::out_of_range>("underflow");
      }
    }

    auto data = std::move(payload).data();
    // uncompress the payload if needed
    if (auto compress = t.metadata.compression_ref()) {
      auto result = uncompressPayload(*compress, std::move(data));
      if (!result) {
        folly::throw_exception<TApplicationException>(
            TApplicationException::INVALID_TRANSFORM,
            fmt::format(
                "decompression failure: {}", std::move(result.error())));
      }
      data = std::move(result.value());
    }

    unpackCompact(t.payload, std::move(data));

    return t;
  });
}

template <typename T>
std::unique_ptr<folly::IOBuf> packCompact(T&& data) {
  CompactProtocolWriter writer;
  folly::IOBufQueue queue;
  writer.setOutput(&queue);
  data.write(&writer);
  return queue.move();
}

template <>
inline std::unique_ptr<folly::IOBuf> packCompact(
    std::unique_ptr<folly::IOBuf>&& data) {
  return std::move(data);
}

template <class T>
folly::Try<rocket::Payload> pack(T&& payload) {
  return folly::makeTryWith([&] {
    return rocket::Payload::makeFromMetadataAndData(
        packCompact(std::forward<T>(payload).metadata),
        packCompact(std::forward<T>(payload).payload));
  });
}

/**
 * Helper method to compress the payload before sending to the remote endpoint.
 */
template <class Metadata>
void compressPayload(
    Metadata& metadata,
    std::unique_ptr<folly::IOBuf>& data,
    CompressionAlgorithm compression);

extern template void compressPayload<>(
    RequestRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& data,
    CompressionAlgorithm compression);

extern template void compressPayload<>(
    ResponseRpcMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& data,
    CompressionAlgorithm compression);

extern template void compressPayload<>(
    StreamPayloadMetadata& metadata,
    std::unique_ptr<folly::IOBuf>& data,
    CompressionAlgorithm compression);
} // namespace rocket
} // namespace thrift
} // namespace apache
