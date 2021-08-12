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

include "thrift/lib/thrift/annotation/scope.thrift"

namespace cpp2 apache.thrift.annotation
namespace py3 thrift.lib.thrift.annotation

enum RefType {
  Unique = 0,
  Shared = 1,
  SharedMutable = 2,
}

@scope.Field
struct Ref {
  1: RefType type;
}
