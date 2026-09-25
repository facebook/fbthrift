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

include "thrift/annotation/cpp.thrift"

package "facebook.com/thrift/fast_thrift/frame/read/test"

namespace cpp2 apache.thrift.fast_thrift.frame.read

@cpp.Type{name = "std::unique_ptr<folly::IOBuf>"}
typedef binary IOBufPtr

// The blob is the first field. That is the layout AlignedParser pads for.
struct Payload {
  1: IOBufPtr blob;
}

// A second field after the blob, so the test can tell a buffer holding only the
// blob from one holding the whole response.
struct ReadChunkResponse {
  1: IOBufPtr data;
  2: i64 checksum;
}

// The test sends a real request through the generated client, so that Thrift
// lays out the bytes instead of the test. It serializes the response with the
// generated result type, for the same reason.
service AlignedService {
  ReadChunkResponse put(1: Payload payload);
}
