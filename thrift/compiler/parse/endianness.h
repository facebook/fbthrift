/*
 * Copyright 2016 Facebook, Inc.
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

namespace apache { namespace thrift { namespace compiler {

inline constexpr bool kIsBigEndian() {
  #ifdef _WIN32
    return false;
  #else
    return __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__;
  #endif
}

/*
 * Use system specific byte swap functions
 */
inline uint64_t bswap(const uint64_t b) {
  #ifdef _WIN32
    return _byteswap_uint64(b);
  #else
    return __builtin_bswap64(b);
  #endif
}

/*
 * Convert a number from any platform to little Endian form.
 */
inline uint64_t bswap_host_to_little_endian(const uint64_t b) {
  return kIsBigEndian() ? bswap(b) : b;
}

}}}
