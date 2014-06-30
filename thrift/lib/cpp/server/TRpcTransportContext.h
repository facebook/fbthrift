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
#ifndef THRIFT_SERVER_TRPCTRANSPORTCONTEXT_H_
#define THRIFT_SERVER_TRPCTRANSPORTCONTEXT_H_ 1

#include <thrift/lib/cpp/server/TConnectionContext.h>

#include <memory>

namespace apache { namespace thrift {

namespace transport {
class TRpcTransport;
}

namespace server {

class TRpcTransportContext : public TConnectionContext {
 public:
  TRpcTransportContext(std::shared_ptr<transport::TRpcTransport> transport)
    : transport_(transport) {}

  TRpcTransportContext(
      const std::shared_ptr<transport::TRpcTransport>& transport,
      const std::shared_ptr<protocol::TProtocol>& iprot,
      const std::shared_ptr<protocol::TProtocol>& oprot)
    : transport_(transport),
      iprot_(iprot),
      oprot_(oprot) {}

  virtual const transport::TSocketAddress* getPeerAddress() const;

  const std::shared_ptr<transport::TRpcTransport>& getTransport() const {
    return transport_;
  }

  virtual std::shared_ptr<protocol::TProtocol> getInputProtocol() const {
    return iprot_;
  }

  virtual std::shared_ptr<protocol::TProtocol> getOutputProtocol() const {
    return oprot_;
  }

 private:
  std::shared_ptr<transport::TRpcTransport> transport_;
  std::shared_ptr<protocol::TProtocol> iprot_;
  std::shared_ptr<protocol::TProtocol> oprot_;
};

}}} // apache::thrift::server

#endif // THRIFT_SERVER_TRPCTRANSPORTCONTEXT_H_
