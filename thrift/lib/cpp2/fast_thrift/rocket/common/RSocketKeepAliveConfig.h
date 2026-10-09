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

namespace apache::thrift::fast_thrift::rocket {

inline constexpr uint32_t kRSocketMaxKeepAliveTime = (1u << 31) - 1;
inline constexpr uint32_t kRSocketMaxLifetime = (1u << 31) - 1;

struct RSocketKeepAliveConfig {
  uint32_t intervalMs{kRSocketMaxKeepAliveTime};
  uint32_t maxLifetimeMs{kRSocketMaxLifetime};

  [[nodiscard]] constexpr bool enabled() const noexcept {
    return intervalMs != kRSocketMaxKeepAliveTime ||
        maxLifetimeMs != kRSocketMaxLifetime;
  }

  [[nodiscard]] constexpr bool valid() const noexcept {
    return intervalMs > 0 && intervalMs <= kRSocketMaxKeepAliveTime &&
        maxLifetimeMs > 0 && maxLifetimeMs <= kRSocketMaxLifetime;
  }
};

} // namespace apache::thrift::fast_thrift::rocket
