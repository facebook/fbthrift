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

#include <dlfcn.h>

#include <thrift/lib/cpp2/Flags.h>

namespace apache {
namespace thrift {
namespace detail {
namespace {

class FlagsBackendDummy : public detail::FlagsBackend {
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
} // namespace

FOLLY_ATTR_WEAK std::unique_ptr<detail::FlagsBackend> createFlagsBackend() {
  using CreateFlagsBackendPtr = std::unique_ptr<detail::FlagsBackend> (*)();
  static auto createFlagsBackendPtr =
      []() -> std::unique_ptr<detail::FlagsBackend> (*)() {
    CreateFlagsBackendPtr (*createFlagsBackendGetPtr)() =
#ifdef FOLLY_HAVE_LINUX_VDSO
        reinterpret_cast<CreateFlagsBackendPtr (*)()>(dlsym(
            RTLD_DEFAULT, "apache_thrift_detail_createFlagsBackend_get_ptr"));
#else
        nullptr;
#endif
    if (createFlagsBackendGetPtr) {
      return createFlagsBackendGetPtr();
    }
    return nullptr;
  }();
  if (createFlagsBackendPtr) {
    return createFlagsBackendPtr();
  }
  return {};
}

detail::FlagsBackend& getFlagsBackend() {
  static auto& flagsBackend = []() -> FlagsBackend& {
    if (auto flagsBackendPtr = createFlagsBackend()) {
      return *flagsBackendPtr.release();
    }
    return *(new FlagsBackendDummy);
  }();
  return flagsBackend;
}
} // namespace detail
} // namespace thrift
} // namespace apache
