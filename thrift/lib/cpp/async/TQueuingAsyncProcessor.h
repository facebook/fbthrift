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
#ifndef _THRIFT_TQUEUINGASYNCPROCESSOR_H_
#define _THRIFT_TQUEUINGASYNCPROCESSOR_H_ 1

#include <functional>
#include <memory>
#include <thrift/lib/cpp/TProcessor.h>
#include <thrift/lib/cpp/async/TAsyncProcessor.h>
#include <thrift/lib/cpp/async/TEventTask.h>
#include <thrift/lib/cpp/concurrency/Exception.h>

namespace apache { namespace thrift { namespace async {

/**
 * Adapter to allow a TProcessor to be used as a TAsyncProcessor.
 *
 * Note: this is not intended for use outside of TEventConnection since the
 * callback mechanism used in TEventTask will invoke handleAsyncTaskComplete()
 * regardless of what is passed in as the cob.
 *
 * Uses a per-server task queue for all calls.
 */
class TQueuingAsyncProcessor : public TAsyncProcessor {
 public:
  TQueuingAsyncProcessor(
   std::shared_ptr<apache::thrift::server::TProcessor> processor,
   std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager,
   int64_t taskExpireTime,
   TEventConnection* connection)
    : processor_(processor)
    , threadManager_(threadManager)
    , taskExpireTime_(taskExpireTime)
    , connection_(connection)
  {}

  virtual void process(
                std::function<void(bool success)> cob,
                std::shared_ptr<apache::thrift::protocol::TProtocol> in,
                std::shared_ptr<apache::thrift::protocol::TProtocol> out,
                TConnectionContext* context) {

    std::shared_ptr<apache::thrift::concurrency::Runnable> task =
      std::shared_ptr<apache::thrift::concurrency::Runnable>(
                                                 new TEventTask(connection_));

    try {
      threadManager_->add(task, 0LL, taskExpireTime_);
    } catch (apache::thrift::concurrency::IllegalStateException & ise) {
      T_ERROR("IllegalStateException: TQueuingAsyncProcessor::process() %s",
              ise.what());
      // no task will be making a callback
      return cob(false);
    }
  }

 private:
  std::shared_ptr<apache::thrift::server::TProcessor> processor_;

  /// For processing via thread pool
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;

  /// Time in milliseconds before an unperformed task expires (0 == infinite).
  int64_t taskExpireTime_;

  /// The worker that started us
  TEventConnection* connection_;
};

}}} // apache::thrift::async

#endif // #ifndef _THRIFT_TQUEUINGASYNCPROCESSOR_H_
