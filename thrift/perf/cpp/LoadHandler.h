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
#ifndef THRIFT_TEST_HANDLERS_LOADHANDLER_H_
#define THRIFT_TEST_HANDLERS_LOADHANDLER_H_ 1

#include "common/fb303/cpp/FacebookBase.h"

#include <thrift/perf/if/gen-cpp/LoadTest.h>

namespace apache { namespace thrift { namespace test {

class LoadHandler : public LoadTestIf
                  , public facebook::fb303::FacebookBase {
 public:
  LoadHandler() : FacebookBase("LoadHandler") {}
  void noop() override;
  void onewayNoop() override;
  void asyncNoop() override;
  void sleep(const int64_t microseconds) override;
  void onewaySleep(const int64_t microseconds) override;
  void burn(const int64_t microseconds) override;
  void onewayBurn(const int64_t microseconds) override;
  void badSleep(const int64_t microseconds) override;
  void badBurn(const int64_t microseconds) override;
  void throwError(const int32_t code) override;
  void throwUnexpected(const int32_t code) override;
  void onewayThrow(const int32_t code) override;
  void send(const std::string& data) override;
  void onewaySend(const std::string& data) override;
  void recv(std::string& _return, const int64_t bytes) override;
  void sendrecv(std::string& _return,
                const std::string& data,
                const int64_t recvBytes) override;
  void echo(std::string& _return, const std::string& data) override;
  int64_t add(int64_t a, int64_t b) override;

  facebook::fb303::fb_status getStatus() override {
    return facebook::fb303::ALIVE;
  }

 private:
  void burnImpl(int64_t microseconds);
  void throwImpl(int32_t code);
};

}}} // apache::thrift::test

#endif // THRIFT_TEST_HANDLERS_LOADHANDLER_H_
