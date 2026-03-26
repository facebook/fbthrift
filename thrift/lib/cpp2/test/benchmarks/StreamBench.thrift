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

namespace cpp2 apache.thrift.benchmark

package "facebook.com/thrift/test/benchmarks/stream"

service StreamBenchService {
  stream<i32> range(1: i32 from, 2: i32 to);
  stream<binary> rangePayload(1: i32 count, 2: i32 payloadBytes);
  stream<i32> rangeWithWork(1: i32 from, 2: i32 to, 3: i32 workIterations);
}
