/*
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

struct StructContainingOptionalList {
  1 : optional list<i32> data;
}

struct StructContainingRequiredList {
  1 : required list<i32> data;
}

typedef i64 int64
struct JsonTypedefs {
 1: map<int64, int64> x,
 2: list<int64> y,
 3: set<int64> z,
 4: int64 w,
}
