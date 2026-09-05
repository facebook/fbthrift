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

#include <cstdint>
#include <memory>

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/common/CompactVariant.h>

namespace apache::thrift::fast_thrift::thrift::stream {

// Messages for an established stream / sink / bidi exchange.
// Direction-agnostic: the same types flow in both directions (direction is the
// pipeline's read/write axis) and serve both sides.

// A demand grant: "send N more items." Not tied to the wire's 31-bit REQUEST_N.
struct RequestN {
  uint64_t n{0};
};

// A stream data item. Holds the item bytes; serialization to a rocket PAYLOAD
// frame is a downstream concern.
struct Payload {
  std::unique_ptr<folly::IOBuf> data{nullptr};
};

// The frames that flow on an established stream today. Grows as more frame
// kinds (cancel, error, ...) come online. CompactVariant keeps the
// discriminator to a single byte so the message stays inline in TypeErasedBox.
using StreamMessageVariant = CompactVariant<RequestN, Payload>;

struct ThriftStreamMessage {
  StreamMessageVariant payload;
};

static_assert(
    channel_pipeline::TypeErasedBox::fits_inline<ThriftStreamMessage>());

} // namespace apache::thrift::fast_thrift::thrift::stream
