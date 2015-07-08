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

#include <stdexcept>

#include <folly/File.h>
#include <folly/MemoryMapping.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>
#include <thrift/lib/thrift/gen-cpp/frozen_constants.h>

namespace apache { namespace thrift { namespace frozen {

class FrozenFileForwardIncompatible : public std::runtime_error {
 public:
  explicit FrozenFileForwardIncompatible(int fileVersion);

  int fileVersion() const { return fileVersion_; }
  int supportedVersion() const {
    return schema::frozen_constants::kCurrentFrozenFileVersion();
  }

 private:
  int fileVersion_;
};

/**
 * Returns the number of bytes needed to freeze the given object, not including
 * padding bytes
 */
template <class T>
size_t frozenSize(const T& v) {
  Layout<T> layout;
  return LayoutRoot::layout(v, layout) - LayoutRoot::kPaddingBytes;
}

template <class T>
void serializeRootLayout(const Layout<T>& layout, std::string& out) {
  schema::MemorySchema memSchema;
  schema::Schema schema;
  saveRoot(layout, memSchema);
  schema::convert(memSchema, schema);

  schema.fileVersion = schema::frozen_constants::kCurrentFrozenFileVersion();
  util::ThriftSerializerCompact<>().serialize(schema, &out);
}

template <class T>
void deserializeRootLayout(folly::ByteRange& range, Layout<T>& layoutOut) {
  schema::Schema schema;
  size_t schemaSize = util::ThriftSerializerCompact<>().deserialize(
      range.begin(), range.size(), &schema);

  if (schema.fileVersion >
      schema::frozen_constants::kCurrentFrozenFileVersion()) {
    throw FrozenFileForwardIncompatible(schema.fileVersion);
  }

  schema::MemorySchema memSchema;
  schema::convert(std::move(schema), memSchema);
  loadRoot(layoutOut, memSchema);
  range.advance(schemaSize);
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
void freezeToFile(const T& x, folly::File file) {
  std::string schemaStr;
  auto layout = folly::make_unique<Layout<T>>();
  auto contentSize = LayoutRoot::layout(x, *layout);

  serializeRootLayout(*layout, schemaStr);

  folly::MemoryMapping mapping(std::move(file),
                               0,
                               contentSize + schemaStr.size(),
                               folly::MemoryMapping::writable());
  auto writeRange = mapping.writableRange();
  std::copy(schemaStr.begin(), schemaStr.end(), writeRange.begin());
  writeRange.advance(schemaStr.size());
  ByteRangeFreezer::freeze(*layout, x, writeRange);
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
void freezeToString(const T& x, std::string& out) {
  out.clear();
  auto layout = folly::make_unique<Layout<T>>();
  size_t contentSize = LayoutRoot::layout(x, *layout);
  serializeRootLayout(*layout, out);

  size_t schemaSize = out.size();
  out.resize(schemaSize + contentSize);
  folly::MutableByteRange writeRange(reinterpret_cast<byte*>(&out[schemaSize]),
                                     contentSize);
  ByteRangeFreezer::freeze(*layout, x, writeRange);
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return mapFrozen(folly::ByteRange range) {
  auto layout = folly::make_unique<Layout<T>>();
  deserializeRootLayout(range, *layout);
  Return ret(layout->view({range.begin(), 0}));
  ret.hold(std::move(layout));
  return ret;
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return mapFrozen(folly::StringPiece range) {
  return mapFrozen<T, Return>(folly::ByteRange(range));
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return mapFrozen(folly::MemoryMapping mapping) {
  auto ret = mapFrozen<T, Return>(mapping.range());
  ret.hold(std::move(mapping));
  return ret;
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return mapFrozen(folly::File file,
                 folly::MemoryMapping::LockMode lockMode =
                     folly::MemoryMapping::LockMode::TRY_LOCK) {
  folly::MemoryMapping mapping(std::move(file), 0);
  mapping.mlock(lockMode);
  return mapFrozen<T, Return>(std::move(mapping));
}

}}}
