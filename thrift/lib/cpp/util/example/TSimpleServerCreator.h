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
#ifndef THRIFT_UTIL_TSIMPLESERVERCREATOR_H_
#define THRIFT_UTIL_TSIMPLESERVERCREATOR_H_ 1

#include <thrift/lib/cpp/util/SyncServerCreator.h>

namespace apache { namespace thrift {

namespace server {
class TSimpleServer;
}

namespace util {

class __attribute__((__deprecated__)) TSimpleServerCreator : public SyncServerCreator {
 public:
  // "Deprecated factory for TSimpleServer"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  typedef server::TSimpleServer ServerType;
  #pragma GCC diagnostic pop

  /**
   * Create a new TSimpleServerCreator.
   */
  TSimpleServerCreator(const std::shared_ptr<TProcessor>& processor,
                       uint16_t port,
                       bool framed = true)
    : SyncServerCreator(processor, port, framed) {}

  /**
   * Create a new TSimpleServerCreator.
   */
  TSimpleServerCreator(const std::shared_ptr<TProcessor>& processor,
                       uint16_t port,
                       std::shared_ptr<transport::TTransportFactory>& tf,
                       std::shared_ptr<protocol::TProtocolFactory>& pf)
    : SyncServerCreator(processor, port, tf, pf) {}

  std::shared_ptr<server::TServer> createServer() override;

  // "Deprecated factory for TSimpleServer"
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
  std::shared_ptr<server::TSimpleServer> createSimpleServer();
  #pragma GCC diagnostic pop
};

}}} // apache::thrift::util

#endif // THRIFT_UTIL_TSIMPLESERVERCREATOR_H_
