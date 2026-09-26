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

#include <concepts>
#include <cstddef>

namespace apache::thrift::fast_thrift::channel_pipeline::pipeline_storage {

// Heap storage is compacted when build() finalizes the pipeline.
struct Large {};

// Construction storage that selects the smallest finalized storage flavor
// from the actual handler count.
struct Automatic {};

// Inline storage shares the pipeline allocation and is sized to the final
// handler count. MaxHandlers is a hard limit; it never falls back to heap.
template <std::size_t MaxHandlers>
struct Inline {
  static_assert(MaxHandlers > 0, "inline pipeline capacity must be positive");
  static constexpr std::size_t maxHandlers = MaxHandlers;
};

using Small = Inline<8>;
using Medium = Inline<16>;

template <std::size_t MaxHandlers>
using Custom = Inline<MaxHandlers>;

template <typename Storage>
inline constexpr bool kIsInline = false;

template <std::size_t MaxHandlers>
inline constexpr bool kIsInline<Inline<MaxHandlers>> = true;

template <typename Storage>
concept Valid = std::same_as<Storage, Large> ||
    std::same_as<Storage, Automatic> || kIsInline<Storage>;

} // namespace apache::thrift::fast_thrift::channel_pipeline::pipeline_storage
