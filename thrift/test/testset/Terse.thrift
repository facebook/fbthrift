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

package "apache.org/thrift/test/testset"

namespace py3 thrift.test
namespace py thrift.test.Terse
namespace py.asyncio thrift_asyncio.test.Terse
namespace java.swift org.apache.thrift.test
namespace go thrift.test.testset.Terse
namespace csharp Apache.Thrift.Test.Terse

include "thrift/annotation/thrift.thrift"

enum TerseEnum {
  zero = 0,
  one = 1,
}

struct TerseEnumStruct {
  @thrift.TerseWrite
  1: TerseEnum field;
}
