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

#include "thrift/perf/cpp/AsyncClientWorker2.h"

#include "thrift/lib/cpp/ClientUtil.h"
#include "thrift/lib/cpp/test/loadgen/RNG.h"
#include "thrift//perf/cpp/ClientLoadConfig.h"
#include "thrift/lib/cpp/protocol/THeaderProtocol.h"
#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp2/async/HeaderClientChannel.h"
#include "thrift/lib/cpp2/async/RequestChannel.h"
#include "thrift/lib/cpp/test/loadgen/ScoreBoard.h"

#include <queue>
using namespace boost;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using namespace apache::thrift;
using apache::thrift::loadgen::ScoreBoard;

namespace apache { namespace thrift {

typedef std::shared_ptr<AsyncClientWorker2::Client> LoadTestClientPtr;

const int kTimeout = 60000;
const int MAX_LOOPS = 0;
const int MAX_MESSAGE_SIZE = 64;

class OpData {
public:
  OpData()
      : opType_(0),
        a(0),
        b(0),
        code(0)
    {}

  uint32_t opType_;
  int64_t a;
  int64_t b;
  int32_t code;
};

class LoadCallback;

class LoopTerminator {
 public:
  explicit LoopTerminator(TEventBase* base)
  : ops_(0)
  ,base_(base) {}

  void incr() {
    ops_++;
  }
  void decr() {
    ops_--;
    if (ops_ == 0) {
      base_->terminateLoopSoon();
    }
  }
 private:
  uint32_t ops_;
  TEventBase* base_;
};

/* This class drives the async load for a single connection */
class AsyncRunner2
{
public:
  friend class LoadCallback;

  AsyncRunner2(
    const std::shared_ptr<apache::thrift::test::ClientLoadConfig>& config,
    LoopTerminator& terminator,
    const std::shared_ptr<ScoreBoard>& scoreboard,
    const LoadTestClientPtr& client,
    int n_ops, int n_async)
      : terminator_(terminator),
        config_(config),
        scoreboard_(scoreboard),
        client_(client),
        ctr_(n_ops),
        n_outstanding_(0),
        n_async_(n_async)
    {
    }

  ~AsyncRunner2()
    {
    }

  void startRun() {
    for (int i = 0; i < n_async_ * 2; i++) {
      terminator_.incr();
      performAsyncOperation();
    }
  }

  LoopTerminator& terminator_;

private:
  void genericCob(LoadTestAsyncClient *client,
                  ClientReceiveState&& rstate,
                  OpData* opData);

  void performAsyncOperation();

  const std::shared_ptr<apache::thrift::test::ClientLoadConfig>& config_;
  const std::shared_ptr<ScoreBoard>& scoreboard_;
  std::shared_ptr<LoadTestAsyncClient> client_;

