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

package "facebook.com/thrift/fast_thrift/thrift/test/aligned_parser"

namespace cpp2 apache.thrift.fast_thrift.thrift.test.aligned_parser

@cpp.Type{name = "std::unique_ptr<folly::IOBuf>"}
typedef binary IOBufPtr

struct Request {
  1: IOBufPtr data;
  2: i64 marker;
}

@cpp.FastServer
@cpp.FastClient
service AlignedParserServer {
  void consume(1: Request request);
  string echo(1: string value);
}
