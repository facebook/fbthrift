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

#pragma once

#include <thrift/lib/cpp/protocol/TType.h>

namespace apache {
namespace thrift {
namespace omniclient {

/**
 * Transcoder contains utility functions that can be used to transcode
 * encoded messages from any Thrift protocol (Reader) to any other (Writer).
 *
 * Supports all protocols except for SimpleJSON, since SimpleJSON does not
 * encode field IDs, types or container sizes.
 *
 * The passed in Reader and Writer must already have the corresponding
 * IOBuf/IOBufQueue set as input/output.
 */
template <class Reader, class Writer>
class Transcoder {
 public:
  /**
   * Transcodes from the Reader protocol into the Writer protocol.
   * The given protocol::TType must match the data in the Reader's buffer.
   */
  static void transcode(
      Reader& in, Writer& out, const apache::thrift::protocol::TType& type);

 private:
  static void transcodeSet(Reader& in, Writer& out);
  static void transcodeList(Reader& in, Writer& out);
  static void transcodeMap(Reader& in, Writer& out);
  static void transcodeStruct(Reader& in, Writer& out);
  static void transcodePrimitive(
      Reader& in, Writer& out, const apache::thrift::protocol::TType& type);
};

} // namespace omniclient
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/omnithrift/transcoder/Transcoder-inl.h>
