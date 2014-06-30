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

#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <thrift/lib/cpp/server/example/TSimpleServer.h>

#include <thrift/lib/cpp/server/TRpcTransportContext.h>
#include <thrift/lib/cpp/transport/TSocketAddress.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <string>
#include <iostream>

namespace apache { namespace thrift { namespace server {

using namespace std;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using std::shared_ptr;

/**
 * A simple single-threaded application server. Perfect for unit tests!
 *
 */
void TSimpleServer::serve() {

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

  // Fetch client from server
  while (!stop_) {
    try {
      client = serverTransport_->accept();
      TTransportPair transports = duplexTransportFactory_->getTransport(client);
      inputTransport = transports.first;
      outputTransport = transports.second;
      TProtocolPair protocols = duplexProtocolFactory_->getProtocol(transports);
      inputProtocol = protocols.first;
      outputProtocol = protocols.second;
    } catch (TTransportException& ttx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = string("TServerTransport died on accept: ") + ttx.what();
      GlobalOutput(errStr.c_str());
      continue;
    } catch (TException& tx) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = string("Some kind of accept exception: ") + tx.what();
      GlobalOutput(errStr.c_str());
      continue;
    } catch (const string& s) {
      if (inputTransport != nullptr) { inputTransport->close(); }
      if (outputTransport != nullptr) { outputTransport->close(); }
      if (client != nullptr) { client->close(); }
      string errStr = string("Some kind of accept exception: ") + s;
      GlobalOutput(errStr.c_str());
      break;
    }

    TRpcTransportContext connCtx(client, inputProtocol, outputProtocol);
    connectionCtx_ = &connCtx;

    // Get the processor
    shared_ptr<TProcessor> processor = getProcessor(&connCtx);
    if (eventHandler_ != nullptr) {
      eventHandler_->newConnection(&connCtx);
    }
    try {
      for (;;) {
        if (!processor->process(inputProtocol, outputProtocol, &connCtx) ||
            // stop waiting for client if server was stopped.
            stop_ ||
            // Peek ahead, is the remote side closed?
            !inputProtocol->getTransport()->peek()) {
          break;
        }
      }
    } catch (const TTransportException& ttx) {
      string errStr = string("TSimpleServer client died: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    } catch (const std::exception& x) {
      GlobalOutput.printf("TSimpleServer exception: %s: %s",
                          typeid(x).name(), x.what());
    } catch (...) {
      GlobalOutput("TSimpleServer uncaught exception.");
    }
    if (eventHandler_ != nullptr) {
      eventHandler_->connectionDestroyed(&connCtx);
    }
    connectionCtx_ = nullptr;

    try {
      if (inputTransport) {
        inputTransport->close();
      }
    } catch (const TTransportException& ttx) {
      string errStr = string("TSimpleServer input close failed: ")
        + ttx.what();
      GlobalOutput(errStr.c_str());
    }
    try {
      if (outputTransport) {
        outputTransport->close();
      }
    } catch (const TTransportException& ttx) {
      string errStr = string("TSimpleServer output close failed: ")
        + ttx.what();
      GlobalOutput(errStr.c_str());
    }
    try {
      client->close();
    } catch (const TTransportException& ttx) {
      string errStr = string("TSimpleServer client close failed: ")
        + ttx.what();
      GlobalOutput(errStr.c_str());
    }
  }

  if (stop_) {
    try {
      serverTransport_->close();
    } catch (TTransportException &ttx) {
      string errStr = string("TServerTransport failed on close: ") + ttx.what();
      GlobalOutput(errStr.c_str());
    }
    stop_ = false;
  }
}

/**
 * Notify the serving thread to stop.
 */
void TSimpleServer::stop() {
  // Set stop_ so that the server will exit the next time around the loop
  stop_ = true;

  // The serving thread may be waiting inside accept().  Interrupt the server
  // transport to wake it up.
  //
  // Note: If the server thread is currently processing a connection, we still
  // have to wait until it finishes that connection.
  serverTransport_->interrupt();
}

TConnectionContext* TSimpleServer::getConnectionContext() const {
  return connectionCtx_;
}

}}} // apache::thrift::server
