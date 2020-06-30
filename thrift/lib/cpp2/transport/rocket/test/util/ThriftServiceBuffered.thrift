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

namespace cpp2 testutil.testservice

include "thrift/lib/cpp2/transport/rocket/test/util/ThriftService.thrift"

service StreamServiceBuffered {
  // Echo input value
  i32 echo(1: i32 value);

  // Generate numbers between `from` to `to`.
  stream<i32> range(1: i32 from, 2: i32 to);
  // ... with a sleep in between iterations
  stream<i32> slowRange(1: i32 from, 2: i32 to, 3: i32 millis);
  stream<ThriftService.Message throws (1: ThriftService.FirstEx e)>
    streamThrows(1: i32 whichEx) throws (1: ThriftService.SecondEx e);

  i32, stream<i32> leakCheck(1: i32 from, 2: i32 to);
  i32, stream<i32> leakCheckWithSleep(1: i32 from, 2: i32 to, 3: i32 sleepMs);
  i32 instanceCount();
}
