/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/async/AsyncClient.h>

#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>

namespace apache {
namespace thrift {

// TODO: make it possible to create a connection context from a thrift channel
GeneratedAsyncClient::GeneratedAsyncClient(
    std::shared_ptr<RequestChannel> channel)
    : connectionContext_(std::make_unique<Cpp2ConnContext>()),
      channel_(std::move(channel)) {}

GeneratedAsyncClient::~GeneratedAsyncClient() {}

} // namespace thrift
} // namespace apache
