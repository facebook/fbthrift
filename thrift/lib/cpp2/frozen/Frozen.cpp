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

#include <thrift/lib/cpp2/frozen/Frozen.h>

namespace apache { namespace thrift { namespace frozen {

std::ostream& operator<<(std::ostream& os, DebugLine dl) {
  os << '\n';
  for (int i = 0; i < dl.level; ++i) {
    os << ' ' << ' ';
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const LayoutBase& layout) {
  layout.print(os, 0);
  return os;
}

bool LayoutBase::resize(FieldPosition after, bool inlined) {
  bool resized = false;
  this->inlined = (this->size == 0 && inlined);
  if (!this->inlined) {
    if (after.offset > this->size) {
      this->size = after.offset;
      resized = true;
    }
  }
  if (after.bitOffset > this->bits) {
    this->bits = after.bitOffset;
    resized = true;
  }
  return resized;
}

void LayoutBase::print(std::ostream& os, int level) const {
  os << DebugLine(level);
  if (size) {
    os << size << " byte";
    if (bits) {
      os << " (with " << bits << " bits)";
    }
  } else if (bits) {
    os << bits << " bit";
  } else {
    os << "empty";
  }
  os << ' ';
}

void LayoutBase::clear() {
  size = 0;
  bits = 0;
  inlined = false;
}

void LayoutBase::loadRoot(const schema::Schema& schema) {
  this->clear();
  this->load(schema, schema.layouts.at(0));
}

void LayoutBase::saveRoot(schema::Schema& schema) const {
  this->save(schema, schema.layouts[0]);
}

void LayoutBase::load(const schema::Schema& schema,
                      const schema::Layout& layout) {
  size = layout.size;
  bits = layout.bits;
  if (!schema.relaxTypeChecks) {
    auto expectedType = folly::demangle(type.name());
    auto actualType = layout.typeName;
    if (expectedType != actualType) {
      throw LayoutTypeMismatchException(expectedType.toStdString(), actualType);
    }
  }
}

void LayoutBase::save(schema::Schema& schema, schema::Layout& layout) const {
  layout.size = size;
  layout.bits = bits;
  layout.typeName = folly::demangle(type.name()).toStdString();
}

namespace detail {

FieldPosition BlockLayout::layout(LayoutRoot& root,
                                  const T& x,
                                  LayoutPosition self) {
  FieldPosition pos = startFieldPosition();
  FROZEN_LAYOUT_FIELD(mask);
  FROZEN_LAYOUT_FIELD(offset);
  return pos;
}

void BlockLayout::freeze(FreezeRoot& root,
                         const T& x,
                         FreezePosition self) const {
  FROZEN_FREEZE_FIELD(mask);
  FROZEN_FREEZE_FIELD(offset);
}

void BlockLayout::print(std::ostream& os, int level) const {
  LayoutBase::print(os, level);
  os << folly::demangle(type.name());
  maskField.print(os, "mask", level + 1);
  offsetField.print(os, "offset", level + 1);
}

void BlockLayout::clear() {
  maskField.clear();
  offsetField.clear();
}

void BlockLayout::save(schema::Schema& schema, schema::Layout& layout) const {
  LayoutBase::save(schema, layout);
  maskField.save(schema, layout);
  offsetField.save(schema, layout);
}

void BlockLayout::load(const schema::Schema& schema,
                       const schema::Layout& layout) {
  LayoutBase::load(schema, layout);
  maskField.load(schema, layout);
  offsetField.load(schema, layout);
}

}}}}
