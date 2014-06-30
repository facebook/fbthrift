/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef THRIFT_UTIL_ENUMUTILS_H_
#define THRIFT_UTIL_ENUMUTILS_H_ 1

#include <cstring>
#include <string>
#include <thrift/lib/cpp/Thrift.h>

namespace apache { namespace thrift {

namespace util {

/**
 * Parses an enum name to the enum type
 */
template<typename String, typename EnumType>
bool tryParseEnum(const String& name, EnumType* out) {
  return TEnumTraits<EnumType>::findValue(name.c_str(), out);
}

template<typename EnumType>
bool tryParseEnum(const char* name, EnumType* out) {
  return TEnumTraits<EnumType>::findValue(name, out);
}

/**
 * Returns the human-readable name for an Enum type.
 */
template<typename EnumType>
const char* enumName(EnumType value,
                     const char* defaultName = nullptr) {
  const char* name = TEnumTraits<EnumType>::findName(value);
  if (!name) return defaultName;
  return name;
}

/**
 * Returns the human-readable name for an Enum type in short form.
 * (That is, always returns VALUE instead of Foo::VALUE, even for
 * "strict" enums)
 * WARNING! By default it returns nullptr if the value is not in enum.
 */
template <typename EnumType>
const char* shortEnumName(EnumType value,
                          const char* defaultName = nullptr) {
  const char* name = TEnumTraits<EnumType>::findName(value);
  if (!name) return defaultName;
  const char* p = strrchr(name, ':');
  if (p) name = p + 1;
  return name;
}

/**
 * Same as shortEnumName but returns the integer value converted to string
 * if it is not in enum, to avoid returning nullptr.
 */
template <typename EnumType>
std::string shortEnumNameSafe(EnumType value) {
  const char* name = shortEnumName(value);
  return name ? name : std::to_string(static_cast<int32_t>(value));
}

}}} // apache::thrift::util

#endif // THRIFT_UTIL_ENUMUTILS_H_ 1
