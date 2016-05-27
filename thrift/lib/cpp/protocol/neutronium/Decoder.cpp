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

#include <thrift/lib/cpp/protocol/neutronium/Decoder.h>
#include <folly/Conv.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace neutronium {

Decoder::Decoder(const Schema* schema, const InternTable* internTable,
                 folly::IOBuf* buffer)
  : schema_(schema),
    internTable_(internTable),
    cursor_(buffer),
    rootType_(0),
    bytesRead_(0) {
}

Decoder::DecoderState::DecoderState(const Schema* schema,
                                    int64_t type,
                                    const DataType* dt,
                                    uint32_t size)
  : dataType(dt),
    bytesRead(0),
    boolStartBit(0),
    totalStrictEnumBits(0) {
  switch (reflection::getType(type)) {
  case reflection::Type::TYPE_MAP:
    state = IN_MAP_VALUE;
    list.remaining = size;
    list.mapKeyType = TypeInfo(schema, dataType->mapKeyType);
    list.valueType = TypeInfo(schema, dataType->valueType);
    break;
  case reflection::Type::TYPE_SET:
    state = IN_SET_VALUE;
    list.remaining = size;
    list.valueType = TypeInfo(schema, dataType->valueType);
    break;
  case reflection::Type::TYPE_LIST:
    state = IN_LIST_VALUE;
    list.remaining = size;
    list.valueType = TypeInfo(schema, dataType->valueType);
    break;
  case reflection::Type::TYPE_STRUCT:
    state = IN_STRUCT;
    str.fieldState = FS_START;
    str.tag = 0;
    break;
  default:
    LOG(FATAL) << "Invalid aggregate type " << type;
  }
}

void Decoder::DecoderState::addType(TypeInfo& tinfo,
                                    const StructField& field,
                                    int16_t tag, uint32_t count) {
  const bool isStruct = (state == IN_STRUCT);

  auto type = tinfo.type();
  switch (type) {
  case reflection::Type::TYPE_BOOL:
    bools.count += count;
    if (isStruct) {
      str.boolTags.emplace_back(tag, type);
    }
    break;
  case reflection::Type::TYPE_BYTE:
    bytes.count += count;
    if (isStruct) {
      str.byteTags.emplace_back(tag, type);
    }
    break;
  case reflection::Type::TYPE_I64:     // fallthrough
  case reflection::Type::TYPE_DOUBLE:
    int64s.count += count;
    if (isStruct) {
      if (field.isFixed) {
        str.fixedInt64Tags.emplace_back(tag, type);
      } else {
        str.int64Tags.emplace_back(tag, type);
      }
    }
    break;
  case reflection::Type::TYPE_I16:
    ints.count += count;
    if (isStruct) {
      if (field.isFixed) {
        str.fixedInt16Tags.emplace_back(tag, type);
      } else {
        str.intTags.emplace_back(tag, type);
      }
    }
    break;
  case reflection::Type::TYPE_ENUM:
    if (field.isStrictEnum) {
      strictEnums.count += count;
      totalStrictEnumBits += count * tinfo.dataType->enumBits();
      if (isStruct) {
        str.strictEnumTags.emplace_back(tag, tinfo);
      }
      break;
    }
    type = reflection::Type::TYPE_I32;  // handle non-strict enums as i32
    // fallthrough (to TYPE_I32)
  case reflection::Type::TYPE_I32:
    ints.count += count;
    if (isStruct) {
      if (field.isFixed) {
        str.fixedInt32Tags.emplace_back(tag, type);
      } else {
        str.intTags.emplace_back(tag, type);
      }
    }
    break;
  case reflection::Type::TYPE_STRING:
    if (field.isInterned) {
      internedStrings.count += count;
      if (isStruct) {
        str.internedStringTags.emplace_back(tag, type);
      }
    } else {
      if (field.isTerminated) {
        tinfo.length = kTerminated;
        tinfo.terminator = field.terminator;
      } else if (field.isFixed) {
        tinfo.length = field.fixedStringSize;
      } else {
        vars.count += count;
      }
      strings.count += count;
      if (isStruct) {
        str.stringTags.emplace_back(tag, tinfo);
      }
    }
    break;

  default:  // user-defined type
    strings.count += count;
    if (isStruct) {
      str.stringTags.emplace_back(tag, tinfo);
    }
    break;
  }
}

