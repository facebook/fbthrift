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
#include <thrift/lib/cpp2/transport/rsocket/server/RSResponder.h>

#include <thrift/lib/cpp2/transport/rsocket/server/RequestResponseThriftChannel.h>

namespace apache {
namespace thrift {

RSResponder::RSResponder(ThriftProcessor* processor) : processor_(processor) {
  evb_ = folly::EventBaseManager::get()->getExistingEventBase();
  CHECK(evb_);
}

yarpl::Reference<yarpl::single::Single<rsocket::Payload>>
RSResponder::handleRequestResponse(
    rsocket::Payload request,
    rsocket::StreamId streamId) {
  return yarpl::single::Single<std::unique_ptr<folly::IOBuf>>::create(
             [ this, request = std::move(request), streamId ](
                 auto subscriber) mutable {
               LOG(INFO) << "RSResponder.handleRequestResponse : " << request;

               auto functionInfo = std::make_unique<FunctionInfo>();
               functionInfo->seqId = streamId;
               functionInfo->kind =
                   FunctionKind::SINGLE_REQUEST_SINGLE_RESPONSE;

               auto headers =
                   std::make_unique<std::map<std::string, std::string>>();
               auto channel = std::make_shared<RequestResponseThriftChannel>(
                   evb_, subscriber);
               processor_->onThriftRequest(
                   std::move(functionInfo),
                   std::move(headers),
                   std::move(request.data),
                   channel);

             })
      ->map([](auto buff) { return rsocket::Payload(std::move(buff)); });
}
}
}
