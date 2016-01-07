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

#include <thrift/lib/cpp2/protocol/JSONProtocolCommon.h>

namespace apache { namespace thrift {

// This table describes the handling for the first 0x30 characters
//  0 : escape using "\u00xx" notation
//  1 : just output index
// <other> : escape using "\<other>" notation
const uint8_t JSONProtocolWriterCommon::kJSONCharTable[0x30] = {
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    0,  0,  0,  0,  0,  0,  0,  0,'b','t','n',  0,'f','r',  0,  0, // 0
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 1
    1,  1,'"',  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, // 2
};

// This string's characters must match up with the elements in kEscapeCharVals
// I don't have '/' on this list even though it appears on www.json.org --
// it is not in the RFC
const std::string JSONProtocolReaderCommon::kEscapeChars("\"\\/bfnrt");

// The elements of this array must match up with the sequence of characters in
// kEscapeChars
const uint8_t JSONProtocolReaderCommon::kEscapeCharVals[8] = {
  '"', '\\', '/', '\b', '\f', '\n', '\r', '\t',
};

}}
