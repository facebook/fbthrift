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

#include <thrift/lib/cpp2/type/Type.h>

#include <thrift/lib/cpp2/type/UniversalName.h>

namespace apache {
namespace thrift {
namespace type {

bool Type::isFull(const TypeNameUnion& typeName) {
  return typeName.getType() == TypeNameUnion::uri;
}

bool Type::isFull(const TypeId& typeId) {
  switch (typeId.getType()) {
    case TypeId::enumType:
      return isFull(*typeId.enumType_ref());
    case TypeId::structType:
      return isFull(*typeId.structType_ref());
    case TypeId::unionType:
      return isFull(*typeId.unionType_ref());
    case TypeId::exceptionType:
      return isFull(*typeId.exceptionType_ref());
    default:
      return true;
  }
}

bool Type::isFull(const TypeStruct& type) {
  for (const auto& param : *type.params()) {
    if (!isFull(param)) {
      return false;
    }
  }
  return isFull(*type.id());
}

void Type::checkName(const std::string& name) {
  validateUniversalName(name);
}

} // namespace type
} // namespace thrift
} // namespace apache
