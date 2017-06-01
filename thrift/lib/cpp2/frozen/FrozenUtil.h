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
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>
#include <thrift/lib/thrift/gen-cpp/frozen_constants.h>

DECLARE_bool(thrift_frozen_util_disable_mlock);

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

template <class T>
void freezeToFile(const T& x, folly::File file) {
  std::string schemaStr;
  auto layout = std::make_unique<Layout<T>>();
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

template <class T>
void freezeToString(const T& x, std::string& out) {
  out.clear();
  auto layout = std::make_unique<Layout<T>>();
  size_t contentSize = LayoutRoot::layout(x, *layout);
  serializeRootLayout(*layout, out);

  size_t schemaSize = out.size();
  out.resize(schemaSize + contentSize);
  folly::MutableByteRange writeRange(reinterpret_cast<byte*>(&out[schemaSize]),
                                     contentSize);
  ByteRangeFreezer::freeze(*layout, x, writeRange);
}

/**
 * mapFrozen<T>() returns an owned reference to a frozen object which can be
 * used as a Frozen view of the object. All overloads of this function return
 * MappedFrozen<T>, which is an alias for a type which bundles the view with its
 * associated resources. This type may be used directly to hold references to
 * mapped frozen objects.
 *
 * Depending on which overload is used, this bundle will hold references to
 * different associated data:
 *
 *  - mapFrozen<T>(ByteRange): Only the layout tree associated with the object.
 *  - mapFrozen<T>(StringPiece): Same as mapFrozen<T>(ByteRange).
 *  - mapFrozen<T>(MemoryMapping): Takes ownership of the memory mapping
 *      in addition to the layout tree.
 *  - mapFrozen<T>(File): Owns the memory mapping created from the File (which,
 *      in turn, takes ownership of the File) in addition to the layout tree.
 */
template <class T>
using MappedFrozen = Bundled<typename Layout<T>::View>;

template <class T>
MappedFrozen<T> mapFrozen(folly::ByteRange range) {
  auto layout = std::make_unique<Layout<T>>();
  deserializeRootLayout(range, *layout);
  MappedFrozen<T> ret(layout->view({range.begin(), 0}));
  ret.hold(std::move(layout));
  return ret;
}

template <class T>
MappedFrozen<T> mapFrozen(folly::StringPiece range) {
  return mapFrozen<T>(folly::ByteRange(range));
}

template <class T>
MappedFrozen<T> mapFrozen(folly::MemoryMapping mapping) {
  auto ret = mapFrozen<T>(mapping.range());
  ret.hold(std::move(mapping));
  return ret;
}

/**
 * Maps from the given string, taking ownership of it and bundling it with the
 * return object to ensure its lifetime.
 * @param trim Trims the serialized layout from the input string.
 */
template <class T>
MappedFrozen<T> mapFrozen(std::string&& str, bool trim = true) {
  auto layout = std::make_unique<Layout<T>>();
  auto holder = std::make_unique<HolderImpl<std::string>>(std::move(str));
  auto& ownedStr = holder->t_;
  folly::ByteRange rangeBefore = folly::StringPiece(ownedStr);
  folly::ByteRange range = rangeBefore;
  deserializeRootLayout(range, *layout);
  if (trim) {
    size_t trimSize = range.begin() - rangeBefore.begin();
    ownedStr.erase(ownedStr.begin(), ownedStr.begin() + trimSize);
    ownedStr.shrink_to_fit();
    range = folly::StringPiece(holder->t_);
  }
  MappedFrozen<T> ret(layout->view({range.begin(), 0}));
  ret.holdImpl(std::move(holder));
  ret.hold(std::move(layout));
  return ret;
}

template <class T>
FOLLY_DEPRECATED(
    "std::string values must be passed by move with std::move(str) or "
    "passed through non-owning StringPiece")
MappedFrozen<T> mapFrozen(const std::string& str) = delete;

template <class T>
MappedFrozen<T> mapFrozen(folly::File file) {
  folly::MemoryMapping mapping(std::move(file), 0);
  if (!FLAGS_thrift_frozen_util_disable_mlock) {
    mapping.mlock(folly::MemoryMapping::LockMode::TRY_LOCK);
  }
  return mapFrozen<T>(std::move(mapping));
}

template <class T>
MappedFrozen<T> mapFrozen(folly::File file,
                          folly::MemoryMapping::LockMode lockMode) {
  folly::MemoryMapping mapping(std::move(file), 0);
  mapping.mlock(lockMode);
  return mapFrozen<T>(std::move(mapping));
}

}}}
