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
#include <string_view>
#include <type_traits>

#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::detail {

// Declaration only; the definition is in tcc_struct_traits_impl.h, explicitly
// instantiated per struct in the generated *_types.cpp. Name-deserializing
// consumers (e.g. SimpleJSON) link against that instantiation.
template <typename T, typename = void>
struct TccStructTraits {
  static void translateFieldName(
      std::string_view _fname,
      int16_t& fid,
      apache::thrift::protocol::TType& _ftype);
};

// A class derived from a generated struct has the generated struct's fields,
// and only the generated struct's traits are instantiated. Specialized through
// the second parameter rather than a constraint: MSVC takes the out-of-line
// `TccStructTraits<T>::translateFieldName` definition for a constrained
// `TccStructTraits<T>` partial specialization.
template <typename T>
struct TccStructTraits<
    T,
    std::enable_if_t<!std::is_same_v<T, typename T::__fbthrift_cpp2_type>>>
    : TccStructTraits<typename T::__fbthrift_cpp2_type> {};

} // namespace apache::thrift::detail
