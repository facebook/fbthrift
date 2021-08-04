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

#include <thrift/lib/cpp2/protocol/detail/index.h>

#include <memory>

#include <xxhash.h>

DEFINE_bool(
    thrift_enable_lazy_deserialization,
    true,
    "Whether to enable lazy deserialization");

namespace apache {
namespace thrift {
namespace detail {

int64_t xxh3_64bits(folly::io::Cursor cursor) {
  thread_local std::unique_ptr<XXH3_state_t, decltype(&XXH3_freeState)> state{
      XXH3_createState(), &XXH3_freeState};
  XXH3_64bits_reset(state.get());
  while (!cursor.isAtEnd()) {
    const auto buf = cursor.peekBytes();
    XXH3_64bits_update(state.get(), buf.data(), buf.size());
    cursor += buf.size();
  }
  return XXH3_64bits_digest(state.get());
}

} // namespace detail
} // namespace thrift
} // namespace apache
