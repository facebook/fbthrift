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

#include "common/base/Deprecation.h"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "thrift/lib/cpp/server/example/TThreadPoolServer.h"

#include "thrift/lib/cpp/concurrency/Thread.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp/server/TRpcTransportContext.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/lib/cpp/transport/TTransportException.h"

#include <string>
#include <iostream>

namespace apache { namespace thrift { namespace server {

using std::shared_ptr;
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;;
using namespace apache::thrift::transport;

class TThreadPoolServer::Task : public Runnable {

public:

  Task(TThreadPoolServer &server,
       shared_ptr<TProtocol> input,
       shared_ptr<TProtocol> output,
       shared_ptr<TRpcTransport> transport) :
    server_(server),
    processor_(),
    input_(input),
    output_(output),
    context_(transport, input, output) {
    processor_ = server_.getProcessorFactory()->getProcessor(&context_);
  }

  ~Task() {}

  void run() {
    server_.setCurrentTask(this);
    try {
      runImpl();
    } catch (...) {
      server_.clearCurrentTask();
      throw;
    }

    server_.clearCurrentTask();
  }

  TConnectionContext* getConnectionContext() {
    return &context_;
  }

 private:
  void runImpl() {
    std::shared_ptr<TServerEventHandler> eventHandler =
      server_.getEventHandler();
    if (eventHandler != nullptr) {
      eventHandler->newConnection(&context_);
    }
    try {
      for (;;) {
        if (!processor_->process(input_, output_, &context_) ||
            !input_->getTransport()->peek()) {
          break;
        }
      }
    } catch (const TTransportException& ttx) {
      // This is reasonably expected, client didn't send a full request so just
      // ignore him
      // string errStr = string("TThreadPoolServer client died: ") + ttx.what();
      // GlobalOutput(errStr.c_str());
    } catch (const std::exception& x) {
      GlobalOutput.printf("TThreadPoolServer exception %s: %s",
                          typeid(x).name(), x.what());
    } catch (...) {
      GlobalOutput("TThreadPoolServer, unexpected exception in "
                   "TThreadPoolServer::Task::run()");
    }

    if (eventHandler != nullptr) {
      eventHandler->connectionDestroyed(&context_);
    }

    try {
      input_->getTransport()->close();
    } catch (TTransportException& ttx) {
      string errStr = string("TThreadPoolServer input close failed: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    }
    try {
      if (output_->getTransport()->isOpen()) {
        output_->getTransport()->close();
      }
    } catch (TTransportException& ttx) {
      string errStr = string("TThreadPoolServer output close failed: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    }
  }

  TThreadPoolServer& server_;
  shared_ptr<TProcessor> processor_;
  shared_ptr<TProtocol> input_;
  shared_ptr<TProtocol> output_;
  TRpcTransportContext context_;
};

TThreadPoolServer::~TThreadPoolServer() {}

void TThreadPoolServer::serve() {
  shared_ptr<TRpcTransport> client;
  shared_ptr<TTransport> inputTransport;
  shared_ptr<TTransport> outputTransport;
  shared_ptr<TProtocol> inputProtocol;
  shared_ptr<TProtocol> outputProtocol;

  // Start the server listening
  serverTransport_->listen();

  // Run the preServe event
  if (eventHandler_ != nullptr) {
    TSocketAddress address;
    serverTransport_->getAddress(&address);
    eventHandler_->preServe(&address);
  }

  while (!stop_) {
    try {
      client.reset();
      inputTransport.reset();
      outputTransport.reset();
      inputProtocol.reset();
      outputProtocol.reset();

      // Fetch client from server
      client = serverTransport_->accept();

      // Make IO transports
      TTransportPair transports = duplexTransportFactory_->getTransport(client);
      inputTransport = transports.first;
      outputTransport = transports.second;
      TProtocolPair protocols = duplexProtocolFactory_->getProtocol(transports);
      inputProtocol = protocols.first;
      outputProtocol = protocols.second;

      // Add to threadmanager pool
      shared_ptr<TThreadPoolServer::Task> task(new TThreadPoolServer::Task(
            *this, inputProtocol, outputProtocol, client));
      threadManager_->add(task, timeout_);

    } catch (TTransportException& ttx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      if (!stop_ || ttx.getType() != TTransportException::INTERRUPTED) {
        string errStr = string("TThreadPoolServer: TServerTransport died on accept: ") + ttx.what();
        GlobalOutput(errStr.c_str());
      }
      continue;
    } catch (TException& tx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = string("TThreadPoolServer: Caught TException: ") + tx.what();
      GlobalOutput(errStr.c_str());
      continue;
    } catch (const string& s) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = "TThreadPoolServer: Unknown exception: " + s;
      GlobalOutput(errStr.c_str());
      break;
    }
  }

  // If stopped manually, join the existing threads
  if (stop_) {
    try {
      serverTransport_->close();
      threadManager_->join();
    } catch (TException &tx) {
      string errStr = string("TThreadPoolServer: Exception shutting down: ") + tx.what();
      GlobalOutput(errStr.c_str());
    }
    stop_ = false;
  }

}

int64_t TThreadPoolServer::getTimeout() const {
  return timeout_;
}

void TThreadPoolServer::setTimeout(int64_t value) {
  timeout_ = value;
}

TConnectionContext* TThreadPoolServer::getConnectionContext() const {
  Task* task = currentTask_.get();
  if (task) {
    return task->getConnectionContext();
  }
  return nullptr;
}

void TThreadPoolServer::setCurrentTask(Task* task) {
  assert(currentTask_.get() == nullptr);
  currentTask_.set(task);
}

void TThreadPoolServer::clearCurrentTask() {
  currentTask_.clear();
}

}}} // apache::thrift::server
