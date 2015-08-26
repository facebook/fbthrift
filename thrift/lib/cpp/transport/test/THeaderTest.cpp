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

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp/util/THttpParser.h>

#include <boost/test/unit_test.hpp>
#include <memory>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

using namespace boost;
using namespace apache::thrift;
using namespace folly;
using namespace apache::thrift::transport;

BOOST_AUTO_TEST_CASE(largetransform) {
  THeader header;
  header.setTransform(THeader::ZLIB_TRANSFORM); // ZLib flag

  size_t buf_size = 1000000;
  std::unique_ptr<IOBuf> buf(IOBuf::create(buf_size));
  buf->append(buf_size);

  std::map<std::string, std::string> persistentHeaders;
  buf = header.addHeader(std::move(buf), persistentHeaders);
  buf_size = buf->computeChainDataLength();
  std::unique_ptr<IOBufQueue> queue(new IOBufQueue);
  std::unique_ptr<IOBufQueue> queue2(new IOBufQueue);
  queue->append(std::move(buf));
  queue2->append(queue->split(buf_size/4));
  queue2->append(queue->split(buf_size/4));
  queue2->append(IOBuf::create(0)); // Empty buffer should work
  queue2->append(queue->split(buf_size/4));
  queue2->append(queue->move());

  size_t needed;

  buf = header.removeHeader(queue2.get(), needed, persistentHeaders);
}

BOOST_AUTO_TEST_CASE(http_clear_header) {
  THeader header;
  header.setClientType(THRIFT_HTTP_CLIENT_TYPE);
  auto parser = std::make_shared<apache::thrift::util::THttpClientParser>(
      "testhost", "testuri");
  header.setHttpClientParser(parser);
  header.setHeader("WriteHeader", "foo");

  size_t buf_size = 1000000;
  std::unique_ptr<IOBuf> buf(IOBuf::create(buf_size));
  std::map<std::string, std::string> persistentHeaders;
  buf = header.addHeader(std::move(buf), persistentHeaders);

  BOOST_CHECK(header.getWriteHeaders().empty());
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]);

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "THeaderTest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
