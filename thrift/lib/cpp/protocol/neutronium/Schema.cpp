/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <thrift/lib/cpp/protocol/neutronium/Schema.h>
#include <thrift/lib/cpp/protocol/neutronium/Utils.h>
#include <folly/Conv.h>
#include <folly/String.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

namespace {

const char* kInternedAnnotation = "neutronium.intern";
const char* kFixedAnnotation = "neutronium.fixed";
const char* kTerminatedAnnotation = "neutronium.terminator";
const char* kPadAnnotation = "neutronium.pad";
const char* kStrictAnnotation = "neutronium.strict";

template <typename T>
bool getAnnotation(const reflection::StructField& rfield,
                   const char* annotation,
                   T& val) {
  auto pos = rfield.annotations.find(annotation);
  if (pos == rfield.annotations.end()) {
    return false;
  }

  val = folly::to<T>(pos->second);
  return true;
}

bool getBoolAnnotation(const reflection::StructField& rfield,
                       const char* annotation) {
  uint32_t val;
  return getAnnotation(rfield, annotation, val) && val != 0;
}

bool getCharAnnotation(const reflection::StructField& rfield,
                       const char* annotation,
                       char& val) {
  std::string valStr;
  if (!getAnnotation(rfield, annotation, valStr)) {
    return false;
  }
  valStr = folly::cUnescape<std::string>(folly::StringPiece(valStr));
  if (valStr.size() != 1) {
    throw std::runtime_error("Invalid annotation value");
  }

  val = valStr[0];
  return true;
}

}  // namespace

void StructField::setFlags(const reflection::DataType& rtype,
                           const reflection::StructField& rfield) {
  bool hasPadAnnotation = false;
  isRequired = rfield.isRequired;
  isInterned = getBoolAnnotation(rfield, kInternedAnnotation);
  isFixed =
    getAnnotation(rfield, kFixedAnnotation, fixedStringSize) &&
    fixedStringSize != 0;
  hasPadAnnotation = getCharAnnotation(rfield, kPadAnnotation, pad);
  isTerminated = getCharAnnotation(rfield, kTerminatedAnnotation, terminator);
  isStrictEnum = getBoolAnnotation(rfield, kStrictAnnotation);

  // Sanity checks
  auto t = reflection::getType(type);
  if (t == reflection::TYPE_STRING) {
    if (isInterned) {
      if (isTerminated || hasPadAnnotation || isFixed) {
        throw std::runtime_error("Invalid annotation on interned string");
      }
    } else if (isFixed) {
      if (isTerminated) {
        throw std::runtime_error("Invalid annotation on fixed-size string");
      }
    } else if (isTerminated) {
      if (hasPadAnnotation) {
        throw std::runtime_error("Invalid annotation on terminated string");
      }
    }
  } else if (t == reflection::TYPE_ENUM) {
    if (isInterned || isTerminated || hasPadAnnotation || fixedStringSize > 1) {
      throw std::runtime_error("Invalid string annotation on enum");
    }
  } else {
    // Hack: fixedStringSize is always set if the "fixed" annotation is set,
    // which is 1 for fixed-size (not varint) numbers
    if (isStrictEnum || isInterned || isTerminated || hasPadAnnotation ||
        fixedStringSize > 1) {
      throw std::runtime_error("Invalid string annotation on non-string");
    }
  }
}


Schema::Schema(const reflection::Schema& rschema) {
  add(rschema);
}

void Schema::add(const reflection::Schema& rschema) {
  for (auto& p : rschema.dataTypes) {
    add(rschema, p.first, p.second);
  }
}


