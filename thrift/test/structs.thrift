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

namespace cpp2 apache.thrift.test

cpp_include "thrift/test/structs_extra.h"

struct Basic {
  1: i32 def_field,
  2: required i32 req_field,
  3: optional i32 opt_field,
}

struct BasicBinaries {
  1: binary def_field,
  2: required binary req_field,
  3: optional binary opt_field,
}

struct HasInt {
  1: required i32 field,
}

struct BasicRefs {
  1: HasInt def_field (cpp.ref),
}

struct BasicRefsShared {
  1: HasInt def_field (cpp.ref_type = "shared"),
}

struct BasicRefsAnnotCppNoexceptMoveCtor {
  1: HasInt def_field (cpp.ref),
} (cpp.noexcept_move_ctor)

typedef binary (cpp.type = "WrappedType<folly::IOBuf>",
             cpp.indirection = ".raw") t_foo
typedef binary (cpp.type = "WrappedType<std::string>",
            cpp.indirection = ".raw") t_bar
typedef binary (cpp.type = "WrappedType<folly::IOBuf>",
            cpp.indirection = ".rawAccessor()") t_baz
struct IOBufIndirection {
  1: t_foo foo
  2: t_bar bar
  3: t_baz baz
}

struct HasSmallSortedVector {
  1: set<i32> (cpp.template = "SmallSortedVectorSet") set_field,
  2: map<i32, i32> (cpp.template = "SmallSortedVectorMap") map_field,
}

struct NoexceptMoveStruct {
  1: string string_field,
  2: i32 i32_field,
} (cpp.noexcept_move)
