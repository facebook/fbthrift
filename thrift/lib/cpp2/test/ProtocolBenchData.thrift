/*
 * Copyright 2014 Facebook, Inc.
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

struct Empty {
}

struct SmallInt {
  1: i32 smallint;
}

struct BigInt {
  1: i64 bigint;
}

struct SmallString {
  1: string str;
}

struct BigString {
  1: string str;
}

typedef binary (cpp2.type = "std::unique_ptr<folly::IOBuf>") IOBuf

struct BigBinary {
  1: IOBuf bin;
}

struct Mixed {
  1: i32 int32;
  2: i64 int64;
  3: bool b;
  4: string str;
}

struct SmallListInt {
  1: list<i32> lst;
}

struct BigListInt {
  1: list<i32> lst;
}

struct BigListMixed {
  1: list<Mixed> lst;
}
