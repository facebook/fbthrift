/*
 * Copyright 2014 Facebook, Inc.
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

#define __STDC_FORMAT_MACROS

#include "thrift/perf/cpp/ClientWorker2.h"

#include "thrift/lib/cpp/ClientUtil.h"
#include "thrift/lib/cpp/test/loadgen/RNG.h"
#include "thrift/perf/cpp/ClientLoadConfig.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp2/async/GssSaslClient.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/security/KerberosSASLThreadManager.h"
#include "thrift/lib/cpp2/security/SecurityLogger.h"
#include "thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h"

using namespace boost;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace apache::thrift;

namespace apache { namespace thrift { namespace test {

const int kTimeout = 60000;

std::shared_ptr<ClientWorker2::Client> ClientWorker2::createConnection() {
  const std::shared_ptr<ClientLoadConfig>& config = getConfig();
  std::shared_ptr<TAsyncSocket> socket;
  std::unique_ptr<
    RequestChannel,
    apache::thrift::async::TDelayedDestruction::Destructor> channel;
  if (config->useSR()) {
    facebook::servicerouter::ConnConfigs configs;
    facebook::servicerouter::ServiceOptions options;
    configs["sock_sendtimeout"] = folly::to<std::string>(kTimeout);
    configs["sock_recvtimeout"] = folly::to<std::string>(kTimeout);
    if (config->SASLPolicy() == "required" ||
        config->SASLPolicy() == "permitted") {
      configs["thrift_security"] = config->SASLPolicy();
      if (!config->SASLServiceTier().empty()) {
        configs["thrift_security_service_tier"] = config->SASLServiceTier();
      }
    }
    channel = std::move(facebook::servicerouter::cpp2::getClientFactory()
                                        .getChannel(config->srTier(),
                                                    ebm_.getEventBase(),
                                                    std::move(options),
                                                    std::move(configs)));
  } else {
    if (config->useSSL()) {
      std::shared_ptr<SSLContext> context = std::make_shared<SSLContext>();
      socket = TAsyncSSLSocket::newSocket(context, ebm_.getEventBase());
      socket->connect(nullptr, *config->getAddress());
      // Loop once to connect
      ebm_.getEventBase()->loop();
    } else {
      socket =
        TAsyncSocket::newSocket(ebm_.getEventBase(),
                                *config->getAddress());
    }
    std::unique_ptr<
      HeaderClientChannel,
      apache::thrift::async::TDelayedDestruction::Destructor> headerChannel(
          HeaderClientChannel::newChannel(socket));
    // Always use binary in loadtesting to get apples to apples comparison
    headerChannel->getHeader()->setProtocolId(
        apache::thrift::protocol::T_BINARY_PROTOCOL);
    if (config->zlib()) {
      headerChannel->getHeader()->setTransform(THeader::ZLIB_TRANSFORM);
    }
    if (!config->useHeaderProtocol()) {
      headerChannel->getHeader()->setClientType(THRIFT_FRAMED_DEPRECATED);
    }
    headerChannel->setTimeout(kTimeout);
    if (config->SASLPolicy() == "permitted") {
      headerChannel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (config->SASLPolicy() == "required") {
      headerChannel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    }

    if (config->SASLPolicy() == "required" ||
        config->SASLPolicy() == "permitted") {
      static auto saslThreadManager_ = std::make_shared<SaslThreadManager>();
      static auto ccManagerLogger_ = std::make_shared<SecurityLogger>();
      static auto credentialsCacheManager_ =
        std::make_shared<krb5::Krb5CredentialsCacheManager>(
          "", ccManagerLogger_);
      headerChannel->setSaslClient(std::unique_ptr<apache::thrift::SaslClient>(
        new apache::thrift::GssSaslClient(socket->getEventBase())
      ));
      headerChannel->getSaslClient()->setSaslThreadManager(saslThreadManager_);
      headerChannel->getSaslClient()->setCredentialsCacheManager(
        credentialsCacheManager_);
      headerChannel->getSaslClient()->setServiceIdentity(
        folly::format(
          "{}@{}",
          config->SASLServiceTier(),
          config->getAddressHostname()).str());
    }
    channel = std::move(headerChannel);
  }

  return std::shared_ptr<ClientWorker2::Client>(
    new ClientWorker2::Client(std::move(channel)));
}

void ClientWorker2::performOperation(const std::shared_ptr<Client>& client,
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

  T_ERROR("ClientWorker2::performOperation() got unknown operation %" PRIu32,
          opType);
  assert(false);
}

void ClientWorker2::performNoop(const std::shared_ptr<Client>& client) {
  client->sync_noop();
}

void ClientWorker2::performOnewayNoop(const std::shared_ptr<Client>& client) {
  client->sync_onewayNoop();
}

void ClientWorker2::performAsyncNoop(const std::shared_ptr<Client>& client) {
  client->sync_asyncNoop();
}

void ClientWorker2::performSleep(const std::shared_ptr<Client>& client) {
  client->sync_sleep(getConfig()->pickSleepUsec());
}

void ClientWorker2::performOnewaySleep(const std::shared_ptr<Client>& client) {
  client->sync_onewaySleep(getConfig()->pickSleepUsec());
}

void ClientWorker2::performBurn(const std::shared_ptr<Client>& client) {
  client->sync_burn(getConfig()->pickBurnUsec());
}

void ClientWorker2::performOnewayBurn(const std::shared_ptr<Client>& client) {
  client->sync_onewayBurn(getConfig()->pickBurnUsec());
}

void ClientWorker2::performBadSleep(const std::shared_ptr<Client>& client) {
  client->sync_badSleep(getConfig()->pickSleepUsec());
}

void ClientWorker2::performBadBurn(const std::shared_ptr<Client>& client) {
  client->sync_badBurn(getConfig()->pickBurnUsec());
}

void ClientWorker2::performThrowError(const std::shared_ptr<Client>& client) {
  uint32_t code = loadgen::RNG::getU32();
  try {
    client->sync_throwError(code);
    T_ERROR("throwError() didn't throw any exception");
  } catch (const LoadError& error) {
    assert(error.code == code);
  }
}

void ClientWorker2::performThrowUnexpected(const std::shared_ptr<Client>& client) {
  try {
    client->sync_throwUnexpected(loadgen::RNG::getU32());
    T_ERROR("throwUnexpected() didn't throw any exception");
  } catch (const TApplicationException& error) {
    // expected; do nothing
  }
}

void ClientWorker2::performOnewayThrow(const std::shared_ptr<Client>& client) {
  client->sync_onewayThrow(loadgen::RNG::getU32());
}

void ClientWorker2::performSend(const std::shared_ptr<Client>& client) {
  std::string str(getConfig()->pickSendSize(), 'a');
  client->sync_send(str);
}

void ClientWorker2::performOnewaySend(const std::shared_ptr<Client>& client) {
  std::string str(getConfig()->pickSendSize(), 'a');
  client->sync_onewaySend(str);
}

void ClientWorker2::performRecv(const std::shared_ptr<Client>& client) {
  std::string str;
  client->sync_recv(str, getConfig()->pickRecvSize());
}

void ClientWorker2::performSendrecv(const std::shared_ptr<Client>& client) {
  std::string sendStr(getConfig()->pickSendSize(), 'a');
  std::string recvStr;
  client->sync_sendrecv(recvStr, sendStr, getConfig()->pickRecvSize());
}

void ClientWorker2::performEcho(const std::shared_ptr<Client>& client) {
  std::string sendStr(getConfig()->pickSendSize(), 'a');
  std::string recvStr;
  client->sync_echo(recvStr, sendStr);
}

void ClientWorker2::performAdd(const std::shared_ptr<Client>& client) {
  boost::uniform_int<int64_t> distribution;
  int64_t a = distribution(loadgen::RNG::getRNG());
  int64_t b = distribution(loadgen::RNG::getRNG());

  int64_t result = client->sync_add(a, b);

  if (result != a + b) {
    T_ERROR("add(%" PRId64 ", %" PRId64 " gave wrong result %" PRId64
            "(expected %" PRId64 ")", a, b, result, a + b);
  }
}

}}} // apache::thrift::test
