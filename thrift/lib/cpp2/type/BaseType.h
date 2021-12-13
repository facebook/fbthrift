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

#include <stdexcept>
#include <string>

#include <folly/lang/Exception.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::type {

enum class BaseType {
  Void = 0,

  // Integer types.
  Bool = 1,
  Byte = 2,
  I16 = 3,
  I32 = 4,
  I64 = 5,

  // Floating point types.
  Float = 6,
  Double = 7,

  // String types.
  String = 8,
  Binary = 9,

  // Enum type class.
  Enum = 10,

  // Structured type classes.
  Struct = 11,
  Union = 12,
  Exception = 13,

  // Container type classes.
  List = 14,
  Set = 15,
  Map = 16
};

const char* getBaseTypeName(BaseType type) noexcept;

constexpr inline protocol::TType toTType(BaseType type) {
  using protocol::TType;
  switch (type) {
    case BaseType::Void:
      return TType::T_VOID;
    case BaseType::Bool:
      return TType::T_BOOL;
    case BaseType::Byte:
      return TType::T_BYTE;
    case BaseType::I16:
      return TType::T_I16;
    case BaseType::Enum:
    case BaseType::I32:
      return TType::T_I32;
    case BaseType::I64:
      return TType::T_I64;
    case BaseType::Double:
      return TType::T_DOUBLE;
    case BaseType::Float:
      return TType::T_FLOAT;
    case BaseType::String:
      return TType::T_UTF8;
    case BaseType::Binary:
      return TType::T_STRING;

    case BaseType::List:
      return TType::T_LIST;
    case BaseType::Set:
      return TType::T_SET;
    case BaseType::Map:
      return TType::T_MAP;

    case BaseType::Struct:
      return TType::T_STRUCT;
    case BaseType::Union:
      return TType::T_STRUCT;
    case BaseType::Exception:
      return TType::T_STRUCT;
    default:
      folly::throw_exception<std::invalid_argument>(
          "Unsupported conversion from: " + std::to_string((int)type));
  }
}

constexpr inline BaseType toBaseType(protocol::TType type) {
  using protocol::TType;
  switch (type) {
    case TType::T_BOOL:
      return BaseType::Bool;
    case TType::T_BYTE:
      return BaseType::Byte;
    case TType::T_I16:
      return BaseType::I16;
    case TType::T_I32:
      return BaseType::I32;
    case TType::T_I64:
      return BaseType::I64;
    case TType::T_DOUBLE:
      return BaseType::Double;
    case TType::T_FLOAT:
      return BaseType::Float;
    case TType::T_LIST:
      return BaseType::List;
    case TType::T_MAP:
      return BaseType::Map;
    case TType::T_SET:
      return BaseType::Set;
    case TType::T_STRING:
      return BaseType::Binary;
    case TType::T_STRUCT:
      return BaseType::Struct;
    case TType::T_UTF8:
      return BaseType::String;
    case TType::T_VOID:
      return BaseType::Void;
    default:
      folly::throw_exception<std::invalid_argument>(
          "Unsupported conversion from: " + std::to_string(type));
  }
}
} // namespace apache::thrift::type
