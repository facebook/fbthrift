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

#include <folly/GLog.h>
#include <folly/synchronization/RelaxedAtomic.h>
#include <thrift/lib/cpp2/server/ServerFlags.h>

THRIFT_FLAG_DEFINE_bool(experimental_use_resource_pools, false);
DEFINE_bool(
    thrift_experimental_use_resource_pools,
    false,
    "Use experimental resource pools");

namespace apache::thrift {

namespace {
folly::relaxed_atomic<bool>& getResourcePoolsRuntimeDisabled() {
  static folly::relaxed_atomic<bool> resourcePoolsRuntimeDisabled{false};
  return resourcePoolsRuntimeDisabled;
}

folly::relaxed_atomic<bool>& getResourcePoolsRuntimeRequested() {
  static folly::relaxed_atomic<bool> resourcePoolsRuntimeRequested{false};
  return resourcePoolsRuntimeRequested;
}

} // namespace

void runtimeDisableResourcePools() {
  getResourcePoolsRuntimeDisabled().store(true);
}

void requireResourcePools() {
  getResourcePoolsRuntimeRequested().store(true);
  // Call this to ensure setting is fixed and we detect conflicts going forward.
  CHECK_EQ(useResourcePools(), true);
}

bool useResourcePools() {
  // If Gflag is turned on, we will just ignore the rest enablements
  static bool gFlag = FLAGS_thrift_experimental_use_resource_pools;
  static bool thriftFlag = THRIFT_FLAG(experimental_use_resource_pools);
  static bool firstResult = gFlag ||
      ((thriftFlag || getResourcePoolsRuntimeRequested().load()) &&
       !getResourcePoolsRuntimeDisabled().load());
  bool result = gFlag ||
      ((thriftFlag || getResourcePoolsRuntimeRequested().load()) &&
       !getResourcePoolsRuntimeDisabled().load());
  if (result == firstResult) {
    return result;
  }
  LOG(ERROR) << "Inconsistent results from useResourcePools";
  return firstResult;
}

bool useResourcePoolsFlagsSet() {
  return useResourcePools();
}

} // namespace apache::thrift
