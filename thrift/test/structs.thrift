/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace cpp2 apache.thrift.test

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
