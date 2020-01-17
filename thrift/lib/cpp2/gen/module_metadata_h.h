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

namespace apache::thrift {
namespace metadata {
class ThriftMetadata;
}

namespace detail::metadata {
template <typename T>
class GeneratedEnumMetadata {
 public:
  static void genMetadata(::apache::thrift::metadata::ThriftMetadata& metadata);
};

template <typename T>
class GeneratedStructMetadata {
 public:
  static void genMetadata(::apache::thrift::metadata::ThriftMetadata& metadata);
};

template <typename T>
class GeneratedExceptionMetadata {
 public:
  static void genMetadata(::apache::thrift::metadata::ThriftMetadata& metadata);
};
} // namespace detail::metadata
} // namespace apache::thrift
