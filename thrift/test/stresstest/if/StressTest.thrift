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

namespace cpp2 apache.thrift.stress

enum ProcessingMode {
  Default = 0, // execute inline in handler (synchronously)
  Async = 1, // execute asynchronously in handler
}

enum WorkSimulationMode {
  Default = 0, // simulate work by busy waiting
  Sleep = 1, // simulate work by sleeping
}

struct ProcessInfo {
  1: i64 processingTimeMs;
  2: ProcessingMode processingMode;
  3: WorkSimulationMode workSimulationMode;
  4: i64 responseSize;
}

struct BasicRequest {
  1: ProcessInfo processInfo;
  2: binary payload;
}

struct BasicResponse {
  1: binary payload;
}

service StressTest {
  void ping() (thread = "eb");

  BasicResponse requestResponseTm(1: BasicRequest req);
  BasicResponse requestResponseEb(1: BasicRequest req) (thread = "eb");
}
