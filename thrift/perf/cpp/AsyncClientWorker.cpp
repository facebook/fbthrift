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

#include <thrift/perf/cpp/AsyncClientWorker.h>

#include <thrift/lib/cpp/ClientUtil.h>
#include <thrift/lib/cpp/test/loadgen/RNG.h>
#include <thrift/perf/cpp/ClientLoadConfig.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <thrift/lib/cpp/async/TFramedAsyncChannel.h>
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>
#include <thrift/lib/cpp/async/TBinaryAsyncChannel.h>
#include <thrift/lib/cpp/test/loadgen/ScoreBoard.h>

#include <queue>
using namespace boost;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::async;
using apache::thrift::loadgen::ScoreBoard;

namespace apache { namespace thrift { namespace test {

typedef std::shared_ptr<AsyncClientWorker::Client> LoadTestClientPtr;

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


/* This class drives the async load for a single connection */
class AsyncRunner
{
public:
  class Creator : public TAsyncSocket::ConnectCallback {
   public:
    Creator(AsyncClientWorker *worker, std::shared_ptr<TAsyncSocket> socket)
        : worker_(worker),
          socket_(socket) {}

    void connectSuccess() noexcept override {
      worker_->createRunner(socket_);
      delete this;
    }
    void connectError(
        const transport::TTransportException& ex) noexcept override {
      delete this;
    }
   private:
    AsyncClientWorker *worker_;
    std::shared_ptr<TAsyncSocket> socket_;
  };

  AsyncRunner(const std::shared_ptr<ClientLoadConfig>& config,
              const std::shared_ptr<ScoreBoard>& scoreboard,
              const LoadTestClientPtr& client,
              int n_ops, int n_async)
      : config_(config),
        scoreboard_(scoreboard),
        client_(client),
        ctr_(n_ops),
        n_outstanding_(0),
        n_async_(n_async),
        q_size_(n_async * 2 + 1),
        readPos_(0),
        writePos_(0)
    {
      recvCob = std::bind(&AsyncRunner::genericCob, this, client_.get());
      // Wrote my own queue to avoid malloc on every op
      outstandingOps_ = new OpData[q_size_];
      client->_resizeSendBuffer(q_size_ * MAX_MESSAGE_SIZE);
    }

  ~AsyncRunner()
    {
      delete []outstandingOps_;
    }

  void startRun() {
    for (int i = 0; i < n_async_ * 2; i++) {
      performAsyncOperation();
    }
  }

  void stop() {
    // This is a workaround to the broken destruction chain of Cob clients
    // and their channels.  If you have pending callbacks and decide to delete
    // the client, the client arg in the Cob is invalid.
    //
    // explicitly shut the channel down before destroying
    std::shared_ptr<TAsyncChannel> channel = client_->getChannel();
    if (config_->useHeaderProtocol()) {
      static_cast<THeaderAsyncChannel *>(channel.get())->closeNow();
    } else if (config_->useFramedTransport()) {
      static_cast<TFramedAsyncChannel *>(channel.get())->closeNow();
    } else {
      static_cast<TBinaryAsyncChannel *>(channel.get())->closeNow();
    }
  }


private:
  void performAsyncOperation();

  void genericCob(LoadTestCobClient *client);

  const std::shared_ptr<ClientLoadConfig>& config_;
  const std::shared_ptr<ScoreBoard>& scoreboard_;
  std::shared_ptr<LoadTestCobClient> client_;

  std::function<void(LoadTestCobClient* client)> recvCob;

  int ctr_;
  int n_outstanding_;
  int n_async_;
  OpData *curOpData_;

