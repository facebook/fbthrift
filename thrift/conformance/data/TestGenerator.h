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

#pragma once

#include <thrift/conformance/data/internal/TestGenerator.h>

#include <set>

#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/if/gen-cpp2/test_suite_types.h>

namespace apache::thrift::conformance::data {

TestSuite createRoundTripSuite(
    const std::set<Protocol>& protocols,
    const AnyRegistry& registry = AnyRegistry::generated());

inline TestSuite createRoundTripSuite(
    const std::set<StandardProtocol>& protocols = detail::kDefaultProtocols,
    const AnyRegistry& registry = AnyRegistry::generated()) {
  return createRoundTripSuite(detail::toProtocols(protocols), registry);
}

inline TestSuite createRoundTripSuite(const AnyRegistry& registry) {
  return createRoundTripSuite(detail::kDefaultProtocols, registry);
}

} // namespace apache::thrift::conformance::data
