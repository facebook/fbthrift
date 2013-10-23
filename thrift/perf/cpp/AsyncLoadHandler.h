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
#ifndef THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER_H_
#define THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER_H_ 1

#include "common/fb303/cpp/AsyncFacebookBase.h"

#include "thrift/perf/if/gen-cpp/LoadTest.h"

namespace apache { namespace thrift {

namespace async {
class TEventServer;
}

namespace test {

class AsyncLoadHandler : public LoadTestCobSvIf
                       , public facebook::fb303::AsyncFacebookBase {
 public:

  void getStatus(
    std::function<void(facebook::fb303::fb_status const& _return)> cob) {
    cob(facebook::fb303::ALIVE);
  }

  typedef std::function<void()> VoidCob;
  typedef std::function<void(const std::exception&)> ErrorCob;
  typedef std::function<void(const std::string&)> StringCob;
  typedef std::function<void(const int64_t&)> I64Cob;

  explicit AsyncLoadHandler(async::TEventServer* server = NULL)
    : AsyncFacebookBase("AsyncLoadHandler")
    , server_(server)
    , burnIntervalUsec_(5000) {}

  void setServer(async::TEventServer* server) {
    server_ = server;
  }

  async::TEventServer* getServer() const {
    return server_;
  }

  /**
   * To play nicely with TEventBase, the burn*() methods yield to the event
   * loop every so often.  This method sets how long the burn methods will
   * burn at one time before yeilding.
   */
  void setBurnIntervalUsec(uint64_t interval) {
    burnIntervalUsec_ = interval;
  }

  uint64_t getBurnIntervalUsec() const {
    return burnIntervalUsec_;
  }

  virtual void noop(VoidCob cob);
  virtual void onewayNoop(VoidCob cob);
  virtual void asyncNoop(VoidCob cob);
  virtual void sleep(VoidCob cob, const int64_t microseconds);
  virtual void onewaySleep(VoidCob cob, const int64_t microseconds);
  virtual void burn(VoidCob cob, const int64_t microseconds);
  virtual void onewayBurn(VoidCob cob, const int64_t microseconds);
  virtual void badSleep(VoidCob cob, const int64_t microseconds);
  virtual void badBurn(VoidCob cob, const int64_t microseconds);
  virtual void throwError(VoidCob cob, ErrorCob exn_cob, const int32_t code);
  virtual void throwUnexpected(VoidCob cob, const int32_t code);
  virtual void onewayThrow(VoidCob cob, const int32_t code);
  virtual void send(VoidCob cob, const std::string& data);
  virtual void onewaySend(VoidCob cob, const std::string& data);
  virtual void recv(StringCob cob, const int64_t bytes);
  virtual void sendrecv(StringCob cob,
                        const std::string& data,
                        const int64_t recvBytes);
  virtual void echo(StringCob cob, const std::string& data);
  virtual void add(I64Cob cob, const int64_t a, const int64_t b);

 private:
  class Burner;

  async::TEventServer* server_;
  uint64_t burnIntervalUsec_;
};

}}} // apache::thrift::test

#endif // THRIFT_TEST_HANDLERS_ASYNCLOADHANDLER_H_
