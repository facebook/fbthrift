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

#include "thrift/conformance/cpp2/ThriftTypeInfo.h"

namespace apache::thrift::conformance {

void validateThriftTypeInfo(const ThriftTypeInfo& type) {
  validateUniversalType(*type.name_ref());
  for (const auto& alias : *type.aliases_ref()) {
    validateUniversalType(alias);
  }
  if (type.aliases_ref()->find(*type.name_ref()) != type.aliases_ref()->end()) {
    folly::throw_exception<std::invalid_argument>("alias matches name");
  }

  if (type.typeHashBytes_ref()) {
    validateTypeHashBytes(type.typeHashBytes_ref().value_unchecked());
  }
}

} // namespace apache::thrift::conformance
