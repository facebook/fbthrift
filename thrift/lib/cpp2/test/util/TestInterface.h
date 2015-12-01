/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>

namespace {
  const std::string kEchoSuffix(45, 'c');
}

class TestInterface : public apache::thrift::test::cpp2::TestServiceSvIf {
  void sendResponse(std::string& _return, int64_t size) override {
    if (size >= 0) {
      usleep(size);
    }

    EXPECT_NE("", getConnectionContext()->getPeerAddress()->describe());

    _return = folly::format("test{0}", size).str();
  }

  void noResponse(int64_t size) override { usleep(size); }

  void echoRequest(std::string& _return,
                   std::unique_ptr<std::string> req) override {
    _return = *req + kEchoSuffix;
  }

  typedef apache::thrift::HandlerCallback<std::unique_ptr<std::string>>
      StringCob;
  void async_tm_serializationTest(std::unique_ptr<StringCob> callback,
                                  bool inEventBase) override {
    std::unique_ptr<std::string> sp(new std::string("hello world"));
    callback->result(std::move(sp));
  }

  void async_eb_eventBaseAsync(std::unique_ptr<StringCob> callback) override {
    std::unique_ptr<std::string> hello(new std::string("hello world"));
    callback->result(std::move(hello));
  }

  void async_tm_notCalledBack(
      std::unique_ptr<apache::thrift::HandlerCallback<void>> cb) override {}

  void echoIOBuf(std::unique_ptr<folly::IOBuf>& ret,
      std::unique_ptr<folly::IOBuf> buf) override {
    ret = std::move(buf);
    folly::io::Appender cursor(ret.get(), kEchoSuffix.size());
    cursor.push(folly::StringPiece(kEchoSuffix.data(), kEchoSuffix.size()));
  }
};
