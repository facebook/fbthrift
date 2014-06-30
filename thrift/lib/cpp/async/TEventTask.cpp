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
#include <thrift/lib/cpp/async/TEventTask.h>

#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/async/TEventWorker.h>

namespace apache { namespace thrift { namespace async {

using apache::thrift::transport::TTransportException;

TEventTask::TEventTask(TEventConnection* connection)
  : processor_(connection->getProcessor())
  , input_(connection->getInputProtocol())
  , output_(connection->getOutputProtocol())
  , connection_(connection)
  , connectionContext_(connection->getConnectionContext()) {}

void TEventTask::run() {
  TEventServer* server = connection_->getWorker()->getServer();

  server->setCurrentConnection(connection_);
  try {
    processor_->process(input_, output_, connectionContext_);
    server->clearCurrentConnection();
  } catch (std::bad_alloc&) {
    T_ERROR("TEventServer task queue caught bad_alloc. Exiting.");
    exit(-1);
  } catch (std::exception const& x) {
    T_ERROR("TEventTask: a %s exception occured during call processing: %s",
            typeid(x).name(), x.what());
    // This will set a flag in TEventConnection that will cause it to
    // shutdown when it is notified of task completion.
    server->clearCurrentConnection();
    connection_->cleanup();
  } catch (...) {
    T_ERROR("TEventServer task queue uncaught exception.");
    // see comment for previous catch().
    server->clearCurrentConnection();
    connection_->cleanup();
  }

  // Signal completion back to the libevent thread via notification queue
  TaskCompletionMessage msg(connection_);
  if (!connection_->notifyCompletion(std::move(msg))) {
    throw TLibraryException("TEventTask::run: failed to notify worker thread");
  }
}

} } } // namespace apache::thrift::async
