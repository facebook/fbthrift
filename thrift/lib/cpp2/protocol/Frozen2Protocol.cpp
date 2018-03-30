/*
 * Copyright 2004-present Facebook, Inc.
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

#include "Frozen2Protocol.h"

namespace apache {
namespace thrift {

[[noreturn]] void Frozen2ProtocolReader::throwBadProtocolId(int32_t sz) {
  throw TProtocolException(
      TProtocolException::BAD_VERSION,
      folly::to<std::string>("Bad protocol id, sz= ", sz));
}

    [[noreturn]] void Frozen2ProtocolReader::throwBadProtocolVersion(
        int32_t sz) {
  throw TProtocolException(
      TProtocolException::BAD_VERSION,
      folly::to<std::string>("Bad protocol version, sz= ", sz));
}
} // namespace thrift
} // namespace apache
