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
#include <thrift/lib/cpp/util/SyncServerCreator.h>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/TServerSocket.h>

using std::shared_ptr;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace apache { namespace thrift { namespace util {

const int SyncServerCreator::DEFAULT_RECEIVE_TIMEOUT;
const int SyncServerCreator::DEFAULT_SEND_TIMEOUT;
const int SyncServerCreator::DEFAULT_ACCEPT_TIMEOUT;
const int SyncServerCreator::DEFAULT_TCP_SEND_BUFFER;
const int SyncServerCreator::DEFAULT_TCP_RECEIVE_BUFFER;

SyncServerCreator::SyncServerCreator(const shared_ptr<TProcessor>& processor,
                                     uint16_t port,
                                     bool framed) {
  init(processor, port);

  if (framed) {
    transportFactory_.reset(new TFramedTransportFactory);
  } else {
    transportFactory_.reset(new TBufferedTransportFactory);
  }
}

SyncServerCreator::SyncServerCreator(const shared_ptr<TProcessor>& processor,
                                     uint16_t port,
                                     shared_ptr<TTransportFactory>& tf,
                                     shared_ptr<TProtocolFactory>& pf) {
  init(processor, port);

  transportFactory_ = tf;
  // We intentionally require the user to explicitly specify the
  // TProtocolFactory if they specify a TTransportFactory.  This way they can
  // pass in a protocol factory specialized for the correct transport type.
  // (Otherwise we'll default to one specialized for TBufferBase.)
  setProtocolFactory(pf);
}

void SyncServerCreator::init(const shared_ptr<TProcessor>& processor,
                             uint16_t port) {
  processor_ = processor;
  port_ = port;

  recvTimeout_ = DEFAULT_RECEIVE_TIMEOUT;
  sendTimeout_ = DEFAULT_SEND_TIMEOUT;
  acceptTimeout_ = DEFAULT_ACCEPT_TIMEOUT;
  tcpSendBuffer_ = DEFAULT_TCP_SEND_BUFFER;
  tcpRecvBuffer_ = DEFAULT_TCP_RECEIVE_BUFFER;
}

shared_ptr<TServerSocket> SyncServerCreator::createServerSocket() {
  shared_ptr<TServerSocket> socket(new TServerSocket(port_));
  if (sendTimeout_ >= 0) {
    socket->setSendTimeout(sendTimeout_);
  }
  if (recvTimeout_ >= 0) {
    socket->setRecvTimeout(recvTimeout_);
  }
  if (acceptTimeout_ >= 0) {
    socket->setAcceptTimeout(acceptTimeout_);
  }
  if (tcpSendBuffer_ >= 0) {
    socket->setTcpSendBuffer(tcpSendBuffer_);
  }
  if (tcpRecvBuffer_ >= 0) {
    socket->setTcpRecvBuffer(tcpRecvBuffer_);
  }

  return socket;
}

shared_ptr<TDuplexTransportFactory>
SyncServerCreator::getDuplexTransportFactory() {
    if (duplexTransportFactory_.get() != nullptr) {
      return duplexTransportFactory_;
    }

    duplexTransportFactory_.reset(
      new transport::TSingleTransportFactory<TTransportFactory>(transportFactory_));

    return duplexTransportFactory_;
  }

}}} // apache::thrift::util
