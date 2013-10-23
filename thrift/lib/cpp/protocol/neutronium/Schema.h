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

#ifndef THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_H_
#define THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_H_

#include <vector>
#include "boost/interprocess/containers/flat_map.hpp"
#include "boost/interprocess/containers/flat_set.hpp"
#include "thrift/lib/cpp/protocol/TProtocol.h"
#include "thrift/lib/cpp/Reflection.h"
#include "external/glog/logging.h"
#include "folly/Bits.h"
#include "thrift/lib/cpp/protocol/neutronium/Utils.h"

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

struct StructField {
  StructField()
    : type(0),
      isRequired(false),
      isInterned(false),
      isFixed(false),
      isTerminated(false),
      fixedStringSize(0),
      pad('\0'),
      terminator('\0') {
  }

  void setFlags(const reflection::DataType& rtype,
                const reflection::StructField& rfield);

  void clear() {
    *this = StructField();
  }

  // from reflection::StructField
  int64_t type;
  bool isRequired : 1;
  bool isInterned : 1;

  // numbers: not varint-encoded
  // strings: fixed size, fixedStringSize bytes, padded with 'pad'
  bool isFixed : 1;

  // strings: terminated with 'terminator' which may not occur in the string
  bool isTerminated : 1;

  // enums: strict (bitfield) encoding
  bool isStrictEnum : 1;

  uint32_t fixedStringSize;
  char pad;
  char terminator;
};

struct DataType {
  DataType() : mapKeyType(0), valueType(0), fixedSize(-1) { }

  // from reflection::StructField
  boost::container::flat_map<int16_t, StructField> fields;
  int64_t mapKeyType;
  int64_t valueType;
  // generated
  int64_t fixedSize;  // -1 = not fixed size
  boost::container::flat_set<int16_t> optionalFields;
  boost::container::flat_set<int32_t> enumValues;

  // Minimum number of bits needed to represent all enum values
  // 0 if there's only one possible value -- there's nothing to encode!
  size_t enumBits() const {
    return enumValues.empty() ? 0 : folly::findLastSet(enumValues.size() - 1);
  }
};

class Schema {
 public:
  typedef std::unordered_map<int64_t, DataType> Map;
  Schema() { }
  explicit Schema(const reflection::Schema& rschema);
  void add(const reflection::Schema& rschema);

  const Map& map() const { return map_; }

 private:
  int64_t fixedSizeForField(const StructField& field) const;

  int64_t add(const reflection::Schema& rschema, int64_t type);
  void add(const reflection::Schema& rschema, int64_t type,
           const reflection::DataType& dt);
  Map map_;
};

TType toTType(reflection::Type t);

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache

#define THRIFT_INCLUDE_SCHEMA_INL
#include "thrift/lib/cpp/protocol/neutronium/Schema-inl.h"
#undef  THRIFT_INCLUDE_SCHEMA_INL

#endif /* THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_SCHEMA_H_ */
