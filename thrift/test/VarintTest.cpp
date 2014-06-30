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

#include <thrift/lib/cpp/util/VarintUtils.h>
#include <boost/test/unit_test.hpp>

using namespace apache::thrift::util;
using namespace std;
using folly::IOBuf;
using namespace folly::io;

BOOST_AUTO_TEST_SUITE( VarintTest )

BOOST_AUTO_TEST_CASE(Varint) {
  unique_ptr<IOBuf> iobuf1(IOBuf::create(1000));
  iobuf1->append(1000);
  RWPrivateCursor wcursor(iobuf1.get());
  Cursor rcursor(iobuf1.get());
  int64_t v = 1;
  writeVarint(wcursor, 0);
  for (int bit = 0; bit < 64; bit++, v <<= 1) {
    if (bit < 8)  writeVarint(wcursor, int8_t(v));
    if (bit < 16) writeVarint(wcursor, int16_t(v));
    if (bit < 32) writeVarint(wcursor, int32_t(v));
    writeVarint(wcursor, v);
  }
  int32_t oversize = 1000000;
  writeVarint(wcursor, oversize);

  BOOST_CHECK_EQUAL(0, readVarint<int8_t>(rcursor));
  v = 1;
  for (int bit = 0; bit < 64; bit++, v <<= 1) {
    if (bit < 8)  BOOST_CHECK_EQUAL(int8_t(v),  readVarint<int8_t>(rcursor));
    if (bit < 16) BOOST_CHECK_EQUAL(int16_t(v), readVarint<int16_t>(rcursor));
    if (bit < 32) BOOST_CHECK_EQUAL(int32_t(v), readVarint<int32_t>(rcursor));
    BOOST_CHECK_EQUAL(v, readVarint<int64_t>(rcursor));
  }
  try {
    // should throw due to large varint
    uint8_t v = readVarint<uint8_t>(rcursor);
    printf("%u\n", v);
    BOOST_CHECK_EQUAL(0, 1);
  } catch (const out_of_range& e) {
  }

}

BOOST_AUTO_TEST_SUITE_END()

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "VarintTest";

  return nullptr;
}
