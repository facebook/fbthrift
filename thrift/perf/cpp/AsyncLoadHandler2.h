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
#ifndef THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER2_H_
#define THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER2_H_ 1

#include "common/fb303/cpp/FacebookBase2.h"

#include "thrift/perf/if/gen-cpp2/LoadTest.h"

#include <thrift/lib/cpp/async/TEventServer.h>

using apache::thrift::async::TEventServer;

namespace apache { namespace thrift {

// Using classes as callbacks is _much_ faster than std::function
class FastNoopCallback
    : public apache::thrift::async::TEventBase::LoopCallback {
 public:
  FastNoopCallback(std::unique_ptr<HandlerCallback<void>> callback)
      : callback_(std::move(callback)) {}
  virtual void runLoopCallback() noexcept {
    callback_->done();
    delete this;
  }
  std::unique_ptr<HandlerCallback<void>> callback_;
};

class AsyncLoadHandler2 : public LoadTestSvIf
                        , public facebook::fb303::FacebookBase2 {

 public:

  facebook::fb303::cpp2::fb_status getStatus() {
    return facebook::fb303::cpp2::fb_status::ALIVE;
  }

  explicit AsyncLoadHandler2(TEventServer* server = nullptr)
    : FacebookBase2("AsyncLoadHandler2") {}

  void async_eb_noop(std::unique_ptr<HandlerCallback<void>> callback);
  void async_eb_onewayNoop(std::unique_ptr<HandlerCallbackBase> callback);
  void async_eb_asyncNoop(std::unique_ptr<HandlerCallback<void>> callback);
  void async_eb_sleep(std::unique_ptr<HandlerCallback<void>> callback,
                   int64_t microseconds);
  void async_eb_onewaySleep(std::unique_ptr<HandlerCallbackBase> callback,
                   int64_t microseconds);
  void sync_burn(int64_t microseconds);
  folly::Future<void> future_burn(int64_t microseconds);
  void sync_onewayBurn(int64_t microseconds);
  folly::Future<void> future_onewayBurn(int64_t microseconds);
  void async_eb_badSleep(std::unique_ptr<HandlerCallback<void>> callback,
                      int64_t microseconds);
  void async_eb_badBurn(std::unique_ptr<HandlerCallback<void>> callback,
                     int64_t microseconds);
  void async_eb_throwError(std::unique_ptr<HandlerCallback<void>> callback,
                        int32_t code);
  void async_eb_throwUnexpected(std::unique_ptr<HandlerCallback<void>> callback,
                             int32_t code);
  void async_eb_onewayThrow(std::unique_ptr<HandlerCallbackBase> callback,
                         int32_t code);
  void async_eb_send(std::unique_ptr<HandlerCallback<void>> callback,
                  std::unique_ptr<std::string> data);
  void async_eb_onewaySend(std::unique_ptr<HandlerCallbackBase> callback,
                        std::unique_ptr<std::string> data);
  void async_eb_recv(
    std::unique_ptr<HandlerCallback<std::unique_ptr<std::string>>> callback,
    int64_t bytes);
  void async_eb_sendrecv(
    std::unique_ptr<HandlerCallback<std::unique_ptr<std::string>>> callback,
    std::unique_ptr<std::string> data,
    int64_t recvBytes);
  void sync_echo(
    std::string& output,
    std::unique_ptr<std::string> data);
  folly::Future<std::unique_ptr<std::string>> future_echo(
    std::unique_ptr<std::string> data);
  void async_eb_add(std::unique_ptr<HandlerCallback<int64_t>>,
                 int64_t a, int64_t b);
};

}} // apache::thrift

#endif // THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER2_H_
