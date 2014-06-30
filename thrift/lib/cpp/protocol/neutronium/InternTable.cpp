/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <thrift/lib/cpp/protocol/neutronium/InternTable.h>
#include <thrift/lib/cpp/protocol/neutronium/gen-cpp/intern_table_types.h>
#include <thrift/lib/cpp/protocol/TNeutroniumProtocol.h>
#include <folly/io/IOBufQueue.h>
#include <folly/Bits.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

namespace {
uint32_t kInitialCapacity = 4000;
uint32_t kGrowth = 4000;

const Schema& schema() {
  struct NeutroniumSchema {
    NeutroniumSchema() {
      InternTableSizes::_reflection_register(rschema);
      schema.add(rschema);
    }

    reflection::Schema rschema;
    Schema schema;
  };

  static NeutroniumSchema schema;
  return schema.schema;
}

}  // namespace

InternTable::InternTable()
  : maybeShared_(false) {
}

std::unique_ptr<folly::IOBuf> InternTable::serialize() {
  InternTableSizes tab;
  tab.sizes.reserve(strings_.size());
  for (auto s : strings_) {
    tab.sizes.push_back(s.size());
  }

  // TODO(tudorb): Remove this allocation, sigh.
  auto serializedBuf = folly::IOBuf::create(0);
  Neutronium(&schema(), nullptr, serializedBuf.get()).serialize(tab);

  if (head_) {
    serializedBuf->prependChain(head_->clone());
  }
  maybeShared_ = true;
  return serializedBuf;
}

void InternTable::deserialize(std::unique_ptr<folly::IOBuf>&& data) {
  clear();

  InternTableSizes tab;
  auto headerSize = Neutronium(&schema(), nullptr, data.get()).deserialize(tab);

  // Split off the header
  folly::IOBufQueue queue;
  queue.append(std::move(data));
  auto header = queue.split(headerSize);
  maybeShared_ = true;
  head_ = queue.move();

  // Ensure that each of the strings is stored contiguously.
  // Note that we have to do this before we store any data in strings_,
  // as each gather() might reallocate the current IOBuf.
  {
    folly::io::RWPrivateCursor cursor(head_.get());
    for (auto length : tab.sizes) {
      cursor.gather(length);
      cursor.skip(length);
    }
  }

  strings_.reserve(tab.sizes.size());

  {
    folly::io::RWPrivateCursor cursor(head_.get());
    for (auto length : tab.sizes) {
      auto pos = cursor.peek();
      // the first pass ensured that we have enough space
      DCHECK_GE(pos.second, length);
      strings_.emplace_back(reinterpret_cast<const char*>(pos.first), length);
      cursor.skip(length);
    }
  }
}

uint32_t InternTable::add(folly::StringPiece str) {
  if (!head_) {
    head_ = folly::IOBuf::create(kInitialCapacity);
    maybeShared_ = false;
  }

  if (maybeShared_) {
    // We only write to the *last* buffer in the chain, so only unshare that
    // buffer -- faster (and more memory-efficient) than unsharing the whole
    // chain.
    head_->prev()->unshareOne();
    maybeShared_ = false;
  }

  if (!writer_) {
    writer_.reset(new Writer(head_.get(), kGrowth));
    for (size_t i = 0; i < strings_.size(); i++) {
      writer_->map[strings_[i]] = i;
    }
  }

  auto pos = writer_->map.find(str);
  if (pos != writer_->map.end()) {
    return pos->second;
  }

  // Insert string in table; note that we are relying on the fact that Appender
  // never reallocates buffers, but instead creates new ones if necessary, so
  // strings never move and pointers to them remain valid.
  writer_->appender.ensure(str.size());

  auto dest = writer_->appender.writableData();
  memcpy(dest, str.data(), str.size());
  writer_->appender.append(str.size());
  folly::StringPiece sp(reinterpret_cast<char*>(dest), str.size());
  uint32_t id = strings_.size();
  writer_->map[sp] = id;
  strings_.push_back(sp);

  return id;
}

void InternTable::clear() {
  maybeShared_ = false;
  head_ = nullptr;
  strings_.clear();
  writer_ = nullptr;
}

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache
