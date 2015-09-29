/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <thrift/tutorial/cpp/stateful/AuthHandler.h>
#include <thrift/tutorial/cpp/stateful/gen-cpp2/ShellService.h>

namespace apache { namespace thrift { namespace tutorial { namespace stateful {

//  Unusual, but we want a per-connection handler. We can do that because
//  ThriftServer only cares about having a global AsyncProcessorFactory, from
//  which it gets a per-connection AsyncProcessor. The AsyncProcessor can do
//  whatever it likes.
class ShellHandler : virtual public ShellServiceSvIf, public AuthHandler {
 public:
  ShellHandler(std::shared_ptr<ServiceAuthState> serviceAuthState,
               apache::thrift::server::TConnectionContext* ctx);
  ~ShellHandler() override;

  void pwd(std::string& _return) override;
  void chdir(const std::string& dir) override;
  void listDirectory(std::vector<StatInfo>& _return,
                     const std::string& dir) override;
  void cat(std::string& _return, const std::string& file) override;

 protected:
  void validateState();
  void throwErrno(const char* msg);

  std::mutex mutex_;
  int cwd_;
};

class ShellHandlerFactory : public apache::thrift::AsyncProcessorFactory {
 public:
  explicit ShellHandlerFactory(std::shared_ptr<ServiceAuthState> authState) :
      authState_(move(authState)) {}

  //  Our custom AsyncProcessor creates a per-connection handler. In our case,
  //  the handler's ctor wants a TConnectionContext, but
  //  AsyncProcessorFactory::getProcessor doesn't have a way to pass one in.
  //  So we create the handler on the first call to AsyncProcessor::process.
  //  Note that all this happens in the io thread specific to the conenction,
  //  so there is no need to do any concurrency control here.
  class CustomProcessor : public ShellServiceAsyncProcessor {
   public:
    explicit CustomProcessor(
        std::shared_ptr<ServiceAuthState> serviceAuthState) :
        ShellServiceAsyncProcessor(nullptr),
        serviceAuthState_(std::move(serviceAuthState)) {}
    void process(
        std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
        std::unique_ptr<folly::IOBuf> buf,
        apache::thrift::protocol::PROTOCOL_TYPES protType,
        apache::thrift::Cpp2RequestContext* context,
        folly::EventBase* eb,
        apache::thrift::concurrency::ThreadManager* tm) override {
      if (!handler_) {
        handler_ = folly::make_unique<ShellHandler>(
            std::move(serviceAuthState_), context->getConnectionContext());
      }
      if (!iface_) {
        AuthenticatedServiceAsyncProcessor::iface_ = handler_.get();
        ShellServiceAsyncProcessor::iface_ = handler_.get();
      }
      ShellServiceAsyncProcessor::process(
          std::move(req), std::move(buf), protType, context, eb, tm);
    }
   private:
    std::shared_ptr<ServiceAuthState> serviceAuthState_;
    std::unique_ptr<ShellHandler> handler_;
  };

  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return folly::make_unique<CustomProcessor>(authState_);
  }

 protected:
  std::shared_ptr<ServiceAuthState> authState_;
};

}}}}