folly::ByteRange Decoder::ensure(size_t n) {
  auto b = cursor_.peekBytes();
  if (UNLIKELY(b.size() < n)) {
    throw TProtocolException("underflow");
  }
  return b;
}

void Decoder::readBoolsAndStrictEnums(size_t skipBits) {
  auto& s = top();

  size_t nbytes = byteCount(skipBits + s.bools.count + s.totalStrictEnumBits);
  if (s.bools.count + s.totalStrictEnumBits) {
    s.boolStartBit = skipBits;
    s.bools.values.resize(nbytes);
    cursor_.pull(&s.bools.values.front(), nbytes);

    // Strict enums are only supported in structs for now
    DCHECK(s.totalStrictEnumBits == 0 || s.state == IN_STRUCT);

    // Extract strict enums
    size_t bit = skipBits + s.bools.count;
    for (size_t i = 0; i < s.strictEnums.count; ++i) {
      auto dt = s.str.strictEnumTags[i].second.dataType;
      auto bitCount = dt->enumBits();
      auto index = getBits(&s.bools.values.front(), bit, bitCount);
      bit += bitCount;
      s.strictEnums.values.push_back(*(dt->enumValues.begin() + index));
    }

    DCHECK_EQ(bit, skipBits + s.bools.count + s.totalStrictEnumBits);
  } else {
    cursor_.skip(nbytes);
  }
  s.bytesRead += nbytes;
}