int64_t Schema::fixedSizeForField(const StructField& field) const {
  auto t = reflection::getType(field.type);
  if (reflection::isBaseType(t) || t == reflection::TYPE_ENUM) {
    switch (t) {
    case reflection::TYPE_BOOL:
      return 0;  // handled separately
    case reflection::TYPE_BYTE:
      return 1;
    case reflection::TYPE_I16:
      return field.isFixed ? 2 : -1;
    case reflection::TYPE_I32:
      return field.isFixed ? 4 : -1;
    case reflection::TYPE_ENUM:
      return (field.isStrictEnum ? 0 :  // handled separately
              field.isFixed ? 4 : -1);
    case reflection::TYPE_I64:
      return field.isFixed ? 8 : -1;
    case reflection::TYPE_DOUBLE:
      return field.isFixed ? 8 : -1;
    case reflection::TYPE_STRING:
      return field.isFixed ? static_cast<int64_t>(field.fixedStringSize) : -1;
    default:
      break;
    }
    return -1;
  }
  return map_.at(field.type).fixedSize;
}


int64_t Schema::add(const reflection::Schema& rschema, int64_t type) {
  auto t = reflection::getType(type);
  if (reflection::isBaseType(t)) {
    return type;
  }

  add(rschema, type, rschema.dataTypes.at(type));
  return type;
}


void Schema::add(const reflection::Schema& rschema, int64_t type,
                 const reflection::DataType& rtype) {
  if (map_.count(type)) {  // already added
    return;
  }

  DataType dt;
  dt.fixedSize = 0;  // optimistic

  if (rtype.__isset.mapKeyType) {
    dt.fixedSize = -1;  // maps have variable size
    dt.mapKeyType = add(rschema, rtype.mapKeyType);
  }

  if (rtype.__isset.valueType) {
    dt.fixedSize = -1;  // maps, lists, sets have variable size
    dt.valueType = add(rschema, rtype.valueType);
  }

  if (rtype.__isset.enumValues) {
    dt.fixedSize = -1;  // ignored anyway
    dt.enumValues.reserve(rtype.enumValues.size());
    for (auto& p : rtype.enumValues) {
      dt.enumValues.insert(p.second);
    }
  }

  size_t requiredBitCount = 0;
  if (rtype.__isset.fields) {
    // struct
    for (auto& p : rtype.fields) {
      auto& rfield = p.second;

      StructField field;
      field.type = add(rschema, rfield.type);
      field.setFlags(rtype, rfield);

      if (!field.isRequired) {
        dt.optionalFields.insert(p.first);
        dt.fixedSize = -1;  // optional fields imply variable size
      } else {
        if (field.type == reflection::TYPE_BOOL) {
          ++requiredBitCount;
        } else if (reflection::getType(field.type) == reflection::TYPE_ENUM &&
                   field.isStrictEnum) {
          requiredBitCount += map_.at(field.type).enumValues.size();
        }
        if (dt.fixedSize != -1) {
          auto sz = fixedSizeForField(field);
          if (sz == -1) {
            dt.fixedSize = -1;
          } else {
            dt.fixedSize += sz;
          }
        }
      }
      dt.fields[p.first] = std::move(field);
    }

    if (dt.fixedSize != -1) {
      // Each bool needs one bit, round up to the nearest byte.
      dt.fixedSize += byteCount(requiredBitCount);
    }
  }

  map_[type] = std::move(dt);
}

namespace detail {

extern const TType typeToTType[] = {
  T_STOP,     // TYPE_VOID
  T_STRING,   // TYPE_STRING
  T_BOOL,     // TYPE_BOOL
  T_BYTE,     // TYPE_BYTE
  T_I16,      // TYPE_I16
  T_I32,      // TYPE_I32
  T_I64,      // TYPE_I64
  T_DOUBLE,   // TYPE_DOUBLE
  T_I32,      // TYPE_ENUM
  T_LIST,     // TYPE_LIST
  T_SET,      // TYPE_SET
  T_MAP,      // TYPE_MAP
  T_STRUCT,   // TYPE_STRUCT
  T_STOP,     // TYPE_SERVICE (invalid)
  T_STOP,     // TYPE_PROGRAM (invalid)
  T_STOP,     // TYPE_STREAM (invalid)
};

}  // namespace

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache
