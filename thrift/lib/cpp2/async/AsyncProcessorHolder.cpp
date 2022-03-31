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

#include <thrift/lib/cpp2/async/AsyncProcessorHolder.h>

namespace apache::thrift {

namespace {

class AsyncProcessorHolder final : public AsyncProcessor {
 public:
  // Currently we have to implement most of these as pass through to support the
  // non resource pools mode. Once this is gone we'll be able to simplify this a
  // lot.
  void processSerializedRequest(
      ResponseChannelRequest::UniquePtr req,
      SerializedRequest&& serializedRequest,
      protocol::PROTOCOL_TYPES protocolType,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm) override {
    processor_->processSerializedRequest(
        std::move(req),
        std::move(serializedRequest),
        protocolType,
        context,
        eb,
        tm);
  }

  void processSerializedCompressedRequestWithMetadata(
      ResponseChannelRequest::UniquePtr req,
      SerializedCompressedRequest&& serializedRequest,
      const MethodMetadata& untypedMethodMetadata,
      protocol::PROTOCOL_TYPES protocolType,
      Cpp2RequestContext* context,
      folly::EventBase* eb,
      concurrency::ThreadManager* tm) override {
    processor_->processSerializedCompressedRequestWithMetadata(
        std::move(req),
        std::move(serializedRequest),
        untypedMethodMetadata,
        protocolType,
        context,
        eb,
        tm);
  }

  void executeRequest(
      ServerRequest&& request,
      const AsyncProcessorFactory::MethodMetadata& methodMetadata) override {
    processor_->executeRequest(std::move(request), methodMetadata);
  }

  void getServiceMetadata(
      metadata::ThriftServiceMetadataResponse& actualResponse) override {
    return processor_->getServiceMetadata(actualResponse);
  }

  void terminateInteraction(
      int64_t id,
      Cpp2ConnContext& ctx,
      folly::EventBase& eb) noexcept override {
    processor_->terminateInteraction(id, ctx, eb);
  }

  void destroyAllInteractions(
      Cpp2ConnContext& ctx, folly::EventBase& eb) noexcept override {
    processor_->destroyAllInteractions(ctx, eb);
  }

  std::pair<AsyncProcessor*, const AsyncProcessorFactory::MethodMetadata*>
  getRequestsProcessor(
      const AsyncProcessorFactory::MethodMetadata& methodMetadata) override {
    return std::make_pair(processor_.get(), &methodMetadata);
  }

  explicit AsyncProcessorHolder(std::unique_ptr<AsyncProcessor>&& processor)
      : processor_(std::move(processor)) {
    DCHECK(processor_);
  }

 private:
  const std::unique_ptr<AsyncProcessor> processor_;
};

} // namespace

AsyncProcessorHolderFactory::AsyncProcessorHolderFactory(
    std::shared_ptr<AsyncProcessorFactory> processorFactory)
    : processorFactory_(processorFactory) {
  CHECK(processorFactory_);
}

std::unique_ptr<AsyncProcessor> AsyncProcessorHolderFactory::getProcessor() {
  return std::make_unique<AsyncProcessorHolder>(
      processorFactory_->getProcessor());
}

AsyncProcessorFactory::CreateMethodMetadataResult
AsyncProcessorHolderFactory::createMethodMetadata() {
  return processorFactory_->createMethodMetadata();
}

std::shared_ptr<folly::RequestContext>
AsyncProcessorHolderFactory::getBaseContextForRequest(
    const MethodMetadata& methodMetadata) {
  return processorFactory_->getBaseContextForRequest(methodMetadata);
}

std::vector<ServiceHandlerBase*>
AsyncProcessorHolderFactory::getServiceHandlers() {
  return processorFactory_->getServiceHandlers();
}

std::optional<std::reference_wrapper<ServiceRequestInfoMap const>>
AsyncProcessorHolderFactory::getServiceRequestInfoMap() const {
  return processorFactory_->getServiceRequestInfoMap();
}

} // namespace apache::thrift
