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

#include <string_view>
#include <type_traits>

#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>

namespace apache::thrift {
/**
 * A class encapsulating utilities that are useful for custom implementations of
 * AsyncProcessor or AsyncProcessorFactory.
 */
class AsyncProcessorHelper {
  AsyncProcessorHelper() = delete;

 public:
  static void sendUnknownMethodError(
      ResponseChannelRequest::UniquePtr req, std::string_view methodName);

  template <
      typename Metadata,
      typename = std::enable_if_t<
          std::is_base_of_v<AsyncProcessorFactory::MethodMetadata, Metadata>>>
  static const Metadata* metadataOfType(
      const AsyncProcessorFactory::MethodMetadata& methodMetadata) {
    // If Metadata is final so we know that dynamic_cast will be a simple vtable
    // comparison. i.e. we know at compile-time that methodMetadata cannot be of
    // a more-derived object type.
    //
    // This should be a trivial operation on any platform where RTTI is based on
    // vtables. Refer to the Itanium C++ ABI for more details:
    //   https://itanium-cxx-abi.github.io/cxx-abi/abi.html#rtti
    // ARM64's "Generic C++ ABI" also follows the Itanium ABI:
    //   https://github.com/ARM-software/abi-aa/blob/8249954f46b725c12d07639123e55a60a0264a3b/cppabi64/cppabi64.rst#the-generic-c-abi
    //
    // Semantically, this is equivalent to comparing the type_info's directly.
    // However, in both libc++ [0] and libstdc++ [1], type_info needs to support
    // object from across shared library boundaries (possibly not sharing vtable
    // pointers), which is implemented using strcmp. The trade-off is that we
    // fail if the metadata object being passed is created in a shared library
    // opened with the RTLD_LOCAL flag. Thankfully, that's a case we don't
    // really care about.
    //   [0]https://github.com/llvm/llvm-project/blob/297088d1add70cae554c8f96dde3a97a3e8d56a5/libcxx/src/typeinfo.cpp#L14-L18
    //   [1]https://github.com/gcc-mirror/gcc/blob/924e02553af64b10c485711d635c0bc0265a8743/libstdc%2B%2B-v3/libsupc%2B%2B/typeinfo#L102-L135
    //
    // Note that it's not strictly necessary that the type is final for this
    // check to work. However, this function will be called in the hot path
    // for every request so we should encourage performant checks. In almost all
    // cases, it should be possible to make the Metadata type final.
    static_assert(std::is_final_v<Metadata>);
    return dynamic_cast<const Metadata*>(&methodMetadata);
  }

  static bool isWildcardMethodMetadata(
      const AsyncProcessorFactory::MethodMetadata& methodMetadata) {
    bool isWildcard =
        &methodMetadata == &AsyncProcessorFactory::kWildcardMethodMetadata;
    if constexpr (folly::kIsDebug) {
      if (!isWildcard) {
        DCHECK(
            dynamic_cast<const AsyncProcessorFactory::WildcardMethodMetadata*>(
                &methodMetadata) == nullptr)
            << "Detected WildcardMethodMetadata object that is distinct from the singleton, AsyncProcessorFactory::kWildcardMethodMetadata.";
      }
    }
    return isWildcard;
  }
};

} // namespace apache::thrift
