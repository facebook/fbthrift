/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp/util/THttpParser.h>

#include <gtest/gtest.h>

namespace {

class THttpClientParserTest : public testing::Test{};

using HeaderMap = std::map<std::string, std::string>;

void write(
    apache::thrift::util::THttpParser& parser,
    folly::StringPiece text) {
  while (!text.empty()) {
    void* buf = nullptr;
    size_t len = 0;
    parser.getReadBuffer(&buf, &len);
    len = std::min(len, text.size());
    std::memcpy(buf, text.data(), len);
    text.advance(len);
    parser.readDataAvailable(len);
  }
}
}

TEST_F(THttpClientParserTest, read_encapsulated_status_line) {
  apache::thrift::util::THttpClientParser parser;
  write(parser, "HTTP/1.1 200 OK\r\n");
  SUCCEED() << "did not crash";
}
