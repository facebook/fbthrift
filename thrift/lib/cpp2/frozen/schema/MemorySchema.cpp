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
#include <thrift/lib/cpp2/frozen/schema/MemorySchema.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>

namespace apache { namespace thrift { namespace frozen { namespace schema {

bool MemoryField::operator==(const MemoryField& other) const {
  return id == other.id && layoutId == other.layoutId && offset == other.offset;
}

bool MemoryLayout::operator==(const MemoryLayout& other) const {
  return bits == other.bits && size == other.size && fields == other.fields;
}

bool MemorySchema::operator==(const MemorySchema& other) const {
  return layouts == other.layouts;
}

void convert(Schema&& schema, MemorySchema& memSchema) {
  if (schema.layouts.size() == 0) {
    return;
  }

  memSchema.layouts.resize(schema.layouts.size());

  for(const auto& layoutKvp : schema.layouts) {
    const auto id = layoutKvp.first;
    const auto& layout = layoutKvp.second;

    // Note: This will throw if there are any id >=
    // schema.layouts.size().
    auto& memLayout = memSchema.layouts.at(id);

    memLayout.size = layout.size;
    memLayout.bits = layout.bits;

    memLayout.fields.reserve(layout.fields.size());

    for(const auto& fieldKvp : layout.fields) {
      MemoryField memField;
      const auto& fieldId = fieldKvp.first;
      const auto& field = fieldKvp.second;

      memField.id = fieldId;
      memField.layoutId = field.layoutId;
      memField.offset = field.offset;
      memLayout.fields.push_back(std::move(memField));
    }
  }
}

void convert(const MemorySchema& memSchema, Schema& schema) {
  if (memSchema.layouts.size() == 0) {
    return;
  }

  for(int i = 0; i < memSchema.layouts.size(); ++i) {
    const auto& memLayout = memSchema.layouts.at(i);
    auto& newLayout = schema.layouts[i];

    newLayout.size = memLayout.size;
    newLayout.bits = memLayout.bits;

    for(const auto& field : memLayout.fields) {
      auto& newField = newLayout.fields[field.id];

      newField.layoutId = field.layoutId;
      newField.offset = field.offset;
    }
  }

  //
  // Type information is discarded when transforming from memSchema to
  // schema, so force this bit to true.
  //
  schema.relaxTypeChecks = true;
}

}}}}
