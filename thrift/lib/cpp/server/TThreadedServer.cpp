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

#include "thrift/lib/cpp/server/TThreadedServer.h"

#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/server/TRpcTransportContext.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/lib/cpp/transport/TTransportException.h"

#include <string>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

namespace apache { namespace thrift { namespace server {

using std::shared_ptr;
using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;

class TThreadedServer::Task: public Runnable {

public:

  Task(TThreadedServer& server,
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
      if (ttx.getType() != TTransportException::END_OF_FILE) {
        string errStr = string("TThreadedServer client died: ") + ttx.what();
        GlobalOutput(errStr.c_str());
      }
    } catch (const std::exception &x) {
      GlobalOutput.printf("TThreadedServer exception: %s: %s",
                          typeid(x).name(), x.what());
    } catch (...) {
      GlobalOutput("TThreadedServer uncaught exception.");
    }
    if (eventHandler != nullptr) {
      eventHandler->connectionDestroyed(&context_);
    }

    try {
      input_->getTransport()->close();
    } catch (TTransportException& ttx) {
      string errStr = string("TThreadedServer input close failed: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    }
    try {
      if (output_->getTransport()->isOpen()) {
        output_->getTransport()->close();
      }
    } catch (TTransportException& ttx) {
      string errStr = string("TThreadedServer output close failed: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    }

    // Remove this task from parent bookkeeping
    {
      Synchronized s(server_.tasksMonitor_);
      server_.tasks_.erase(this);
      if (server_.tasks_.empty()) {
        server_.tasksMonitor_.notify();
      }
    }

  }

  TConnectionContext* getConnectionContext() {
    return &context_;
  }

 private:
  TThreadedServer& server_;
  friend class TThreadedServer;

  shared_ptr<TProcessor> processor_;
  shared_ptr<TProtocol> input_;
  shared_ptr<TProtocol> output_;
  TRpcTransportContext context_;
};

void TThreadedServer::init() {
  stop_ = false;

  if (!threadFactory_) {
    threadFactory_.reset(new PosixThreadFactory);
  }
}

TThreadedServer::~TThreadedServer() {}

void TThreadedServer::serve() {

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

      TThreadedServer::Task* task = new TThreadedServer::Task(*this,
                                                              inputProtocol,
                                                              outputProtocol,
                                                              client);

      // Create a task
      shared_ptr<Runnable> runnable =
        shared_ptr<Runnable>(task);

      // Create a thread for this task
      shared_ptr<Thread> thread =
        shared_ptr<Thread>(threadFactory_->newThread(runnable));

      // Insert thread into the set of threads
      {
        Synchronized s(tasksMonitor_);
        tasks_.insert(task);
      }

      // Start the thread!
      thread->start();

    } catch (TTransportException& ttx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      if (!stop_ || ttx.getType() != TTransportException::INTERRUPTED) {
        string errStr = string("TThreadedServer: TServerTransport died on accept: ") + ttx.what();
        GlobalOutput(errStr.c_str());
      }
      continue;
    } catch (TException& tx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = string("TThreadedServer: Caught TException: ") + tx.what();
      GlobalOutput(errStr.c_str());
      continue;
    } catch (const string& s) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = "TThreadedServer: Unknown exception: " + s;
      GlobalOutput(errStr.c_str());
      break;
    }
  }

  // If stopped manually, make sure to close server transport
  if (stop_) {
    try {
      serverTransport_->close();
    } catch (TException &tx) {
      string errStr = string("TThreadedServer: Exception shutting down: ") + tx.what();
      GlobalOutput(errStr.c_str());
    }
    try {
      Synchronized s(tasksMonitor_);
      while (!tasks_.empty()) {
        tasksMonitor_.wait();
      }
    } catch (TException &tx) {
      string errStr = string("TThreadedServer: Exception joining workers: ") + tx.what();
      GlobalOutput(errStr.c_str());
    }
    stop_ = false;
  }

}

TConnectionContext* TThreadedServer::getConnectionContext() const {
  Task* task = currentTask_.get();
  if (task) {
    return task->getConnectionContext();
  }
  return nullptr;
}

void TThreadedServer::setCurrentTask(Task* task) {
  assert(currentTask_.get() == nullptr);
  currentTask_.set(task);
}

}}} // apache::thrift::server
