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

#include <thrift/conformance/cpp2/ThriftTypeInfo.h>

namespace apache::thrift::conformance {
using type::validateUniversalHashBytes;
using type::validateUniversalName;

void validateThriftTypeInfo(const ThriftTypeInfo& type) {
  validateUniversalName(*type.uri_ref());
  for (const auto& uri : *type.altUris_ref()) {
    validateUniversalName(uri);
  }
  if (type.altUris_ref()->find(*type.uri_ref()) != type.altUris_ref()->end()) {
    folly::throw_exception<std::invalid_argument>(
        "duplicate uri: " + *type.uri_ref());
  }

  if (type.typeHashBytes_ref()) {
    validateUniversalHashBytes(
        type.typeHashBytes_ref().value(), kMinTypeHashBytes);
  }
}

} // namespace apache::thrift::conformance
