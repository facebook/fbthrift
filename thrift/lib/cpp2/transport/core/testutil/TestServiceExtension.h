/*
 * Copyright 2004-present Facebook, Inc.
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

#include <folly/Synchronized.h>
#include <yarpl/Flowable.h>
#include <vector>

#include "thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h"

// Extension functions & classes for server side to enable calling
// 'helloChannel' function from the server side.

namespace testutil {
namespace testservice {

class TestServiceAsyncProcessorExtension;

class TestServiceExtensionSvIf : public TestServiceSvIf {
 public:
  typedef TestServiceAsyncProcessorExtension ProcessorType;

  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor();

  // Utility functions
  std::unique_ptr<folly::IOBuf> encodeString(const std::string& result);
  std::string decodeString(std::unique_ptr<folly::IOBuf> buffer);

  virtual yarpl::Reference<yarpl::flowable::Flowable<std::string>> helloChannel(
      yarpl::Reference<yarpl::flowable::Flowable<std::string>> input);
  folly::Future<yarpl::Reference<yarpl::flowable::Flowable<std::string>>>
  future_helloChannel(
      yarpl::Reference<yarpl::flowable::Flowable<std::string>> input);
  void async_tm_helloChannel(
      std::unique_ptr<apache::thrift::HandlerCallback<void>> callback);
};

class TestServiceAsyncProcessorExtension : public TestServiceAsyncProcessor {
 protected:
  // overrides the instance in the parent class..
  TestServiceExtensionSvIf* iface_;

 public:
  explicit TestServiceAsyncProcessorExtension(TestServiceExtensionSvIf* iface)
      : TestServiceAsyncProcessor(iface),
        // Keep one more instance,  so no need to cast etc, already the compiler
        // generated code will not cast etc.
        iface_(iface) {}

  // Trick to add the new methods without touching the compiler generated ones.
  // We will not need this function when we do the compiler updates.
  static void injectNewMethods();

 private:
  // HelloChannel
  template <typename ProtocolIn_, typename ProtocolOut_>
  void _processInThread_helloChannel(
      std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ProtocolIn_> iprot,
      apache::thrift::Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void process_helloChannel(
      std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
      std::unique_ptr<folly::IOBuf> buf,
      std::unique_ptr<ProtocolIn_> iprot,
      apache::thrift::Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm);
  template <class ProtocolIn_, class ProtocolOut_>
  static folly::IOBufQueue return_helloChannel(
      int32_t protoSeqId,
      apache::thrift::ContextStack* ctx);
  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped_helloChannel(
      std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
      int32_t protoSeqId,
      apache::thrift::ContextStack* ctx,
      folly::exception_wrapper ew,
      apache::thrift::Cpp2RequestContext* reqCtx);
};
} // namespace testservice
} // namespace testutil