void Decoder::read() {
  auto& s = top();

  size_t totalVarIntCount = 0;
  size_t optionalFieldBits = 0;
  if (s.state == IN_STRUCT) {
    size_t nbits = s.dataType->optionalFields.size();
    optionalFieldBits = nbits;
    size_t nbytes = byteCount(nbits);

    cursor_.gather(nbytes);
    auto optionalSet = ensure(nbytes).data();

    // Now we know which optionals are set
    // Assign by tag.
    size_t optionalIdx = 0;
    for (auto& p : s.dataType->fields) {
      auto tag = p.first;
      if (!p.second.isRequired && !testBit(optionalSet, optionalIdx++)) {
        continue;
      }
      TypeInfo tinfo(schema_, p.second.type);
      s.addType(tinfo, p.second, tag, 1);
    }

    // Structs encode the bitfield first, so we can use only one bitfield
    // for both optional fields and bools and strict enums
    readBoolsAndStrictEnums(nbits);
  } else {
    if (s.state == IN_MAP_VALUE) {
      s.addType(s.list.mapKeyType, StructField(), 0, s.list.remaining);
    }
    s.addType(s.list.valueType, StructField(), 0, s.list.remaining);
    ++totalVarIntCount;  // element count
  }

  s.ints.values.reserve(s.ints.count);
  s.int64s.values.reserve(s.int64s.count);
  s.bytes.values.reserve(s.bytes.count);
  // Waste some memory. Oh well, the code is simpler :)
  if (s.bools.count + s.totalStrictEnumBits) {
    s.bools.values.reserve(byteCount(optionalFieldBits + s.bools.count +
                                     s.totalStrictEnumBits));
  }
  s.strictEnums.values.reserve(s.strictEnums.count);
  s.vars.values.reserve(s.vars.count);
  s.internedStrings.values.reserve(s.internedStrings.count);

  // Read ints
  size_t varIntCount = s.ints.count;
  size_t varInt64Count = s.int64s.count;
  size_t varLengthCount = s.vars.count;
  size_t internCount = s.internedStrings.count;
  if (s.state == IN_STRUCT) {
    varIntCount -= (s.str.fixedInt16Tags.size() + s.str.fixedInt32Tags.size());
    varInt64Count -= s.str.fixedInt64Tags.size();
  }
  totalVarIntCount += varIntCount + 2 * varInt64Count + varLengthCount +
    internCount;
  if (totalVarIntCount) {
    size_t maxSize = folly::GroupVarint32::maxSize(totalVarIntCount);
    cursor_.gatherAtMost(maxSize);

    folly::StringPiece data{cursor_.peekBytes()};
    folly::GroupVarint32Decoder decoder(data, totalVarIntCount);

    if (s.state != IN_STRUCT) {
      uint32_t n;
      if (!decoder.next(&n)) {
        throw TProtocolException("too few ints on the wire");
      }
      DCHECK_EQ(n, s.list.remaining);
    }

    for (size_t i = 0; i < varIntCount; i++) {
      uint32_t val;
      if (!decoder.next(&val)) {
        throw TProtocolException("too few ints on the wire: int");
      }
      s.ints.values.push_back(val);
    }

    for (size_t i = 0; i < varInt64Count; i++) {
      uint32_t hi;
      uint32_t lo;
      if (!decoder.next(&hi) || !decoder.next(&lo)) {
        throw TProtocolException("too few ints on the wire: int64");
      }
      uint64_t val = ((uint64_t)hi << 32) | lo;
      s.int64s.values.push_back(val);
    }

    for (size_t i = 0; i < varLengthCount; i++) {
      uint32_t val;
      if (!decoder.next(&val)) {
        throw TProtocolException("too few ints on the wire: var");
      }
      s.vars.values.push_back(val);
    }

    for (size_t i = 0; i < internCount; i++) {
      uint32_t val;
      if (!decoder.next(&val)) {
        throw TProtocolException("too few ints on the wire: intern");
      }
      auto sp = internTable_->get(val);
      s.internedStrings.values.push_back(sp);
      if (s.state == IN_STRUCT) {
        // fixed size
        s.str.internedStringTags[i].second.length = sp.size();
      }
    }

    uint32_t tmp;
    CHECK(!decoder.next(&tmp));  // or else we have an internal error

    size_t bytesUsed = data.size() - decoder.rest().size();
    cursor_.skip(bytesUsed);
    s.bytesRead += bytesUsed;
  }

  // Read bools and strict enums, already done for structs
  if (s.state != IN_STRUCT) {
    readBoolsAndStrictEnums(0);
  }

  // Read bytes
  if (s.bytes.count) {
    s.bytes.values.resize(s.bytes.count);
    cursor_.pull(&s.bytes.values.front(), s.bytes.count);
    s.bytesRead += s.bytes.count;
  }

  // Read fixed-size fields, currently only for structs
  if (s.state == IN_STRUCT) {
    if (!s.str.fixedInt16Tags.empty()) {
      for (auto tag : s.str.fixedInt16Tags) {
        s.ints.values.push_back(cursor_.readBE<uint16_t>());
        s.str.intTags.emplace_back(tag);
      }
      s.bytesRead += s.str.fixedInt16Tags.size() * sizeof(uint16_t);
    }

    if (!s.str.fixedInt32Tags.empty()) {
      for (auto tag : s.str.fixedInt32Tags) {
        s.ints.values.push_back(cursor_.readBE<uint32_t>());
        s.str.intTags.emplace_back(tag);
      }
      s.bytesRead += s.str.fixedInt32Tags.size() * sizeof(uint32_t);
    }

    if (!s.str.fixedInt64Tags.empty()) {
      for (auto tag : s.str.fixedInt64Tags) {
        s.int64s.values.push_back(cursor_.readBE<uint64_t>());
        s.str.int64Tags.push_back(tag);
      }
      s.bytesRead += s.str.fixedInt64Tags.size() * sizeof(uint64_t);
    }
  }

}

}  // namespace neutronium
}  // namespace protocol
}  // namespace thrift
}  // namespace apache
