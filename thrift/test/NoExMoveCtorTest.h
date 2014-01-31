/*
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
#include <unordered_map>

namespace thrift { namespace test { namespace noexcept_move_ctor {

// The move ctor is not "noexcept" in gcc 4.8
typedef std::unordered_map<std::string, std::string> s2sumap;

// A type that may throw in move ctor.
class ThrowCtorType : public std::string {
 public:
  ThrowCtorType() {}

  // the move ctor is not annotated with "noexcept"
  ThrowCtorType(ThrowCtorType&& other)
    : std::string(std::move(other)) {
    throw (1);
  }

  explicit ThrowCtorType(std::string&& other)
    : std::string(std::move(other)) {
  }

  ThrowCtorType& operator=(const ThrowCtorType& other) = default;
  ThrowCtorType& operator=(const std::string& other) { return *this; }
};

}}} // namespaces
