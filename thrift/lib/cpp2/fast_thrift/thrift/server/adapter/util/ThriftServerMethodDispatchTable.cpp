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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/util/ThriftServerMethodDispatchTable.h>

namespace apache::thrift::fast_thrift::thrift {

ThriftServerMethodDispatchTable::ThriftServerMethodDispatchTable(
    std::initializer_list<Method> methods) {
  methods_.reserve(methods.size());
  for (const auto& method : methods) {
    methods_.insert_or_assign(std::string(method.name), method.dispatch);
  }
}

ThriftServerMethodDispatchTable::ThriftServerMethodDispatchTable(
    const std::vector<Method>& methods) {
  methods_.reserve(methods.size());
  for (const auto& method : methods) {
    methods_.insert_or_assign(std::string(method.name), method.dispatch);
  }
}

} // namespace apache::thrift::fast_thrift::thrift
