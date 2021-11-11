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

include "thrift/annotation/hack.thrift"
include "thrift/annotation/meta.thrift"

@meta.Transitive
@hack.ExperimentalAdapter{
  name = "\MyFieldAdapter",
  adapted_generic_type = "\MyFieldWrapper",
}
struct AnnotaionStruct {}

struct MyStruct {
  @hack.ExperimentalAdapter{
    name = "\MyFieldAdapter",
    adapted_generic_type = "\MyFieldWrapper",
  }
  1: i64 wrapped_field;
  @AnnotaionStruct
  2: i64 annotated_field;
  3: i64 (hack.adapter = "\MyAdapter") adapted_type;
  @hack.ExperimentalAdapter{name = "\MyFieldAdapter"}
  4: i64 adapted_field;
}

union MyUnion {
  @AnnotaionStruct
  1: i64 union_annotated_field;
  3: i64 (hack.adapter = "\AdapterTestIntToString") union_adapted_type;
}

exception MyException {
  1: i64 code;
  2: string message;
  @AnnotaionStruct
  3: string annotated_message;
}

service Service {
  i32 func(1: string arg1, 2: MyStruct arg2);
}
