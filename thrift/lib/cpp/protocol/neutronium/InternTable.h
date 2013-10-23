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

#ifndef THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_INTERNTABLE_H_
#define THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_INTERNTABLE_H_

#include <memory>
#include <unordered_map>
#include <vector>
#include "folly/Range.h"
#include "folly/io/IOBuf.h"
#include "folly/io/Cursor.h"

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

/**
 * Interned string table.
 *
 * Strings are stored in a chain of IOBufs, so serializing and deserializing
 * the InternTable is fast and easy.
 */
class InternTable {
 public:
  InternTable();

  /**
   * Add a string to the table, returning the id.
   */
  uint32_t add(folly::StringPiece str);

  /**
   * Get a string from the table, by id; throws std::out_of_range if the
   * id is invalid.
   */
  folly::StringPiece get(uint32_t id) const {
    return strings_.at(id);
  }

  /**
   * Serialize the table to a IOBuf chain.
   * Note that part of the chain may be shared with the InternTable, so
   * you must call unshare() on the returned buffers if you want to
   * write to them.
   */
  std::unique_ptr<folly::IOBuf> serialize();

  /**
   * Load the table from a IOBuf chain.
   * Takes ownership of the IOBuf chain.
   * InternTable will call unshare() appropriately before writing to
   * buffers from the provided chain.
   */
  void deserialize(std::unique_ptr<folly::IOBuf>&& data);

  /**
   * Clear the table.
   */
  void clear();

 private:
  // Performance optimization to reduce the number of calls to unshare() /
  // unshareOne(), which folly::io::Cursor benchmarks have shown to be
  // somewhat expensive.  Given that we know that we only write to the tail
  // of the IOBuf chain, we can often tell with certainty that the tail
  // is private to us and so we don't need to check or call unshareOne().
  bool maybeShared_;
  std::unique_ptr<folly::IOBuf> head_;
  std::vector<folly::StringPiece> strings_;

  typedef
    std::unordered_map<folly::StringPiece, uint32_t, folly::StringPieceHash>
    StringMap;

  // Only allocate writer if needed (when actually add()ing)
  struct Writer {
    template <typename... Args>
    /* implicit */ Writer(Args... args) : appender(args...) { }
    folly::io::Appender appender;
    StringMap map;
  };
  std::unique_ptr<Writer> writer_;

  // Not copiable; not movable
  InternTable(const InternTable&) = delete;
  InternTable(InternTable&&) = delete;
  InternTable& operator=(const InternTable&) = delete;
  InternTable& operator=(InternTable&&) = delete;

};

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache

#endif /* THRIFT_LIB_CPP_PROTOCOL_NEUTRONIUM_INTERNTABLE_H_ */
