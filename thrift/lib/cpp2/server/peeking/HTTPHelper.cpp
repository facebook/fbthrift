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

#include <thrift/lib/cpp2/server/peeking/HTTPHelper.h>

namespace apache {
namespace thrift {

bool HTTPHelper::looksLikeHTTP(const std::vector<uint8_t>& bytes) {
  /*
   * HTTP/2.0 requests start with the following sequence:
   *   Octal: 0x505249202a20485454502f322e300d0a0d0a534d0d0a0d0a
   *  String: "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
   *
   * For more, see: https://tools.ietf.org/html/rfc7540#section-3.5
   */
  if (bytes[0] != 0x50 || bytes[1] != 0x52 || bytes[2] != 0x49) {
    return false;
  }

  /*
   * HTTP requests start with the following sequence:
   *   Octal: "0x485454502f..."
   *  String: "HTTP/X.X"
   *
   * For more, see: https://tools.ietf.org/html/rfc2616#section-3
   */
  if (bytes[0] != 0x48 || bytes[1] != 0x54 || bytes[2] != 0x54) {
    return false;
  }

  return true;
}
}
}
