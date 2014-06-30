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
#include <thrift/lib/cpp/util/ServerCreatorBase.h>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>

using std::shared_ptr;
using apache::thrift::protocol::TProtocolFactory;
using apache::thrift::protocol::TSingleProtocolFactory;
using apache::thrift::protocol::TDuplexProtocolFactory;
using apache::thrift::protocol::TBinaryProtocolFactoryT;
using apache::thrift::server::TServer;
using apache::thrift::transport::TBufferBase;

namespace apache { namespace thrift { namespace util {

const bool ServerCreatorBase::DEFAULT_STRICT_READ;
const bool ServerCreatorBase::DEFAULT_STRICT_WRITE;
const int32_t ServerCreatorBase::DEFAULT_STRING_LIMIT;
const int32_t ServerCreatorBase::DEFAULT_CONTAINER_LIMIT;

ServerCreatorBase::ServerCreatorBase()
  : strictRead_(DEFAULT_STRICT_READ)
  , strictWrite_(DEFAULT_STRICT_WRITE)
  , stringLimit_(DEFAULT_STRING_LIMIT)
  , containerLimit_(DEFAULT_CONTAINER_LIMIT) {
}

void ServerCreatorBase::setProtocolFactory(
    const shared_ptr<TProtocolFactory>& protocolFactory) {
  protocolFactory_ = protocolFactory;
}

void ServerCreatorBase::setDuplexProtocolFactory(
    const shared_ptr<TDuplexProtocolFactory>& duplexProtocolFactory) {
  duplexProtocolFactory_ = duplexProtocolFactory;
}

void ServerCreatorBase::setStrictProtocol(bool strictRead, bool strictWrite) {
  strictRead_ = strictRead;
  strictWrite_ = strictWrite;
}

void ServerCreatorBase::setStringSizeLimit(int32_t stringLimit) {
  stringLimit_ = stringLimit;
}

void ServerCreatorBase::setContainerSizeLimit(int32_t containerLimit) {
  containerLimit_ = containerLimit;
}

void ServerCreatorBase::configureServer(const shared_ptr<TServer>& server) {
  // The TServerEventHandler shouldn't already be set
  assert(!server->getEventHandler());

  // Set the TServerEventHandler
  server->setServerEventHandler(serverEventHandler_);
}

shared_ptr<TProtocolFactory> ServerCreatorBase::getProtocolFactory() {
  // If the protocol factory was explicitly specified, use that
  if (protocolFactory_) {
    return protocolFactory_;
  }

  // Return a new TBinaryProtocolFactory, configured appropriately
  // Pretty much everyone should be using a TBufferBase for the transport.
  // If in the future a subclass wants to create a TBinaryProtocolFactory for
  // another transport type, we'll need to turn getProtocolFactory() into a
  // template method, and take the transport type as an argument.
  shared_ptr< TBinaryProtocolFactoryT<TBufferBase> > protocolFactory(
      new TBinaryProtocolFactoryT<TBufferBase>(stringLimit_, containerLimit_,
                                               strictRead_, strictWrite_));
  return protocolFactory;
}

shared_ptr<TDuplexProtocolFactory>
ServerCreatorBase::getDuplexProtocolFactory() {
  // If a duplex protocol factory was explicitly specified, use that
  if (duplexProtocolFactory_) {
    return duplexProtocolFactory_;
  }

  // Otherwise adapt the simplex protocol factory into a duplex protocol
  // factory using TSingleProtocolFactory adaptor.
  shared_ptr<TDuplexProtocolFactory> duplexFactoryAdaptor(
    new TSingleProtocolFactory<TProtocolFactory>(getProtocolFactory()));

  return duplexFactoryAdaptor;
}

}}} // apache::thrift::util
