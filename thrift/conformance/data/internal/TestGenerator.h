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

#include <initializer_list>
#include <set>

#include <boost/mp11.hpp>

#include <thrift/conformance/cpp2/Protocol.h>

namespace apache::thrift::conformance::data::detail {

using PrimaryTypeTags = boost::mp11::mp_list<
    type::bool_t,
    type::byte_t,
    type::i16_t,
    type::i32_t,
    type::float_t,
    type::double_t,
    type::string_t,
    type::binary_t>;

template <typename C>
std::set<Protocol> toProtocols(const C& protocolCtorArgs) {
  std::set<Protocol> result;
  for (const auto& arg : protocolCtorArgs) {
    result.emplace(arg);
  }
  return result;
}

constexpr std::initializer_list<StandardProtocol> kDefaultProtocols = {
    StandardProtocol::Binary, StandardProtocol::Compact};

} // namespace apache::thrift::conformance::data::detail
