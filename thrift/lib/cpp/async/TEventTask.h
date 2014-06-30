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
#ifndef _THRIFT_TEVENTTASK_H_
#define _THRIFT_TEVENTTASK_H_ 1

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp/async/TEventConnection.h>
#include <functional>
#include <memory>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>

namespace apache { namespace thrift { namespace async {

class TEventTask : public apache::thrift::concurrency::Runnable {
 public:
  explicit TEventTask(TEventConnection* connection);

  void run();

  TEventConnection* getConnection() const {
    return connection_;
  }

 private:
  std::shared_ptr<apache::thrift::server::TProcessor> processor_;
  std::shared_ptr<apache::thrift::protocol::TProtocol> input_;
  std::shared_ptr<apache::thrift::protocol::TProtocol> output_;
  TEventConnection* connection_;
  TConnectionContext* connectionContext_;
};

class TaskCompletionMessage {
 public:
  explicit TaskCompletionMessage(TEventConnection *inConnection)
      : connection(inConnection) {}

  TaskCompletionMessage(TaskCompletionMessage &&msg)
      : connection(msg.connection) {
    msg.connection = nullptr;
  }

  TEventConnection *connection;
};

} } } // namespace apache::thrift::async

#endif // !_THRIFT_TEVENTTASK_H_
