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

#include <thrift/lib/py3/metadata.h>

namespace thrift {
namespace py3 {

void extractMetadataFromServiceContext(
    apache::thrift::metadata::ThriftMetadata& metadata,
    const apache::thrift::metadata::ThriftServiceContext& serviceContext) {
  if (!serviceContext.service_info_ref().has_value()) {
    return;
  }

  const auto& service = *serviceContext.service_info_ref();
  metadata.services_ref()->emplace(*service.name_ref(), service);
}

} // namespace py3
} // namespace thrift
