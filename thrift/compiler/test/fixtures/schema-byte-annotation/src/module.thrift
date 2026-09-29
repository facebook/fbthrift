/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

package "facebook.com/thrift/test/fixtures/schema_byte_annotation"

struct ByteAnnotation {
  1: byte value;
}

const byte kNegativeByte = -7;
const byte kMaxByte = 127;
const byte kMinByte = -128;
const ByteAnnotation kStructConst = ByteAnnotation{value = -7};
const list<byte> kByteList = [-128, 0, 127];
const map<byte, byte> kByteMap = {-1: 1};

@ByteAnnotation{value = -7}
struct Annotated {
  @ByteAnnotation{value = -128}
  1: byte field = 127;
}
