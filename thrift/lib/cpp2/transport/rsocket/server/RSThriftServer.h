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

#include <rsocket/rsocket.h>
#include <memory>

#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

namespace apache {
namespace thrift {

class RSThriftServer : public BaseThriftServer {
 public:
  RSThriftServer() = default;

  ~RSThriftServer() override;

  void serve() override;

  void stop() override;

  void stopListening() override;

  bool isOverloaded(const transport::THeader* header) override;

  uint64_t getNumDroppedConnections() const override;

  int getPort();

 private:
  void setup();

  void initializeThreadManager();

  // This is globally unique, so we will pass the raw pointer to everywhere
  std::unique_ptr<ThriftProcessor> thrift_;

  std::unique_ptr<rsocket::RSocketServer> server_;
};

} // namespace thrift
} // namespace apache
