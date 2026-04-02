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

#include <folly/ExceptionWrapper.h>
#include <folly/Expected.h>
#include <thrift/lib/cpp/TApplicationException.h>

#include <type_traits>
#include <fmt/core.h>

namespace apache::thrift::fast_thrift::thrift {

/**
 * Deserialize a Thrift response from a buffer.
 *
 * Sets up a ProtocolReader on the buffer and calls deserializeFn.
 * The deserializeFn returns Expected<T, exception_wrapper> where
 * errors include declared exceptions (with original types preserved).
 *
 * The try-catch only catches unexpected deserialization failures
 * (e.g. malformed data), wrapping them in TApplicationException.
 *
 * Used by both the fast client path (fbthrift_deserialize_*) and
 * the callback path (recv_wrapped_*).
 */
template <typename ProtocolReader, typename DeserializeFn>
std::invoke_result_t<DeserializeFn, ProtocolReader&> deserializeResponse(
    const folly::IOBuf* buf, DeserializeFn&& deserializeFn) {
  try {
    ProtocolReader reader;
    reader.setInput(buf);
    return deserializeFn(reader);
  } catch (...) {
    return folly::makeUnexpected(
        folly::make_exception_wrapper<apache::thrift::TApplicationException>(
            fmt::format(
                "Failed to deserialize response: {}",
                folly::exceptionStr(std::current_exception()))));
  }
}

} // namespace apache::thrift::fast_thrift::thrift