  int ctr_;
  int n_outstanding_;
  int n_async_;
};

class LoadCallback :
      public RequestCallback {
 public:

  LoadCallback(AsyncRunner2* r, LoadTestAsyncClient* client)
      : r_(r)
      , client_(client)
      , oneway_(false) {
  }

  void setOneway() {
    oneway_ = true;
  }

  void requestSent() {
    if (oneway_) {
      r_->genericCob(client_, ClientReceiveState(), &data_);
    }
  }

  void replyReceived(ClientReceiveState&& rstate) {
    r_->genericCob(client_, std::move(rstate), &data_);
  }

  void requestError(ClientReceiveState&& rstate) {
    r_->terminator_.decr();
    r_->scoreboard_->opFailed(data_.opType_);
  }

  OpData data_;

 private:
  AsyncRunner2* r_;
  LoadTestAsyncClient* client_;
  bool oneway_;
};

facebook::servicerouter::cpp2::ClientFactory
AsyncClientWorker2::srClientFactory_;

LoadTestClientPtr AsyncClientWorker2::createConnection() {
  const std::shared_ptr<apache::thrift::test::ClientLoadConfig>& config =
    getConfig();
  if (config->useSR()) {
    std::shared_ptr<LoadTestAsyncClient> loadClient;
    if (config->SASLPolicy() != "required" &&
        config->SASLPolicy() != "permitted") {
      loadClient.reset(srClientFactory_.getClient<LoadTestAsyncClient>(
        config->srTier(), &eb_));
    } else {
      facebook::servicerouter::ConnConfigs configs;
      configs["thrift_security"] = config->SASLPolicy();
      if (!config->SASLServiceTier().empty()) {
        configs["thrift_security_service_tier"] = config->SASLServiceTier();
      }
      loadClient.reset(srClientFactory_.getClient<LoadTestAsyncClient>(
        config->srTier(),
        &eb_,
        facebook::servicerouter::ServiceOptions(),
        configs));
    }
    return loadClient;
  } else {
    std::shared_ptr<TAsyncSocket> socket =
      TAsyncSocket::newSocket(&eb_, *config->getAddress(), kTimeout);

    LoadTestAsyncClient *loadClient = nullptr;
    std::unique_ptr<
      apache::thrift::HeaderClientChannel,
      apache::thrift::async::TDelayedDestruction::Destructor> channel(
        HeaderClientChannel::newChannel(socket));
    channel->setTimeout(kTimeout);

    if (config->SASLPolicy() == "permitted") {
      channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (config->SASLPolicy() == "required") {
      channel->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    }

    if (config->SASLPolicy() == "required" ||
        config->SASLPolicy() == "permitted") {
      channel->getSaslClient()->setServiceIdentity(
        folly::format(
          "{}@{}",
          config->SASLServiceTier(),
          config->getAddressHostname()).str());
    }

    loadClient = new LoadTestAsyncClient(std::move(channel));
    return std::shared_ptr<LoadTestAsyncClient>(loadClient);
  }
}

void
AsyncClientWorker2::run() {
  int loopCount = 0;
  std::list<AsyncRunner2 *> clients;
  std::list<AsyncRunner2 *>::iterator it;
  LoopTerminator loopTerminator(&eb_);
  do {
    // Create a new connection
    int n_clients = getConfig()->getAsyncClients();
    // Determine how many operations to perform on this connection
    uint32_t nops = getConfig()->pickOpsPerConnection();
    // How many outstanding ops at a time
    int n_async = getConfig()->getAsyncOpsPerClient();

    for (int i = 0; i < n_clients; i++) {
      std::shared_ptr<LoadTestAsyncClient> client;
      try {
        client = createConnection();
        AsyncRunner2 *r = new AsyncRunner2(getConfig(), loopTerminator,
                                           getScoreBoard(),
                                           client, nops, n_async);
        clients.push_back(r);
      } catch (const std::exception& ex) {
        ErrorAction action = handleConnError(ex);
        if (action == EA_CONTINUE || action == EA_NEXT_CONNECTION) {
          // continue the next connection loop
          continue;
        } else if (action == EA_DROP_THREAD) {
          T_ERROR("worker %d exiting after connection error", getID());
          stopWorker();
          return;
        } else if (action == EA_ABORT) {
          T_ERROR("worker %d causing abort after connection error", getID());
          abort();
        } else {
          T_ERROR("worker %d received unknown conn error action %d; aborting",
                  getID(), action);
          abort();
        }
      }
    }
    for (auto r : clients) {
      r->startRun();
    }

    eb_.loopForever();

    for (auto r : clients) {
      delete r;
    }
    clients.clear();

  }
  while (MAX_LOOPS == 0 || ++loopCount < MAX_LOOPS);

  stopWorker();
}

void AsyncRunner2::performAsyncOperation() {
  // We don't support throttled QPS right now.  The right way to do
  // this requires an AsyncIntervalTimer

  LoadCallback* cb =     new LoadCallback(this, client_.get());
  std::unique_ptr<RequestCallback> recvCob(cb);

  uint32_t opType = config_->pickOpType();
  scoreboard_->opStarted(opType);

  OpData *d = &cb->data_;

  d->opType_ = opType;
  n_outstanding_++;
  ctr_--;

  switch (static_cast<apache::thrift::test::ClientLoadConfig::OperationEnum>(
            opType)) {
    case apache::thrift::test::ClientLoadConfig::OP_NOOP:
      return client_->noop(std::move(recvCob));
    case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_NOOP:
      cb->setOneway();
      return client_->onewayNoop(std::move(recvCob));
    case apache::thrift::test::ClientLoadConfig::OP_ASYNC_NOOP:
      return client_->asyncNoop(std::move(recvCob));
    case apache::thrift::test::ClientLoadConfig::OP_SLEEP:
      return client_->sleep(std::move(recvCob), config_->pickSleepUsec());
    case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_SLEEP:
      cb->setOneway();
      return client_->onewaySleep(std::move(recvCob), config_->pickSleepUsec());
    case apache::thrift::test::ClientLoadConfig::OP_BURN:
      return client_->burn(std::move(recvCob), config_->pickBurnUsec());
    case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_BURN:
      cb->setOneway();
      return client_->onewayBurn(std::move(recvCob), config_->pickBurnUsec());
    case apache::thrift::test::ClientLoadConfig::OP_BAD_SLEEP:
      return client_->badSleep(std::move(recvCob), config_->pickSleepUsec());
    case apache::thrift::test::ClientLoadConfig::OP_BAD_BURN:
      return client_->badBurn(std::move(recvCob), config_->pickBurnUsec());
    case apache::thrift::test::ClientLoadConfig::OP_THROW_ERROR:
      d->code = loadgen::RNG::getU32();
      return client_->throwError(std::move(recvCob), d->code);
    case apache::thrift::test::ClientLoadConfig::OP_THROW_UNEXPECTED:
      return client_->throwUnexpected(
        std::move(recvCob), loadgen::RNG::getU32());
    case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_THROW:
      cb->setOneway();
      return client_->onewayThrow(std::move(recvCob), loadgen::RNG::getU32());
    case apache::thrift::test::ClientLoadConfig::OP_SEND:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->send(std::move(recvCob), str);
    }
    case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_SEND:
    {
      std::string str(config_->pickSendSize(), 'a');
      cb->setOneway();
      return client_->onewaySend(std::move(recvCob), str);
    }
    case apache::thrift::test::ClientLoadConfig::OP_RECV:
      return client_->recv(std::move(recvCob), config_->pickRecvSize());
    case apache::thrift::test::ClientLoadConfig::OP_SENDRECV:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->sendrecv(
        std::move(recvCob), str, config_->pickRecvSize());
    }
    case apache::thrift::test::ClientLoadConfig::OP_ECHO:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->echo(std::move(recvCob), str);
    }
    case apache::thrift::test::ClientLoadConfig::OP_ADD:
    {
      boost::uniform_int<int64_t> distribution;
      d->a = distribution(loadgen::RNG::getRNG());
      d->b = distribution(loadgen::RNG::getRNG());

      return client_->add(std::move(recvCob), d->a, d->b);
    }
    case apache::thrift::test::ClientLoadConfig::NUM_OPS:
      // fall through
      break;
      // no default case, so gcc will warn us if a new op is added
      // and this switch statement is not updated
  }

