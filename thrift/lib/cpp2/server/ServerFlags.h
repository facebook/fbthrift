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

#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp2/Flags.h>

THRIFT_FLAG_DECLARE_bool(experimental_use_resource_pools);
DECLARE_bool(thrift_experimental_use_resource_pools);

namespace apache::thrift {

FOLLY_ALWAYS_INLINE bool useResourcePoolsFlagsSet() {
  return THRIFT_FLAG(experimental_use_resource_pools) ||
      FLAGS_thrift_experimental_use_resource_pools;
}

} // namespace apache::thrift
