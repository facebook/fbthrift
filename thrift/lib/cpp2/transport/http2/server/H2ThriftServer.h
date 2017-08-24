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

#include <thrift/lib/cpp2/server/BaseThriftServer.h>

#include <proxygen/httpserver/HTTPServer.h>
#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>
#include <memory>

namespace apache {
namespace thrift {

/**
 * This is a thrift server that uses HTTP/2 to communicate with the
 * client.  The Proxygen library is used to support the HTTP/2
 * communication.
 */
class H2ThriftServer : public BaseThriftServer {
 public:
  H2ThriftServer() = default;
  ~H2ThriftServer() override = default;

  void serve() override;

  void stop() override;

  void stopListening() override;

  bool isOverloaded(const transport::THeader* header) override;

  uint64_t getNumDroppedConnections() const override;

 private:
  void setup();

  void initializeThreadManager();

  std::unique_ptr<ThriftProcessor> processor_;
  std::unique_ptr<proxygen::HTTPServer> server_;
};

} // namespace thrift
} // namespace apache
