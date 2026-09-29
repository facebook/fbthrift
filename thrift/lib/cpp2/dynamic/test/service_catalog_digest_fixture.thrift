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

// Services whose digests are the cross-language golden values in
// service_catalog_digest_expected_values.thrift.
package "facebook.com/thrift/service_catalog_digest_test"

namespace cpp2 apache.thrift.dynamic.test.service_catalog_digest_fixture
namespace py3 apache.thrift.dynamic.test

struct RichAnnotation {
  1: optional string label;
  2: optional bool enabled;
  3: optional byte byteValue;
  4: optional i16 i16Value;
  5: optional i32 i32Value;
  6: optional i64 i64Value;
  7: optional float floatValue;
  8: optional double doubleValue;
  9: optional binary data;
  10: optional list<string> tags;
  11: optional set<i32> levels;
  12: optional map<string, i64> weights;
}

safe transient server exception AppError {}

service Calculator {
  i32 subtract(1: i32 left, 2: i32 right);
  i32 add(1: i32 left, 2: i32 right);
}

@RichAnnotation{
  label = "runtime value",
  enabled = true,
  byteValue = -7,
  i16Value = -1234,
  i32Value = 123456,
  i64Value = 1234567890123,
  floatValue = 1.25,
  doubleValue = -2.5,
  data = "bin data",
  tags = ["alpha", "beta"],
  levels = [2, 1],
  weights = {"left": 10, "right": 20},
}
interaction CatalogGoldenSession {
  @RichAnnotation{
    label = "runtime value",
    enabled = true,
    byteValue = -7,
    i16Value = -1234,
    i32Value = 123456,
    i64Value = 1234567890123,
    floatValue = 1.25,
    doubleValue = -2.5,
    data = "bin data",
    tags = ["alpha", "beta"],
    levels = [2, 1],
    weights = {"left": 10, "right": 20},
  }
  readonly i64 get();
}

@RichAnnotation{
  label = "runtime value",
  enabled = true,
  byteValue = -7,
  i16Value = -1234,
  i32Value = 123456,
  i64Value = 1234567890123,
  floatValue = 1.25,
  doubleValue = -2.5,
  data = "bin data",
  tags = ["alpha", "beta"],
  levels = [2, 1],
  weights = {"left": 10, "right": 20},
}
service CatalogGolden {
  @RichAnnotation{
    label = "runtime value",
    enabled = true,
    byteValue = -7,
    i16Value = -1234,
    i32Value = 123456,
    i64Value = 1234567890123,
    floatValue = 1.25,
    doubleValue = -2.5,
    data = "bin data",
    tags = ["alpha", "beta"],
    levels = [2, 1],
    weights = {"left": 10, "right": 20},
  }
  idempotent CatalogGoldenSession makeSession(
    @RichAnnotation{
      label = "runtime value",
      enabled = true,
      byteValue = -7,
      i16Value = -1234,
      i32Value = 123456,
      i64Value = 1234567890123,
      floatValue = 1.25,
      doubleValue = -2.5,
      data = "bin data",
      tags = ["alpha", "beta"],
      levels = [2, 1],
      weights = {"left": 10, "right": 20},
    }
    1: i32 seed,
  );
  @RichAnnotation{
    label = "runtime value",
    enabled = true,
    byteValue = -7,
    i16Value = -1234,
    i32Value = 123456,
    i64Value = 1234567890123,
    floatValue = 1.25,
    doubleValue = -2.5,
    data = "bin data",
    tags = ["alpha", "beta"],
    levels = [2, 1],
    weights = {"left": 10, "right": 20},
  }
  sink<string>, stream<i32> observe();
  sink<i32, string> upload() throws (1: AppError appError);
  # @lint-ignore THRIFTCHECKS avoid-oneway-method
  oneway void notify();
}
