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

include "thrift/test/structs.thrift"

namespace cpp2 apache.thrift.test

struct BasicRefsSharedTerseWrites {
  1: optional structs.HasInt def_field (cpp.ref);
  2: optional structs.HasInt shared_field (cpp.ref_type = "shared");
  3: optional list<structs.HasInt> shared_fields (cpp.ref_type = "shared");
  4: optional structs.HasInt shared_field_const (cpp.ref_type = "shared_const");
  5: optional list<structs.HasInt> shared_fields_const (
    cpp.ref_type = "shared_const",
  );
  6: structs.HasInt shared_field_req (cpp.ref_type = "shared");
  7: list<structs.HasInt> shared_fields_req (cpp.ref_type = "shared");
}
