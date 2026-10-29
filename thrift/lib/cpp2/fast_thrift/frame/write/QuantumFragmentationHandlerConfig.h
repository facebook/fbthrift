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

#include <cstddef>

namespace apache::thrift::fast_thrift::frame::write {

/**
 * This is the largest `streamQuantum` that a handler accepts.
 *
 * Rocket's `kMaxFragmentedPayloadSize` has the same value. Both limits count
 * metadata plus data. RSocket stores the frame length in three bytes. The 512
 * bytes below `0xffffff` cover the frame header, which uses at most 13 bytes.
 */
inline constexpr size_t kMaxStreamQuantum = 0xffffff - 512;

/**
 * Limits how much a fragmentation handler sends at once when it defers data
 * frames to later EventBase callbacks.
 *
 * `streamQuantum` counts metadata plus data. It is the most that one fragment
 * can carry, so a frame that fits in one quantum goes out whole. If metadata
 * reaches or exceeds the quantum, the first fragment carries all metadata and
 * no data. The frame header is outside this count.
 *
 * `callbackBudget` counts data only. Metadata is outside this count. After a
 * callback has sent this many data bytes, it starts no new fragment. The
 * fragment that crosses the budget is still sent whole.
 */
struct QuantumFragmentationHandlerConfig {
  // The mixed-payload benchmark uses 64 KiB fragments.
  size_t streamQuantum{64 * 1024};

  // The default allows 256 full 64 KiB fragments in one callback.
  size_t callbackBudget{16 * 1024 * 1024};

  /**
   * Returns false for a `streamQuantum` of 0 or above `kMaxStreamQuantum`, or
   * for a `callbackBudget` of 0. With such a config, the handler returns Error
   * and drops each data frame that it would otherwise queue.
   */
  [[nodiscard]] constexpr bool isValid() const noexcept {
    return streamQuantum != 0 && streamQuantum <= kMaxStreamQuantum &&
        callbackBudget != 0;
  }
};

} // namespace apache::thrift::fast_thrift::frame::write
