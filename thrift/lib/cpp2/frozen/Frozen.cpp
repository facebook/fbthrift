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

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

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

void LayoutBase::loadRoot(const schema::MemorySchema& schema) {
  this->clear();
  this->load(schema, schema.layouts.at(0));
}

void LayoutBase::saveRoot(schema::MemorySchema& schema) const {
  // Bootstrap implicit ID mechanism
  schema.layouts.push_back(schema::MemoryLayout());

  schema::MemorySchemaHelper helper;
  schema::MemoryLayout myLayout;
  this->save(schema, myLayout, helper);
  schema.layouts[0] = std::move(myLayout);
  schema.layouts.shrink_to_fit();
}

void LayoutBase::load(const schema::MemorySchema& schema,
                      const schema::MemoryLayout& layout) {
  size = layout.size;
  bits = layout.bits;
}

void LayoutBase::save(schema::MemorySchema& schema,
                      schema::MemoryLayout& layout,
                      schema::MemorySchemaHelper& helper) const {
  layout.size = size;
  layout.bits = bits;
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
  maskField.print(os, level + 1);
  offsetField.print(os, level + 1);
}

void BlockLayout::clear() {
  maskField.clear();
  offsetField.clear();
}

size_t BufferHelpers<std::unique_ptr<folly::IOBuf>>::size(
    const std::unique_ptr<folly::IOBuf>& src) {
  return src->computeChainDataLength();
}

void BufferHelpers<std::unique_ptr<folly::IOBuf>>::copyTo(
    const std::unique_ptr<folly::IOBuf>& src, folly::MutableByteRange dst) {
  folly::io::Cursor(src.get()).pull(dst.begin(), dst.size());
}

void BufferHelpers<std::unique_ptr<folly::IOBuf>>::thawTo(
    folly::ByteRange src, std::unique_ptr<folly::IOBuf>& dst) {
  dst = folly::IOBuf::copyBuffer(src.begin(), src.size());
}

}}}}
