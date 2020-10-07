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

namespace cpp2 facebook.thrift.test.tablebased

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr

struct StructA {
  1: optional string fieldA;
  2: optional i64 fieldB;
  3: optional StructB fieldC;
  5: optional list<string> fieldD;
  10: optional map<string, i64> fieldE;
  11: string fieldF;
}

struct StructB {
  1: string fieldA;
  2: optional i64 fieldB;
  3: optional IOBufPtr fieldC;
  5: list<i64> fieldD (cpp2.ref_type = "shared");
}

union Union {
  1: StructA fieldA;
  2: StructB fieldB;
  3: string fieldC;
}

union UnionWithRef {
  1: StructA fieldA (cpp2.ref_type = "unique");
  2: StructB fieldB;
}
