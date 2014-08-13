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

#include <limits>

#include <folly/Hash.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>


THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryField);
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryLayoutBase);
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryLayout);
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemorySchema);

namespace apache { namespace thrift { namespace frozen { namespace schema {

size_t MemoryField::hash() const {
  return folly::hash::hash_combine(id, layoutId, offset);
}
bool MemoryField::operator==(const MemoryField& other) const {
  return id == other.id && layoutId == other.layoutId && offset == other.offset;
}

size_t MemoryLayoutBase::hash() const {
  return folly::hash::hash_combine(bits, size);
}
bool MemoryLayoutBase::operator==(const MemoryLayoutBase& other) const {
  return bits == other.bits && size == other.size;
}

size_t MemoryLayout::hash() const {
  return folly::hash::hash_combine(
      MemoryLayoutBase::hash(),
      folly::hash::hash_range(fields.begin(), fields.end()));
}
bool MemoryLayout::operator==(const MemoryLayout& other) const {
  return MemoryLayoutBase::operator==(other) && fields == other.fields;
}

size_t MemorySchema::hash() const {
  return folly::hash::hash_combine(
      folly::hash::hash_range(layouts.begin(), layouts.end()), rootLayout);
}
bool MemorySchema::operator==(const MemorySchema& other) const {
  return layouts == other.layouts;
}

int16_t MemorySchemaHelper::add(MemoryLayout&& layout) {
  // Add distinct layout, bounds check layoutId
  size_t layoutId = layoutTable_.add(std::move(layout));
  CHECK_LE(layoutId, std::numeric_limits<int16_t>::max()) << "Layout overflow";
  return static_cast<int16_t>(layoutId);
}

void convert(Schema&& schema, MemorySchema& memSchema) {
  if (!schema.layouts.empty()) {
    memSchema.layouts.resize(schema.layouts.size());

    for (const auto& layoutKvp : schema.layouts) {
      const auto id = layoutKvp.first;
      const auto& layout = layoutKvp.second;

      // Note: This will throw if there are any id >=
      // schema.layouts.size().
      auto& memLayout = memSchema.layouts.at(id);

      memLayout.size = layout.size;
      memLayout.bits = layout.bits;

      memLayout.fields.reserve(layout.fields.size());

      for (const auto& fieldKvp : layout.fields) {
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
  memSchema.rootLayout = schema.rootLayout;
}

void convert(const MemorySchema& memSchema, Schema& schema) {
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
  schema.rootLayout = memSchema.rootLayout;
}

}}}}
