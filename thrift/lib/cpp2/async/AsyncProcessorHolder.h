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

#pragma once

#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache::thrift {

/**
 * AsyncProcessorHolder implements the AsyncProcessorSet interface
 * (getRequestsProcessor) and contains a single AsyncProcessor. It is used for
 * the request routers which have a single processor but are interacted with by
 * the transport code through the AsyncProcessorSet interface.
 */
class AsyncProcessorHolderFactory final : public AsyncProcessorFactory {
 public:
  explicit AsyncProcessorHolderFactory(
      std::shared_ptr<AsyncProcessorFactory> processorFactory);

  std::unique_ptr<AsyncProcessor> getProcessor() override;
  CreateMethodMetadataResult createMethodMetadata() override;

  std::shared_ptr<folly::RequestContext> getBaseContextForRequest(
      const MethodMetadata&) override;
  std::vector<ServiceHandlerBase*> getServiceHandlers() override;

  std::optional<std::reference_wrapper<ServiceRequestInfoMap const>>
  getServiceRequestInfoMap() const override;

 private:
  const std::shared_ptr<AsyncProcessorFactory> processorFactory_;
};

} // namespace apache::thrift
