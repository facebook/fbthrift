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

#include <thrift/lib/cpp2/Flags.h>

#include <thrift/lib/cpp2/PluggableFunction.h>

namespace apache {
namespace thrift {
namespace detail {
namespace {

class FlagsBackendDummy : public apache::thrift::detail::FlagsBackend {
 public:
  folly::observer::Observer<folly::Optional<bool>> getFlagObserverBool(
      folly::StringPiece) override {
    return folly::observer::makeObserver(
        []() -> folly::Optional<bool> { return folly::none; });
  }
  folly::observer::Observer<folly::Optional<int64_t>> getFlagObserverInt64(
      folly::StringPiece) override {
    return folly::observer::makeObserver(
        []() -> folly::Optional<int64_t> { return folly::none; });
  }
};

THRIFT_PLUGGABLE_FUNC_REGISTER(
    std::unique_ptr<FlagsBackend>, createFlagsBackend) {
  return {};
}
} // namespace

apache::thrift::detail::FlagsBackend& getFlagsBackend() {
  static auto& obj = *[] {
    auto backend = THRIFT_PLUGGABLE_FUNC(createFlagsBackend)();
    if (!backend) {
      backend = std::make_unique<FlagsBackendDummy>();
    }
    return backend.release();
  }();
  return obj;
}
} // namespace detail
} // namespace thrift
} // namespace apache