  // Outstanding operation queue for this connection
  OpData *outstandingOps_;
  int q_size_;
  int readPos_;
  int writePos_;
};


std::shared_ptr<TAsyncSocket> AsyncClientWorker::createSocket() {
  const std::shared_ptr<ClientLoadConfig>& config = getConfig();

  std::shared_ptr<TAsyncSocket> socket;
  if (config->useSSL()) {
    std::shared_ptr<SSLContext> context(new SSLContext());
    socket = TAsyncSSLSocket::newSocket(context, &eb_);
  } else {
    socket = TAsyncSocket::newSocket(&eb_);
  }
  socket->setSendTimeout(kTimeout);
  socket->connect(new AsyncRunner::Creator(this, socket), *config->getAddress(),
                  kTimeout);
  return socket;
}

LoadTestClientPtr AsyncClientWorker::createConnection(
  std::shared_ptr<TAsyncSocket> socket) {
  const std::shared_ptr<ClientLoadConfig>& config = getConfig();

  LoadTestCobClient *loadClient = nullptr;
  if (config->useHeaderProtocol()) {
    std::shared_ptr<THeaderAsyncChannel> channel(
      THeaderAsyncChannel::newChannel(socket));
    channel->setRecvTimeout(kTimeout);
    if (config->zlib()) {
      duplexProtoFactory_.setTransform(THeader::ZLIB_TRANSFORM);
    }
    loadClient = new LoadTestCobClient(channel, &duplexProtoFactory_);
  } else if (config->useFramedTransport()) {
    std::shared_ptr<TFramedAsyncChannel> channel(
      TFramedAsyncChannel::newChannel(socket));
    channel->setRecvTimeout(kTimeout);
    loadClient = new LoadTestCobClient(channel, &binProtoFactory_);
  } else {
    std::shared_ptr<TBinaryAsyncChannel> channel(
      TBinaryAsyncChannel::newChannel(socket));
    channel->setRecvTimeout(kTimeout);
    loadClient = new LoadTestCobClient(channel, &binProtoFactory_);
  }
  loadClient->_disableSequenceIdChecks();
  return std::shared_ptr<LoadTestCobClient>(loadClient);
}

AsyncRunner *AsyncClientWorker::createRunner(std::shared_ptr<TAsyncSocket> socket) {
  LoadTestClientPtr client = createConnection(socket);
  AsyncRunner *r = new AsyncRunner(getConfig(), getScoreBoard(),
                                   client, getConfig()->pickOpsPerConnection(),
                                   getConfig()->getAsyncOpsPerClient());
  clients_.push_back(r);
  r->startRun();
  return r;
}

void
AsyncClientWorker::run() {
  int loopCount = 0;
  std::list<AsyncRunner *> clients;
  std::list<AsyncRunner *>::iterator it;
  do {
    // Create a new connection
    int n_clients = getConfig()->getAsyncClients();
    // Determine how many operations to perform on this connection

    for (int i = 0; i < n_clients; i++) {
      std::shared_ptr<TAsyncSocket> socket;
      try {
        socket = createSocket();
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

    eb_.loop();

    for (it = clients_.begin(); it != clients_.end(); ++it) {
      AsyncRunner *r = *it;
      r->stop();
      delete r;
    }
    clients_.clear();

  }
  while (MAX_LOOPS == 0 || ++loopCount < MAX_LOOPS);

  stopWorker();
}

void AsyncRunner::performAsyncOperation() {
  // We don't support throttled QPS right now.  The right way to do
  // this requires an AsyncIntervalTimer

  uint32_t opType = config_->pickOpType();
  scoreboard_->opStarted(opType);

  OpData *d = &(outstandingOps_[writePos_]);
  writePos_ = (writePos_ + 1) % (q_size_);

  d->opType_ = opType;
  n_outstanding_++;
  ctr_--;
  curOpData_ = d;

  switch (static_cast<ClientLoadConfig::OperationEnum>(opType)) {
    case ClientLoadConfig::OP_NOOP:
      return client_->noop(recvCob);
    case ClientLoadConfig::OP_ONEWAY_NOOP:
      return client_->onewayNoop(recvCob);
    case ClientLoadConfig::OP_ASYNC_NOOP:
      return client_->asyncNoop(recvCob);
    case ClientLoadConfig::OP_SLEEP:
      return client_->sleep(recvCob, config_->pickSleepUsec());
    case ClientLoadConfig::OP_ONEWAY_SLEEP:
      return client_->onewaySleep(recvCob, config_->pickSleepUsec());
    case ClientLoadConfig::OP_BURN:
      return client_->burn(recvCob, config_->pickBurnUsec());
    case ClientLoadConfig::OP_ONEWAY_BURN:
      return client_->onewayBurn(recvCob, config_->pickBurnUsec());
    case ClientLoadConfig::OP_BAD_SLEEP:
      return client_->badSleep(recvCob, config_->pickSleepUsec());
    case ClientLoadConfig::OP_BAD_BURN:
      return client_->badBurn(recvCob, config_->pickBurnUsec());
    case ClientLoadConfig::OP_THROW_ERROR:
      d->code = loadgen::RNG::getU32();
      return client_->throwError(recvCob, d->code);
    case ClientLoadConfig::OP_THROW_UNEXPECTED:
      return client_->throwUnexpected(recvCob, loadgen::RNG::getU32());
    case ClientLoadConfig::OP_ONEWAY_THROW:
      return client_->onewayThrow(recvCob, loadgen::RNG::getU32());
    case ClientLoadConfig::OP_SEND:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->send(recvCob, str);
    }
    case ClientLoadConfig::OP_ONEWAY_SEND:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->onewaySend(recvCob, str);
    }
    case ClientLoadConfig::OP_RECV:
      return client_->recv(recvCob, config_->pickRecvSize());
    case ClientLoadConfig::OP_SENDRECV:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->sendrecv(recvCob, str, config_->pickRecvSize());
    }
    case ClientLoadConfig::OP_ECHO:
    {
      std::string str(config_->pickSendSize(), 'a');
      return client_->echo(recvCob, str);
    }
    case ClientLoadConfig::OP_ADD:
    {
      boost::uniform_int<int64_t> distribution;
      curOpData_->a = distribution(loadgen::RNG::getRNG());
      curOpData_->b = distribution(loadgen::RNG::getRNG());

      return client_->add(recvCob, curOpData_->a, curOpData_->b);
    }
    case ClientLoadConfig::NUM_OPS:
      // fall through
      break;
      // no default case, so gcc will warn us if a new op is added
      // and this switch statement is not updated
  }

