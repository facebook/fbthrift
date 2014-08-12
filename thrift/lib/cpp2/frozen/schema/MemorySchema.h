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

namespace apache { namespace thrift { namespace frozen { namespace schema {

struct MemoryField {
  // Thrift field index
  int16_t id;

  // Index into MemorySchema::layouts
  int16_t layoutId;

  // field offset:
  //  < 0: -(bit offset)
  //  >= 0: byte offset
  int16_t offset;

  bool operator==(const MemoryField& other) const;
};

static_assert(sizeof(MemoryField) == 3 * sizeof(int16_t),
              "Memory Field is not padded.");

struct MemoryLayout {
  std::vector<MemoryField> fields;
  int32_t size;
  int16_t bits;

  bool operator==(const MemoryLayout& other) const;
};

struct MemorySchema {
  std::vector<MemoryLayout> layouts;

  bool operator==(const MemorySchema& other) const;
};

struct MemorySchemaHelper {
  // Add helper structures here to help minimize size of schema during
  // save() operations.
};

class Schema;
void convert(Schema&& schema, MemorySchema& memSchema);
void convert(const MemorySchema& memSchema, Schema& schema);

}}}}
