/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <vector>
#include <cstdint>
#include <thrift/lib/cpp/DistinctTable.h>


#define THRIFT_DECLARE_HASH(T)           \
  namespace std {                        \
  template <>                            \
  struct hash<T> {                       \
    size_t operator()(const T& x) const; \
  };                                     \
  }
#define THRIFT_IMPL_HASH(T)                      \
  namespace std {                                \
  size_t hash<T>::operator()(const T& x) const { \
    return x.hash();                             \
  }                                              \
  }

namespace apache { namespace thrift { namespace frozen { namespace schema {

class MemoryField;
class MemoryLayoutBase;
class MemoryLayout;
class MemorySchema;

}}}}

THRIFT_DECLARE_HASH(apache::thrift::frozen::schema::MemoryField);
THRIFT_DECLARE_HASH(apache::thrift::frozen::schema::MemoryLayoutBase);
THRIFT_DECLARE_HASH(apache::thrift::frozen::schema::MemoryLayout);
THRIFT_DECLARE_HASH(apache::thrift::frozen::schema::MemorySchema);

namespace apache { namespace thrift { namespace frozen { namespace schema {

// Trivially copyable, hashed bytewise.
struct MemoryField {
  // Thrift field index
  int16_t id;

  // Index into MemorySchema::layouts
  int16_t layoutId;

  // field offset:
  //  < 0: -(bit offset)
  //  >= 0: byte offset
  int16_t offset;

  size_t hash() const;
  bool operator==(const MemoryField& other) const;
};

static_assert(sizeof(MemoryField) == 3 * sizeof(int16_t),
              "Memory Field is not padded.");


struct MemoryLayoutBase {
  int32_t size;
  int16_t bits;

  size_t hash() const;
  bool operator==(const MemoryLayoutBase& other) const;
};

struct MemoryLayout : MemoryLayoutBase {
  std::vector<MemoryField> fields;

  size_t hash() const;
  bool operator==(const MemoryLayout& other) const;
};

struct MemorySchema {
  std::vector<MemoryLayout> layouts;
  // TODO(#4910107): Separate MemoryLayouts from MemoryLayoutBases
  int16_t rootLayout;

  size_t hash() const;
  bool operator==(const MemorySchema& other) const;
};

class MemorySchemaHelper {
  // Add helper structures here to help minimize size of schema during
  // save() operations.
 public:
  explicit MemorySchemaHelper(MemorySchema& schema)
      : layoutTable_(&schema.layouts) {}

  int16_t add(MemoryLayout&& layout);

 private:
  DistinctTable<MemoryLayout> layoutTable_;
};

class Schema;
void convert(Schema&& schema, MemorySchema& memSchema);
void convert(const MemorySchema& memSchema, Schema& schema);

}}}}
