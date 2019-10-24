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

struct NoDefaults {
  1: required i64 req_field
  2: i64 unflagged_field
  3: optional i64 opt_field
  4: optional list<string> strings
}

struct WithDefaults {
  1: required i64 req_field = 10
  2: i64 unflagged_field = 20
  3: optional i64 opt_field = 30
}

service NullService {
}
