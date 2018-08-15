/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <string>
#include <utility>

namespace apache {
namespace thrift {
namespace compiler {

#if __cpp_lib_exchange_function || _LIBCPP_STD_VER > 11 || _MSC_VER

/* using override */ using std::exchange;

#else

//  mimic: std::exchange, C++14
//  from: http://en.cppreference.com/w/cpp/utility/exchange, CC-BY-SA
template <class T, class U = T>
T exchange(T& obj, U&& new_value) {
  T old_value = std::move(obj);
  obj = std::forward<U>(new_value);
  return old_value;
}

#endif

//  strip_left_margin
//
//  Looks for the least indented non-whitespace-only line and removes its amount
//  of leading whitespace from every line. Assumes leading whitespace is either
//  all spaces or all tabs.
//
//  The leading line is removed if it is whitespace-only. The trailing line is
//  kept but its content removed if it is whitespace-only.
//
//  Purpose: including a multiline string literal in source code, indented to
//  the level expected from context.
//
//  mimic: folly::stripLeftMargin
std::string strip_left_margin(std::string const& s);

} // namespace compiler
} // namespace thrift
} // namespace apache
