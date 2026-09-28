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

#pragma once

#include <algorithm>
#include <cstring>
#include <memory>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncTransport.h>

namespace apache::thrift::fast_thrift::transport::test {

/**
 * Delivers data to the transport's current read callback. A movable callback
 * receives the whole chain through readBufferAvailable.
 *
 * Other callbacks receive chunks no larger than the room they offer, and a
 * chunk never crosses a node of the chain. The helper looks up the callback
 * after every chunk because it may detach itself. If it does, the helper drops
 * the remaining bytes. A real socket would keep them until reads resume, but
 * this in-memory transport has nowhere to keep them.
 *
 * The helper also drops the remaining bytes if the callback returns no buffer
 * or no room. A real transport reports a read error instead.
 */
inline void deliverReadData(
    folly::AsyncTransport* transport, std::unique_ptr<folly::IOBuf> data) {
  folly::AsyncTransport::ReadCallback* callback = transport->getReadCallback();
  if (callback == nullptr || !data) {
    return;
  }
  if (callback->isBufferMovable()) {
    callback->readBufferAvailable(std::move(data));
    return;
  }

  const folly::IOBuf* node = data.get();
  do {
    folly::ByteRange left{node->data(), node->length()};
    while (!left.empty()) {
      callback = transport->getReadCallback();
      if (callback == nullptr) {
        return;
      }
      void* buf = nullptr;
      size_t room = 0;
      callback->getReadBuffer(&buf, &room);
      if (buf == nullptr || room == 0) {
        return;
      }
      const size_t taken = std::min(room, left.size());
      std::memcpy(buf, left.data(), taken);
      left.advance(taken);
      callback->readDataAvailable(taken);
    }
    node = node->next();
  } while (node != data.get());
}

} // namespace apache::thrift::fast_thrift::transport::test
