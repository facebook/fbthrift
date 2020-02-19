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
namespace metadata {
class ThriftMetadata;
}

namespace detail {
namespace md {
template <typename T>
class EnumMetadata {
 public:
  static void gen(::apache::thrift::metadata::ThriftMetadata&) {}
};
template <typename T>
class StructMetadata {
 public:
  static void gen(::apache::thrift::metadata::ThriftMetadata&) {}
};

template <typename T>
class ExceptionMetadata {
 public:
  static void gen(::apache::thrift::metadata::ThriftMetadata&) {}
};

template <typename T>
class ServiceMetadata {
 public:
  static void gen(
      ::apache::thrift::metadata::ThriftMetadata&,
      ::apache::thrift::metadata::ThriftServiceContext&) {}
};
} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache
