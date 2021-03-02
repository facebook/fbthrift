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

#include <array>

#include <folly/Indestructible.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>

namespace apache {
namespace thrift {
namespace detail {
namespace md {

using ThriftMetadata = ::apache::thrift::metadata::ThriftMetadata;
using ThriftServiceContext = ::apache::thrift::metadata::ThriftServiceContext;
using ThriftService = ::apache::thrift::metadata::ThriftService;

class EmptyMetadata {
 protected:
  FOLLY_ERASE static void gen(ThriftMetadata&) {}
};

class EmptyServiceMetadata {
 protected:
  FOLLY_ERASE static void gen(ThriftMetadata&, ThriftServiceContext&) {}
};

template <typename T>
class EnumMetadata {
  static_assert(!sizeof(T), "invalid use of base template");
};

template <typename T>
class StructMetadata {
  static_assert(!sizeof(T), "invalid use of base template");
};

template <typename T>
class ExceptionMetadata {
  static_assert(!sizeof(T), "invalid use of base template");
};

template <typename T>
class ServiceMetadata {
  static_assert(!sizeof(T), "invalid use of base template");
};

/**
 * Get ThriftMetadata of given thrift structure. If no_metadata option is
 * enabled, return empty data.
 *
 * @tparam T thrift structure
 *
 * @return ThriftStruct (https://git.io/JJQpW)
 */
template <class T>
const auto& get_struct_metadata() {
  static const folly::Indestructible<metadata::ThriftStruct> data =
      StructMetadata<T>::gen(std::array<ThriftMetadata, 1>()[0]);
  return *data;
}

} // namespace md
} // namespace detail

using detail::md::get_struct_metadata;
} // namespace thrift
} // namespace apache
