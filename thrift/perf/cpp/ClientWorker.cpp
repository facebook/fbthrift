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
#define __STDC_FORMAT_MACROS

#include "thrift/perf/cpp/ClientWorker.h"

#include "thrift/lib/cpp/ClientUtil.h"
#include "thrift/lib/cpp/test/loadgen/RNG.h"
#include "thrift/perf/cpp/ClientLoadConfig.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"
#include "thrift/lib/cpp/transport/TSSLSocket.h"

using namespace boost;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace apache { namespace thrift { namespace test {

template<typename ClientT, typename ProtocolT, typename TransportT>
ClientT* createClient(
  const std::shared_ptr<transport::SSLContext> &ctx,
  const transport::TSocketAddress* address) {
  std::shared_ptr<transport::TSSLSocket> socket(
    new transport::TSSLSocket(ctx, address->getAddressStr(),
                              address->getPort()));
  // We could specialize this to not create a wrapper transport when
  // TransportT is TTransport or TSocket.  However, everyone should always
  // use a TFramedTransport or TBufferedTransport wrapper for performance
  // reasons.
  std::shared_ptr<TransportT> transport(new TransportT(socket));
  std::shared_ptr<ProtocolT> protocol( new ProtocolT(transport));
  transport->open();

  return new ClientT(protocol);
}

template<typename ClientT, typename ProtocolT, typename TransportT>
std::shared_ptr<ClientT> createClientPtr(
  const std::shared_ptr<transport::SSLContext> &ctx,
  const transport::TSocketAddress* address) {
  return std::shared_ptr<ClientT>(
    createClient<ClientT, ProtocolT, TransportT>(ctx, address));
}



std::shared_ptr<ClientWorker::Client> ClientWorker::createConnection() {
  const std::shared_ptr<ClientLoadConfig>& config = getConfig();
  if (config->useSSL()) {
    std::shared_ptr<SSLContext> context(new SSLContext());
    if (config->useHeaderProtocol()) {
      std::shared_ptr<ClientWorker::Client> client;
      std::shared_ptr<THeaderProtocol> protocol;
      client = createClientPtr<Client, THeaderProtocol, TBufferedTransport>(
        context, config->getAddress());
      protocol = std::dynamic_pointer_cast<THeaderProtocol>(
        client->getOutputProtocol());
      protocol->setProtocolId(T_BINARY_PROTOCOL);
      return client;
    } else {
      if (config->useFramedTransport()) {
        return createClientPtr<Client, TBinaryProtocol, TFramedTransport>(
          context, config->getAddress());
      } else {
        return createClientPtr<Client, TBinaryProtocol, TBufferedTransport>(
          context, config->getAddress());
      }
    }
  } else {
    if (config->useHeaderProtocol()) {
      std::shared_ptr<ClientWorker::Client> client;
      std::shared_ptr<THeaderProtocol> protocol;
      client = util::createClientPtr<Client, THeaderProtocol>(
        config->getAddress(), false);
      protocol = std::dynamic_pointer_cast<THeaderProtocol>(
        client->getOutputProtocol());
      protocol->setProtocolId(T_BINARY_PROTOCOL);
      return client;
    } else {
      return util::createClientPtr<Client>(config->getAddress(),
                                           config->useFramedTransport());
    }
  }
}

void ClientWorker::performOperation(const std::shared_ptr<Client>& client,
                                    uint32_t opType) {
  switch (static_cast<ClientLoadConfig::OperationEnum>(opType)) {
    case ClientLoadConfig::OP_NOOP:
      return performNoop(client);
    case ClientLoadConfig::OP_ONEWAY_NOOP:
      return performOnewayNoop(client);
    case ClientLoadConfig::OP_ASYNC_NOOP:
      return performAsyncNoop(client);
    case ClientLoadConfig::OP_SLEEP:
      return performSleep(client);
    case ClientLoadConfig::OP_ONEWAY_SLEEP:
      return performOnewaySleep(client);
    case ClientLoadConfig::OP_BURN:
      return performBurn(client);
    case ClientLoadConfig::OP_ONEWAY_BURN:
      return performOnewayBurn(client);
    case ClientLoadConfig::OP_BAD_SLEEP:
      return performBadSleep(client);
    case ClientLoadConfig::OP_BAD_BURN:
      return performBadBurn(client);
    case ClientLoadConfig::OP_THROW_ERROR:
      return performThrowError(client);
    case ClientLoadConfig::OP_THROW_UNEXPECTED:
      return performThrowUnexpected(client);
    case ClientLoadConfig::OP_ONEWAY_THROW:
      return performOnewayThrow(client);
    case ClientLoadConfig::OP_SEND:
      return performSend(client);
    case ClientLoadConfig::OP_ONEWAY_SEND:
      return performOnewaySend(client);
    case ClientLoadConfig::OP_RECV:
      return performRecv(client);
    case ClientLoadConfig::OP_SENDRECV:
      return performSendrecv(client);
    case ClientLoadConfig::OP_ECHO:
      return performEcho(client);
    case ClientLoadConfig::OP_ADD:
      return performAdd(client);
    case ClientLoadConfig::NUM_OPS:
      // fall through
      break;
    // no default case, so gcc will warn us if a new op is added
    // and this switch statement is not updated
  }

  T_ERROR("ClientWorker::performOperation() got unknown operation %" PRIu32,
          opType);
  assert(false);
}

void ClientWorker::performNoop(const std::shared_ptr<Client>& client) {
  client->noop();
}

void ClientWorker::performOnewayNoop(const std::shared_ptr<Client>& client) {
  client->onewayNoop();
}

void ClientWorker::performAsyncNoop(const std::shared_ptr<Client>& client) {
  client->asyncNoop();
}

void ClientWorker::performSleep(const std::shared_ptr<Client>& client) {
  client->sleep(getConfig()->pickSleepUsec());
}

void ClientWorker::performOnewaySleep(const std::shared_ptr<Client>& client) {
  client->onewaySleep(getConfig()->pickSleepUsec());
}

void ClientWorker::performBurn(const std::shared_ptr<Client>& client) {
  client->burn(getConfig()->pickBurnUsec());
}

void ClientWorker::performOnewayBurn(const std::shared_ptr<Client>& client) {
  client->onewayBurn(getConfig()->pickBurnUsec());
}

void ClientWorker::performBadSleep(const std::shared_ptr<Client>& client) {
  client->badSleep(getConfig()->pickSleepUsec());
}

void ClientWorker::performBadBurn(const std::shared_ptr<Client>& client) {
  client->badBurn(getConfig()->pickBurnUsec());
}

void ClientWorker::performThrowError(const std::shared_ptr<Client>& client) {
  uint32_t code = loadgen::RNG::getU32();
  try {
    client->throwError(code);
    T_ERROR("throwError() didn't throw any exception");
  } catch (const LoadError& error) {
    assert(error.code == code);
  }
}

void ClientWorker::performThrowUnexpected(const std::shared_ptr<Client>& client) {
  try {
    client->throwUnexpected(loadgen::RNG::getU32());
    T_ERROR("throwUnexpected() didn't throw any exception");
  } catch (const TApplicationException& error) {
    // expected; do nothing
  }
}

void ClientWorker::performOnewayThrow(const std::shared_ptr<Client>& client) {
  client->onewayThrow(loadgen::RNG::getU32());
}

void ClientWorker::performSend(const std::shared_ptr<Client>& client) {
  std::string str(getConfig()->pickSendSize(), 'a');
  client->send(str);
}

void ClientWorker::performOnewaySend(const std::shared_ptr<Client>& client) {
  std::string str(getConfig()->pickSendSize(), 'a');
  client->onewaySend(str);
}

void ClientWorker::performRecv(const std::shared_ptr<Client>& client) {
  std::string str;
  client->recv(str, getConfig()->pickRecvSize());
}

void ClientWorker::performSendrecv(const std::shared_ptr<Client>& client) {
  std::string sendStr(getConfig()->pickSendSize(), 'a');
  std::string recvStr;
  client->sendrecv(recvStr, sendStr, getConfig()->pickRecvSize());
}

void ClientWorker::performEcho(const std::shared_ptr<Client>& client) {
  std::string sendStr(getConfig()->pickSendSize(), 'a');
  std::string recvStr;
  client->echo(recvStr, sendStr);
}

void ClientWorker::performAdd(const std::shared_ptr<Client>& client) {
  boost::uniform_int<int64_t> distribution;
  int64_t a = distribution(loadgen::RNG::getRNG());
  int64_t b = distribution(loadgen::RNG::getRNG());

  int64_t result = client->add(a, b);

  if (result != a + b) {
    T_ERROR("add(%" PRId64 ", %" PRId64 " gave wrong result %" PRId64
            "(expected %" PRId64 ")", a, b, result, a + b);
  }
}

}}} // apache::thrift::test
