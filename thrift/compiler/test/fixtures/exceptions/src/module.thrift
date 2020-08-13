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

namespace java.swift test.fixtures.exceptions

exception Banal { }
exception Fiery {
  1: required string message,
} (message = "message")
exception Serious {
  1: optional string sonnet,
} (message = "sonnet")
exception ComplexFieldNames {
  1: string error_message,
  2: string internal_error_message
} (message = "internal_error_message")
exception CustomFieldNames {
  1: string error_message,
  2: string internal_error_message (java.swift.name = "internalGreatMessage")
} (message = "internal_error_message")

service Raiser {
  void doBland(),
  void doRaise() throws (1: Banal b, 2: Fiery f, 3: Serious s),
  string get200(),
  string get500() throws (1: Fiery f, 2: Banal b, 3: Serious s),
}
