/*
 * Copyright 2019-present Facebook, Inc.
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

#include <folly/Try.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>

namespace apache {
namespace thrift {
namespace rocket {
template <class T>
folly::Try<T> unpack(rocket::Payload&& payload) {
  return folly::makeTryWith([&] {
    T t{nullptr, {}};
    if (payload.hasNonemptyMetadata()) {
      CompactProtocolReader reader;
      reader.setInput(payload.buffer());
      t.metadata.read(&reader);
      if (reader.getCursorPosition() > payload.metadataSize()) {
        folly::throw_exception<std::out_of_range>("underflow");
      }
    }
    t.payload = std::move(payload).data();
    return t;
  });
}

template <class T>
folly::Try<rocket::Payload> pack(T&& payload) {
  return folly::makeTryWith([&] {
    CompactProtocolWriter writer;
    folly::IOBufQueue queue;
    writer.setOutput(&queue);
    payload.metadata.write(&writer);
    return rocket::Payload::makeFromMetadataAndData(
        queue.move(), std::move(payload.payload));
  });
}

} // namespace rocket
} // namespace thrift
} // namespace apache
