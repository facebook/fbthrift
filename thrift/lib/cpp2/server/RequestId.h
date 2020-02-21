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
  friend class RequestsRegistry;

 public:
  RequestId(const RequestId&) = delete;
  RequestId& operator=(const RequestId&) = delete;
  RequestId(RequestId&&) = default;
  RequestId& operator=(RequestId&&) = default;

  std::string toString() const {
    return fmt::format("{:08x}.{:016x}", registry_, local_);
  }

  uint32_t getRegistryId() const {
    return registry_;
  }

  uint64_t getLocalId() const {
    return local_;
  }

 private:
  explicit RequestId(uint32_t registry, uint64_t local)
      : registry_(registry), local_(local) {}

  uint32_t registry_;
  uint64_t local_;
};

} // namespace thrift
} // namespace apache
