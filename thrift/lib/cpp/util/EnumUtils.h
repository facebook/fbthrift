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

// Note: Below methods don't work for class scoped enums

#ifndef THRIFT_UTIL_ENUMUTILS_H_
#define THRIFT_UTIL_ENUMUTILS_H_ 1

#include <cstring>

#include <folly/Conv.h>

#include <thrift/lib/cpp/Thrift.h>

namespace apache {
namespace thrift {

namespace util {

/**
 * Parses an enum name to the enum type
 */
template <typename String, typename EnumType>
bool tryParseEnum(const String& name, EnumType* out) {
  return TEnumTraits<EnumType>::findValue(name.c_str(), out);
}

template <typename EnumType>
bool tryParseEnum(const char* name, EnumType* out) {
  return TEnumTraits<EnumType>::findValue(name, out);
}

/**
 * Returns the human-readable name for an Enum type.
 * WARNING! By default it returns nullptr if the value is not in enum.
 */
template <typename EnumType>
const char* enumName(EnumType value, const char* defaultName = nullptr) {
  const char* name = TEnumTraits<EnumType>::findName(value);
  if (!name)
    return defaultName;
  return name;
}

/**
 * Same as enumName but returns the integer value converted to string
 * if it is not in enum, to avoid returning nullptr.
 */
template <typename EnumType>
std::string enumNameSafe(EnumType value) {
  const char* name = enumName(value);
  return name ? name : folly::to<std::string>(static_cast<int32_t>(value));
}

} // namespace util
} // namespace thrift
} // namespace apache

#endif // THRIFT_UTIL_ENUMUTILS_H_ 1
