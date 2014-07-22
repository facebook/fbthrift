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
#include <folly/File.h>
#include <folly/MemoryMapping.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>

namespace apache { namespace thrift { namespace frozen {

template <class T>
size_t frozenSize(const T& v) {
  Layout<T> layout;
  return LayoutRoot::layout(v, layout);
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return freezeToFile(const T& x, folly::File file) {
  auto layout = folly::make_unique<Layout<T>>();
  auto size = LayoutRoot::layout(x, *layout);

  std::string schemaStr;
  {
    schema::Schema schema;
    layout->saveRoot(schema);
    util::ThriftSerializerCompact<>().serialize(schema, &schemaStr);
  }

  folly::MemoryMapping mapping(std::move(file),
                               0,
                               size + schemaStr.size(),
                               folly::MemoryMapping::writable());

  auto writeRange = mapping.writableRange();

  std::copy(schemaStr.begin(), schemaStr.end(), writeRange.begin());
  writeRange.advance(schemaStr.size());

  Return ret(ByteRangeFreezer::freeze(*layout, x, writeRange));
  ret.hold(std::move(layout));
  ret.hold(std::move(mapping));
  return ret;
}

template <class T, class Return = Bundled<typename Layout<T>::View>>
Return mapFrozen(folly::MemoryMapping mapping) {
  auto layout = folly::make_unique<Layout<T>>();
  auto range = mapping.range();

  {
    schema::Schema schema;
    size_t schemaSize = util::ThriftSerializerCompact<>().deserialize(
        range.begin(), range.size(), &schema);
    layout->loadRoot(schema);
    range.advance(schemaSize);
  }

  Return ret(layout->view({range.begin(), 0}));
  ret.hold(std::move(layout));
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