  T_ERROR("AsyncClientWorker::performOperation() got unknown operation %"
          PRIu32, opType);
  assert(false);
}

void AsyncRunner::genericCob(LoadTestCobClient *client)
{
  int64_t int64_result;
  std::string string_result;

  n_outstanding_--;
  assert(readPos_ != writePos_);
  OpData *opData = &(outstandingOps_[readPos_]);
  readPos_ = (readPos_ + 1) % (q_size_);
  curOpData_ = opData;

  if (!client->getChannel()->good()) {
    T_ERROR("bad channel");
    scoreboard_->opFailed(curOpData_->opType_);
    // there was some fancy error handling - bah
    return;
  }

  try {
    switch (static_cast<ClientLoadConfig::OperationEnum>(opData->opType_)) {
      case ClientLoadConfig::OP_NOOP:
        client->recv_noop();
        break;
      case ClientLoadConfig::OP_ONEWAY_NOOP:
        break;
      case ClientLoadConfig::OP_ASYNC_NOOP:
        client->recv_asyncNoop();
        break;
      case ClientLoadConfig::OP_SLEEP:
        client->recv_sleep();
        break;
      case ClientLoadConfig::OP_ONEWAY_SLEEP:
        break;
      case ClientLoadConfig::OP_BURN:
        client->recv_burn();
        break;
      case ClientLoadConfig::OP_ONEWAY_BURN:
        break;
      case ClientLoadConfig::OP_BAD_SLEEP:
        client->recv_badSleep();
        break;
      case ClientLoadConfig::OP_BAD_BURN:
        client->recv_badBurn();
        break;
      case ClientLoadConfig::OP_THROW_ERROR:
        try {
          client->recv_throwError();
          T_ERROR("throwError() didn't throw any exception");
        } catch (const LoadError& error) {
          assert(error.code == opData->code);
        }
        break;
      case ClientLoadConfig::OP_THROW_UNEXPECTED:
        try {
          client->recv_throwUnexpected();
        } catch (const TApplicationException& error) {
          // expected; do nothing
        }
        break;
      case ClientLoadConfig::OP_ONEWAY_THROW:
        break;
      case ClientLoadConfig::OP_SEND:
        client->recv_send();
        break;
      case ClientLoadConfig::OP_ONEWAY_SEND:
        break;
      case ClientLoadConfig::OP_RECV:
        client->recv_recv(string_result);
        break;
      case ClientLoadConfig::OP_SENDRECV:
        client->recv_sendrecv(string_result);
        break;
      case ClientLoadConfig::OP_ECHO:
        client->recv_echo(string_result);
        break;
      case ClientLoadConfig::OP_ADD:
        int64_result = client->recv_add();
        if (int64_result != opData->a + opData->b) {
          T_ERROR("add(%" PRId64 ", %" PRId64 " gave wrong result %" PRId64
                  "(expected %" PRId64 ")", opData->a, opData->b, int64_result,
                  opData->a + opData->b);
        }
        break;
      case ClientLoadConfig::NUM_OPS:
        // fall through
        break;
        // no default case, so gcc will warn us if a new op is added
        // and this switch statement is not updated
    }
  } catch (const std::exception &ex) {
    T_ERROR("Unexpected exception: %s", ex.what());
    scoreboard_->opFailed(opData->opType_);
    // don't launch anymore
    return;
  }

  scoreboard_->opSucceeded(curOpData_->opType_);

  if (ctr_ > 0) {
    // launch more ops, attempt to keep between 1x and 2x n_async_ ops
    // outstanding
    if (n_outstanding_ <= n_async_) {
      for (int i = 0; i < n_async_; i++) {
        performAsyncOperation();
      }
    }
  }
}


}}} // apache::thrift::test