  T_ERROR("AsyncClientWorker2::performOperation() unknown operation %" PRIu32,
          opType);
  assert(false);
}

void AsyncRunner2::genericCob(LoadTestAsyncClient *client,
                              ClientReceiveState&& rstate,
                              OpData *opData)
{
  int64_t int64_result;
  std::string string_result;

  n_outstanding_--;

  try {
    switch (static_cast<apache::thrift::test::ClientLoadConfig::OperationEnum>(
              opData->opType_)) {
      case apache::thrift::test::ClientLoadConfig::OP_NOOP:
        client->recv_noop(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_NOOP:
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ASYNC_NOOP:
        client->recv_asyncNoop(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_SLEEP:
        client->recv_sleep(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_SLEEP:
        break;
      case apache::thrift::test::ClientLoadConfig::OP_BURN:
        client->recv_burn(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_BURN:
        break;
      case apache::thrift::test::ClientLoadConfig::OP_BAD_SLEEP:
        client->recv_badSleep(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_BAD_BURN:
        client->recv_badBurn(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_THROW_ERROR:
        try {
          client->recv_throwError(rstate);
          T_ERROR("throwError() didn't throw any exception");
        } catch (const LoadError& error) {
          assert(error.code == opData->code);
        }
        break;
      case apache::thrift::test::ClientLoadConfig::OP_THROW_UNEXPECTED:
        try {
          client->recv_throwUnexpected(rstate);
        } catch (const TApplicationException& error) {
          // expected; do nothing
        }
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_THROW:
        break;
      case apache::thrift::test::ClientLoadConfig::OP_SEND:
        client->recv_send(rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ONEWAY_SEND:
        break;
      case apache::thrift::test::ClientLoadConfig::OP_RECV:
        client->recv_recv(string_result, rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_SENDRECV:
        client->recv_sendrecv(string_result, rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ECHO:
        client->recv_echo(string_result, rstate);
        break;
      case apache::thrift::test::ClientLoadConfig::OP_ADD:
        int64_result = client->recv_add(rstate);
        if (int64_result != opData->a + opData->b) {
          T_ERROR("add(%" PRId64 ", %" PRId64 " gave wrong result %" PRId64
                  "(expected %" PRId64 ")", opData->a, opData->b, int64_result,
                  opData->a + opData->b);
        }
        break;
      case apache::thrift::test::ClientLoadConfig::NUM_OPS:
        // fall through
        break;
        // no default case, so gcc will warn us if a new op is added
        // and this switch statement is not updated
    }
  } catch (const std::exception &ex) {
    terminator_.decr();
    T_ERROR("Unexpected exception: %s", ex.what());
    scoreboard_->opFailed(opData->opType_);
    // don't launch anymore
    return;
  }

  scoreboard_->opSucceeded(opData->opType_);

  if (ctr_ > 0) {
    // launch more ops, attempt to keep between 1x and 2x n_async_ ops
    // outstanding
    if (n_outstanding_ <= n_async_) {
      for (int i = 0; i < n_async_; i++) {
        terminator_.incr();
        performAsyncOperation();
      }
    }
  }
  terminator_.decr();
}


}} // apache::thrift
