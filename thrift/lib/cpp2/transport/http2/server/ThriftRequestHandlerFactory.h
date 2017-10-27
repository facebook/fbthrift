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

#include <proxygen/httpserver/RequestHandlerFactory.h>

#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandler.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * The factory that is provided to the HTTPServer object inside
 * H2ThriftServer which is used to create ThriftRequestHandler objects
 * for each incoming stream.  Proxygen creates a single factory object
 * which is used for all requests.
 */
class ThriftRequestHandlerFactory : public proxygen::RequestHandlerFactory {
 public:
  explicit ThriftRequestHandlerFactory(ThriftProcessor* processor)
      : processor_(processor) {}

  ~ThriftRequestHandlerFactory() override = default;

  void onServerStart(folly::EventBase* /*evb*/) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage*) noexcept override {
    LOG(FATAL) << "This method is now deprecated";
    return new ThriftRequestHandler(processor_);
  }

 private:
  // There is a single ThriftProcessor object which is used for all requests.
  // Owned by H2ThriftServer.
  ThriftProcessor* processor_;
};

} // namespace thrift
} // namespace apache
