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

#include "thrift/lib/cpp/protocol/neutronium/Encoder.h"
#include "folly/GroupVarint.h"
#include "folly/io/Cursor.h"

using folly::IOBuf;

static_assert(sizeof(int8_t) == 1, "int8_t");
static_assert(sizeof(int16_t) == 2, "int16_t");
static_assert(sizeof(int32_t) == 4, "int32_t");
static_assert(sizeof(int64_t) == 8, "int64_t");
static_assert(sizeof(double) == 8, "double");
static_assert(std::numeric_limits<double>::is_iec559, "double format");

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

namespace {
// Somewhat less than 256
static uint32_t kInitialIOBufCapacity = 200;
static uint32_t kSubsequentIOBufCapacity = 4040;
}  // namespace

Encoder::EncoderState::EncoderState(
    int64_t type, const DataType* dt, uint32_t size)
  : dataType(dt),
    tag(0),
    buf(IOBuf::create(kInitialIOBufCapacity)),
    appender(buf.get(), kSubsequentIOBufCapacity),
    totalStrictEnumBits(0),
    optionalSet(dt->optionalFields.size(), false),
    bytesWritten(0) {
  switch (reflection::getType(type)) {
  case reflection::TYPE_MAP:
    state = IN_MAP_KEY;
    field.type = dataType->mapKeyType;
    break;
  case reflection::TYPE_SET:
    state = IN_SET_VALUE;
    field.type = dataType->valueType;
    break;
  case reflection::TYPE_LIST:
    state = IN_LIST_VALUE;
    field.type = dataType->valueType;
    break;
  case reflection::TYPE_STRUCT:
    state = IN_STRUCT;
    break;
  default:
    LOG(FATAL) << "Invalid aggregate type " << type;
  }
}

Encoder::Encoder(const Schema* schema, InternTable* internTable,
                 folly::IOBuf* buf)
  : schema_(schema),
    rootType_(0),
    internTable_(internTable),
    outputBuf_(buf) {
}

void Encoder::setRootType(int64_t rootType) {
  CHECK(stack_.empty());
  rootType_ = rootType;
}

namespace {

class CountingAppender {
 public:
  /* implicit */ CountingAppender(folly::io::Appender* appender)
    : appender_(appender),
      bytesWritten_(0) { }
  void operator()(folly::StringPiece sp) {
    appender_->push(reinterpret_cast<const uint8_t*>(sp.data()), sp.size());
    bytesWritten_ += sp.size();
  }

  size_t bytesWritten() const {
    return bytesWritten_;
  }

 private:
  folly::io::Appender* appender_;
  size_t bytesWritten_;
};

typedef folly::GroupVarintEncoder<uint32_t, CountingAppender>
  GroupVarint32Encoder;

template <class Pair>
struct LessFirst {
  bool operator()(const Pair& a, const Pair& b) {
    return a.first < b.first;
  }
};

template <class Vec>
void sortByTag(Vec& vec) {
  std::sort(vec.begin(), vec.end(), LessFirst<typename Vec::value_type>());
}
}  // namespace

void Encoder::flushStruct() {
  // Sort values in tag order

  auto& s = top();
  sortByTag(s.bools);
  sortByTag(s.strictEnums);
  sortByTag(s.bytes);
  sortByTag(s.fixedInt16s);
  sortByTag(s.fixedInt32s);
  sortByTag(s.fixedInt64s);
  sortByTag(s.varInts);
  sortByTag(s.varInt64s);
  sortByTag(s.varLengths);
  sortByTag(s.varInternIds);
  sortByTag(s.strings);

  // Write __isset bitset and all other bools now that we're at it
  // (this allows us to only use one byte if the number of optional fields
  // + the number of bool fields is <= 8)
  flushBitValues();
}

void Encoder::flushBitValues() {
  // Flush bit values (optionalSet bitmap for structs, bools and strict enums)
  // These are flushed at the beginning for structs, and later on for
  // containers; structs expect to be able to get the list of optional fields
  // quickly (and so it should be first), whereas containers expect to be
  // able to get the element count quickly (and so that should be first)
  auto& s  = top();

  size_t byteSize =
    byteCount(s.optionalSet.size() + s.bools.size() + s.totalStrictEnumBits);
  s.appender.ensure(byteSize);
  uint8_t* out = s.appender.writableData();
  memset(out, 0, byteSize);
  size_t j = 0;
  for (auto f : s.optionalSet) {
    if (f) {
      setBit(out, j);
    }
    ++j;
  }

  for (auto& p : s.bools) {
    if (p.second) {
      setBit(out, j);
    }
    ++j;
  }

  for (auto& p : s.strictEnums) {
    setBits(out, j, p.second.bits, p.second.value);
    j += p.second.bits;
  }

  DCHECK_EQ(j, s.optionalSet.size() + s.bools.size() + s.totalStrictEnumBits);

  s.appender.append(byteSize);
  s.bytesWritten += byteSize;
}

void Encoder::flushData(bool isStruct) {
  // Group-Varint encode all ints (including lengths of variable-length
  // fields)
  // This must be first -- the first int is the element count for maps,
  // lists, and sets
  auto& s = top();
  {
    GroupVarint32Encoder enc(&s.appender);
    if (!isStruct) {  // set/list/map
      // Encode element count
      uint32_t n =
        s.bools.size() + s.strictEnums.size() + s.bytes.size() +
        s.fixedInt16s.size() + s.fixedInt32s.size() + s.fixedInt64s.size() +
        s.varInts.size() + s.varInt64s.size() + s.strings.size() +
        s.varInternIds.size();
      if (s.state == IN_MAP_KEY) {
        DCHECK_EQ(0, n % 2);
        n /= 2;
      }
      enc.add(n);
    }

    for (auto& p : s.varInts) {
      enc.add(p.second);
    }

    for (auto& p : s.varInt64s) {
      uint32_t hi = (uint64_t)p.second >> 32;
      uint32_t lo = (uint64_t)p.second & ((1ULL << 32) - 1);
      enc.add(hi);
      enc.add(lo);
    }

    for (auto& p : s.varLengths) {
      enc.add(p.second);
    }

    for (auto& p : s.varInternIds) {
      enc.add(p.second);
    }

    enc.finish();
    s.bytesWritten += enc.output().bytesWritten();
  }

  if (!isStruct) {
    // Write all bools and strict enums, in tag order; we've already done this
    // for structs.
    flushBitValues();
  }

  // Write all bytes, in tag order
  s.appendToOutput(s.bytes);

  // Write all fixed-size int16s, int32s and int64s
  s.appendToOutput(s.fixedInt16s);
  s.appendToOutput(s.fixedInt32s);
  s.appendToOutput(s.fixedInt64s);

  // Write all other fields (strings, subobjects)
  for (auto& p : s.strings) {
    // Strings increment bytesWritten themselves
    s.buf->prependChain(std::move(p.second));
  }
}

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache
