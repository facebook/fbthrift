/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/omnithrift/transcoder/Transcoder.h>

#include <string>

namespace apache {
namespace thrift {
namespace omniclient {

using apache::thrift::protocol::TType;

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcode(
    Reader& in, Writer& out, const TType& type) {
  switch (type) {
    case TType::T_SET: {
      transcodeSet(in, out);
      break;
    }
    case TType::T_LIST: {
      transcodeList(in, out);
      break;
    }
    case TType::T_MAP: {
      transcodeMap(in, out);
      break;
    }
    case TType::T_STRUCT: {
      transcodeStruct(in, out);
      break;
    }
    // The remaining types are primitive types
    default: {
      transcodePrimitive(in, out, type);
    }
  }
}

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcodeSet(Reader& in, Writer& out) {
  TType elemType;
  uint32_t size;
  in.readSetBegin(elemType, size);
  out.writeSetBegin(elemType, size);
  // Transcode all set elements recursively
  for (uint32_t i = 0; i < size; ++i) {
    transcode(in, out, elemType);
  }
  in.readSetEnd();
  out.writeSetEnd();
}

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcodeList(Reader& in, Writer& out) {
  TType elemType;
  uint32_t size;
  in.readListBegin(elemType, size);
  out.writeListBegin(elemType, size);
  // Transcode all list elements recursively
  for (uint32_t i = 0; i < size; i++) {
    transcode(in, out, elemType);
  }
  in.readListEnd();
  out.writeListEnd();
}

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcodeMap(Reader& in, Writer& out) {
  TType keyType;
  TType valType;
  uint32_t size;
  in.readMapBegin(keyType, valType, size);
  // Some protocols don't store types for empty maps. However, the Writer might
  // expect valid types, so in that case set them to something random (T_I32).
  if (keyType == TType::T_STOP || valType == TType::T_STOP) {
    keyType = TType::T_I32;
    valType = TType::T_I32;
  }
  out.writeMapBegin(keyType, valType, size);
  // Transcode all key-value pairs recursively
  for (uint32_t i = 0; i < size; i++) {
    transcode(in, out, keyType);
    transcode(in, out, valType);
  }
  in.readMapEnd();
  out.writeMapEnd();
}

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcodeStruct(Reader& in, Writer& out) {
  std::string name;
  TType type;
  int16_t id;
  in.readStructBegin(name);
  out.writeStructBegin(name.c_str());
  // Transcode all of the struct's fields
  while (true) {
    in.readFieldBegin(name, type, id);
    if (type == TType::T_STOP) {
      break;
    }
    out.writeFieldBegin(name.c_str(), type, id);
    transcode(in, out, type);
    in.readFieldEnd();
    out.writeFieldEnd();
  }
  out.writeFieldStop();
  in.readStructEnd();
  out.writeStructEnd();
}

template <class Reader, class Writer>
void Transcoder<Reader, Writer>::transcodePrimitive(
    Reader& in, Writer& out, const TType& type) {
  switch (type) {
    case TType::T_BOOL: {
      bool val;
      in.readBool(val);
      out.writeBool(val);
      break;
    }
    case TType::T_BYTE: {
      int8_t byte;
      in.readByte(byte);
      out.writeByte(byte);
      break;
    }
    case TType::T_I16: {
      int16_t num;
      in.readI16(num);
      out.writeI16(num);
      break;
    }
    case TType::T_I32: {
      int32_t num;
      in.readI32(num);
      out.writeI32(num);
      break;
    }
    case TType::T_I64: {
      int64_t num;
      in.readI64(num);
      out.writeI64(num);
      break;
    }
    case TType::T_FLOAT: {
      float fl;
      in.readFloat(fl);
      out.writeFloat(fl);
      break;
    }
    case TType::T_DOUBLE: {
      double d;
      in.readDouble(d);
      out.writeDouble(d);
      break;
    }
    case TType::T_STRING: {
      std::string val;
      in.readString(val);
      out.writeString(val);
      break;
    }
    default:
      break;
  }
}

} // namespace omniclient
} // namespace thrift
} // namespace apache
