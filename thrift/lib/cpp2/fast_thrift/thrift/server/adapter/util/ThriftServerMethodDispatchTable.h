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
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <folly/Portability.h>
#include <folly/container/F14Map.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>

namespace apache::thrift::fast_thrift::channel_pipeline::detail {
class ContextImpl;
}

namespace apache::thrift::fast_thrift::thrift {

class ThriftServerAppAdapter;

/**
 * Immutable method dispatch shared by every connection serving one adapter
 * type. Entries contain unbound thunks: the connection-local adapter is
 * supplied only when a request is dispatched.
 */
class ThriftServerMethodDispatchTable final {
 public:
  using DispatchFn = channel_pipeline::Result (*)(
      ThriftServerAppAdapter*,
      channel_pipeline::detail::ContextImpl&,
      channel_pipeline::TypeErasedBox&&) noexcept;

  struct Method {
    std::string_view name;
    DispatchFn dispatch;
  };

  explicit ThriftServerMethodDispatchTable(
      std::initializer_list<Method> methods);

  explicit ThriftServerMethodDispatchTable(const std::vector<Method>& methods);
  FOLLY_ALWAYS_INLINE DispatchFn find(std::string_view name) const noexcept {
    const auto it = methods_.find(name);
    return it == methods_.end() ? nullptr : it->second;
  }

  template <typename F>
  void forEach(F&& f) const {
    for (const auto& [name, dispatch] : methods_) {
      f(std::string_view{name}, dispatch);
    }
  }

  std::size_t size() const noexcept { return methods_.size(); }

 private:
  folly::F14FastMap<std::string, DispatchFn> methods_;
};

} // namespace apache::thrift::fast_thrift::thrift
