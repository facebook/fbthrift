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

class EmptyStructMetadata {
 protected:
  FOLLY_ERASE static const ::apache::thrift::metadata::ThriftStruct gen(
      ThriftMetadata&) {
    return {};
  }
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

} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache
