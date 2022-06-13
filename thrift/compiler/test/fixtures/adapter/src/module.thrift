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

namespace android test.fixtures.adapter
namespace java test.fixtures.adapter
namespace java2 test.fixtures.adapter
namespace java.swift test.fixtures.adapter

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/python.thrift"
include "thrift/annotation/thrift.thrift"
include "thrift/annotation/hack.thrift"

@hack.Adapter{name = '\Adapter2'}
typedef set<string> (
  cpp.adapter = 'my::Adapter2',
  py.adapter = 'my.Adapter2',
) SetWithAdapter
@hack.Adapter{name = '\Adapter1'}
typedef string (
  cpp.adapter = 'my::Adapter1',
  py.adapter = 'my.Adapter1',
) StringWithAdapter
typedef list<StringWithAdapter> ListWithElemAdapter
@hack.Adapter{name = '\Adapter2'}
typedef ListWithElemAdapter ListWithElemAdapter_withAdapter

@cpp.Adapter{name = "my::Adapter1"}
@python.Adapter{
  name = "my.module.Adapter2",
  typeHint = "my.another.module.AdaptedType2",
}
typedef i64 MyI64

typedef MyI64 DoubleTypedefI64

@hack.Adapter{name = '\Adapter1'}
typedef i32 MyI32

struct Foo {
  @hack.Adapter{name = '\Adapter1'}
  1: i32 (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1') intField;
  @hack.Adapter{name = '\Adapter1'}
  2: optional i32 (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) optionalIntField;
  @hack.Adapter{name = '\Adapter1'}
  3: i32 (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) intFieldWithDefault = 13;
  4: SetWithAdapter setField;
  5: optional SetWithAdapter optionalSetField;
  @hack.Adapter{name = '\Adapter3'}
  6: map<
    string,
    ListWithElemAdapter_withAdapter (
      cpp.adapter = 'my::Adapter2',
      py.adapter = 'my.Adapter2',
    )
  > (cpp.adapter = 'my::Adapter3', py.adapter = 'my.Adapter3') mapField;
  @hack.Adapter{name = '\Adapter3'}
  7: optional map<
    string,
    ListWithElemAdapter_withAdapter (
      cpp.adapter = 'my::Adapter2',
      py.adapter = 'my.Adapter2',
    )
  > (cpp.adapter = 'my::Adapter3', py.adapter = 'my.Adapter3') optionalMapField;
  @hack.Adapter{name = '\Adapter1'}
  8: binary (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) binaryField;
  9: MyI64 longField;
  @cpp.Adapter{name = "my::Adapter2"}
  @python.Adapter{name = "my.Adapter3", typeHint = "my.AdaptedType3"}
  10: MyI64 adaptedLongField;
  11: DoubleTypedefI64 doubleAdaptedField;
} (
  thrift.uri = "facebook.com/thrift/compiler/test/fixtures/adapter/src/module/Foo",
)

union Baz {
  @hack.Adapter{name = '\Adapter1'}
  1: i32 (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1') intField;
  4: SetWithAdapter setField;
  @hack.Adapter{name = '\Adapter3'}
  6: map<
    string,
    ListWithElemAdapter_withAdapter (
      cpp.adapter = 'my::Adapter2',
      py.adapter = 'my.Adapter2',
    )
  > (cpp.adapter = 'my::Adapter3', py.adapter = 'my.Adapter3') mapField;
  @hack.Adapter{name = '\Adapter1'}
  8: binary (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) binaryField;
  9: MyI64 longField;
}

@hack.Adapter{name = '\Adapter1'}
typedef Foo FooWithAdapter

struct Bar {
  @hack.Adapter{name = '\Adapter1'}
  @cpp.Adapter{name = 'my::Adapter1', adaptedType = 'my::Cpp::Type1'}
  1: Foo (py.adapter = 'my.Adapter1') structField;
  @hack.Adapter{name = '\Adapter1'}
  2: optional Foo (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) optionalStructField;
  3: list<
    FooWithAdapter (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1')
  > structListField;
  4: optional list<
    FooWithAdapter (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1')
  > optionalStructListField;
  @hack.Adapter{name = '\Adapter1'}
  5: Baz (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1') unionField;
  @hack.Adapter{name = '\Adapter1'}
  6: optional Baz (
    cpp.adapter = 'my::Adapter1',
    py.adapter = 'my.Adapter1',
  ) optionalUnionField;
}

struct StructWithFieldAdapter {
  @cpp.Adapter{name = "my::Adapter1"}
  @python.Adapter{name = "my.Adapter1", typeHint = "my.AdaptedType1"}
  1: i32 field;
  @cpp.Adapter{name = "my::Adapter1"}
  @cpp.Ref{type = cpp.RefType.Shared}
  2: i32 shared_field;
  @cpp.Adapter{name = "my::Adapter1"}
  @cpp.Ref{type = cpp.RefType.Shared}
  3: optional i32 opt_shared_field;
  @cpp.Adapter{name = "my::Adapter1"}
  @thrift.Box
  4: optional i32 opt_boxed_field;
}

@hack.Adapter{name = '\Adapter2'}
typedef Bar (
  cpp.adapter = 'my::Adapter2',
  py.adapter = 'my.Adapter2',
) StructWithAdapter

@hack.Adapter{name = '\Adapter2'}
typedef Baz (
  cpp.adapter = 'my::Adapter2',
  py.adapter = 'my.Adapter2',
) UnionWithAdapter

struct B {
  1: AdaptedA a;
}
@cpp.Adapter{name = "my::Adapter"}
typedef A AdaptedA
struct A {}

service Service {
  MyI32 (cpp.adapter = 'my::Adapter1', py.adapter = 'my.Adapter1') func(
    1: StringWithAdapter (
      cpp.adapter = 'my::Adapter2',
      py.adapter = 'my.Adapter2',
    ) arg1,
    @cpp.Adapter{name = "my::Adapter2"}
    2: string arg2,
    3: Foo arg3,
  );
}
