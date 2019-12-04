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

#pragma once
#include <fmt/core.h>

namespace apache {
namespace thrift {

class RequestId {
 public:
  static RequestId gen();

  RequestId(const RequestId&) = delete;
  RequestId& operator=(const RequestId&) = delete;
  RequestId(RequestId&&) = default;
  RequestId& operator=(RequestId&&) = default;

  std::string toString() const {
    return fmt::format("{:016x}", val_);
  }

  uint64_t getVal() const {
    return val_;
  }

 private:
  explicit RequestId(uint64_t val) : val_(val) {}

  static const uint64_t maxVal;
  uint64_t val_;
};

} // namespace thrift
} // namespace apache
