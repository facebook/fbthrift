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

#include <thrift/lib/cpp2/frozen/schema/MemorySchema.h>

#include <limits>

#include <folly/Utility.h>

THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryField)
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryLayoutBase)
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemoryLayout)
THRIFT_IMPL_HASH(apache::thrift::frozen::schema::MemorySchema)

namespace apache {
namespace thrift {
namespace frozen {
namespace schema {

int16_t MemorySchema::Helper::add(MemoryLayout&& layout) {
  // Add distinct layout, bounds check layoutId
  size_t layoutId = layoutTable_.add(std::move(layout));
  CHECK_LE(layoutId, folly::to_unsigned(std::numeric_limits<int16_t>::max()))
      << "Layout overflow";
  return static_cast<int16_t>(layoutId);
}

void MemorySchema::initFromSchema(Schema&& schema) {
  if (!schema.layouts_ref()->empty()) {
    layouts.resize(schema.layouts_ref()->size());

    for (const auto& layoutKvp : *schema.layouts_ref()) {
      const auto id = layoutKvp.first;
      const auto& layout = layoutKvp.second;

      // Note: This will throw if there are any id >=
      // schema.layouts.size().
      auto& memLayout = layouts.at(id);

      memLayout.setSize(*layout.size_ref());
      memLayout.setBits(*layout.bits_ref());

      for (const auto& fieldKvp : *layout.fields_ref()) {
        MemoryField memField;
        const auto& fieldId = fieldKvp.first;
        const auto& field = fieldKvp.second;

        memField.setId(fieldId);
        memField.setLayoutId(*field.layoutId_ref());
        memField.setOffset(*field.offset_ref());
        memLayout.addField(std::move(memField));
      }
    }
  }
  setRootLayoutId(*schema.rootLayout_ref());
}

void convert(Schema&& schema, MemorySchema& memSchema) {
  memSchema.initFromSchema(std::move(schema));
}

void convert(const MemorySchema& memSchema, Schema& schema) {
  std::size_t i = 0;
  for (const auto& memLayout : memSchema.getLayouts()) {
    auto& newLayout = schema.layouts_ref()[i];

    *newLayout.size_ref() = memLayout.getSize();
    *newLayout.bits_ref() = memLayout.getBits();

    for (const auto& field : memLayout.getFields()) {
      auto& newField = newLayout.fields_ref()[field.getId()];

      *newField.layoutId_ref() = field.getLayoutId();
      *newField.offset_ref() = field.getOffset();
    }
    ++i;
  }

  //
  // Type information is discarded when transforming from memSchema to
  // schema, so force this bit to true.
  //
  *schema.relaxTypeChecks_ref() = true;
  *schema.rootLayout_ref() = memSchema.getRootLayoutId();
}

} // namespace schema
} // namespace frozen
} // namespace thrift
} // namespace apache
