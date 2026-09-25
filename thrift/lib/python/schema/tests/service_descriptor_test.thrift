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

package "thrift.com/python/schema/service_descriptor_test"

namespace py3 thrift.lib.python.schema.tests

safe permanent client exception TestException {
  1: string message;
}

struct TestAnnotation {
  1: string label;
}

struct Request {
  1: i32 value;
}

interaction TestInteraction {
  i32 getValue();
}

service BaseService {
  string inherited(1: string value);
}

@TestAnnotation{label = "service"}
service TestService extends BaseService {
  i32 add(
    @TestAnnotation{label = "param"}
    1: i32 a,
    2: i32 b,
  );
  void ping();
  string greet(1: Request request) throws (
    @TestAnnotation{label = "exception"}
    1: TestException error,
  );
  idempotent i32 idempotentCall();
  readonly i32 readOnlyCall();
  i32, stream<string> streamNames();
  sink<string, i32> collectStrings();
  sink<string>, stream<i32> bidiEcho();
  TestInteraction createInteraction();
  performs TestInteraction;
  # @lint-ignore THRIFTCHECKS avoid-oneway-method
  oneway void fireAndForget();
}
