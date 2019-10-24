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

#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

#include <cstdint>
#include <exception>
#include <string>

#include <folly/Conv.h>

namespace apache {
namespace thrift {
namespace rocket {
namespace detail {

[[noreturn]] void throwUnexpectedFrameType(uint8_t frameType) {
  throw std::runtime_error(
      folly::to<std::string>("Parsed unexpected frame type: ", frameType));
}

} // namespace detail
} // namespace rocket
} // namespace thrift
} // namespace apache
