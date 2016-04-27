/*
 * Copyright 2016 Facebook, Inc.
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

namespace apache { namespace thrift {

namespace detail { namespace compact_v1 {

constexpr uint8_t kCompactV1ProtocolVersion = 0x01;

}}

inline uint32_t CompactV1ProtocolWriter::writeMessageBegin(
    const std::string& name,
    MessageType messageType,
    int32_t seqid) {
  uint32_t wsize = 0;
  wsize += writeByte(detail::compact::PROTOCOL_ID);
  wsize += writeByte(detail::compact_v1::kCompactV1ProtocolVersion |
                     ((messageType << detail::compact::TYPE_SHIFT_AMOUNT) &
                      detail::compact::TYPE_MASK ));
  wsize += apache::thrift::util::writeVarint(out_, seqid);
  wsize += writeString(name);
  return wsize;
}

inline uint32_t CompactV1ProtocolWriter::writeDouble(double dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = bitwise_cast<uint64_t>(dub);
  out_.writeLE(bits);
  return sizeof(bits);
}

inline uint32_t CompactV1ProtocolReader::readMessageBegin(
    std::string& name,
    MessageType& messageType,
    int32_t& seqid) {
  uint32_t rsize = 0;
  int8_t protocolId;
  int8_t versionAndType;

  rsize += readByte(protocolId);
  if (protocolId != detail::compact::PROTOCOL_ID) {
    throw TProtocolException(TProtocolException::BAD_VERSION,
                             "Bad protocol identifier");
  }

  rsize += readByte(versionAndType);
  if ((int8_t)(versionAndType & VERSION_MASK) !=
      detail::compact_v1::kCompactV1ProtocolVersion) {
    throw TProtocolException(TProtocolException::BAD_VERSION,
                             "Bad protocol version");
  }

  messageType = (MessageType)
    ((versionAndType & detail::compact::TYPE_MASK) >>
     detail::compact::TYPE_SHIFT_AMOUNT);
  rsize += apache::thrift::util::readVarint(in_, seqid);
  rsize += readString(name);

  return rsize;
}

inline uint32_t CompactV1ProtocolReader::readDouble(double& dub) {
  BOOST_STATIC_ASSERT(sizeof(double) == sizeof(uint64_t));
  BOOST_STATIC_ASSERT(std::numeric_limits<double>::is_iec559);

  uint64_t bits = in_.readLE<int64_t>();
  dub = bitwise_cast<double>(bits);
  return 8;
}

}}
