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

#pragma once

#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/transport/core/StreamRequestCallback.h>
#include <thrift/lib/cpp2/transport/core/ThriftClient.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

// This class extends the capabilities of ThriftClient with Stream Support.
// When we want to add Stream Support to the core we can just copy paste the
// functions to the parent class.
class StreamThriftClient : public ThriftClient {
 public:
  using Ptr =
      std::unique_ptr<ThriftClient, folly::DelayedDestruction::Destructor>;

  StreamThriftClient(
      std::shared_ptr<ClientConnectionIf> connection,
      folly::EventBase* callbackEvb);

  uint32_t sendRequestSync(
      RpcOptions&,
      std::unique_ptr<RequestCallback>,
      std::unique_ptr<apache::thrift::ContextStack>,
      std::unique_ptr<folly::IOBuf>,
      std::shared_ptr<apache::thrift::transport::THeader>) override;

  uint32_t sendRequest(
      RpcOptions& rpcOptions,
      std::unique_ptr<RequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header) override;

 protected:
  uint32_t sendStreamRequestHelper(
      RpcOptions& rpcOptions,
      std::unique_ptr<StreamRequestCallback> cb,
      std::unique_ptr<ContextStack> ctx,
      std::unique_ptr<folly::IOBuf> buf,
      std::shared_ptr<apache::thrift::transport::THeader> header,
      folly::EventBase* callbackEvb);
};

} // namespace thrift
} // namespace apache
