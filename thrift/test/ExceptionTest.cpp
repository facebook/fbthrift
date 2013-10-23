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
#define BOOST_TEST_MODULE ExceptionTest
#include <boost/test/unit_test.hpp>
#include "thrift/test/gen-cpp/ExceptionTest_types.h"

BOOST_AUTO_TEST_SUITE( ExceptionTest )

BOOST_AUTO_TEST_CASE( test_default_constructor ) {
  try {
    MyException e;
    e.msg = "what!!!";
    throw e;
  } catch (const std::exception& ex) {
    BOOST_CHECK_EQUAL(ex.what(), "what!!!");
  }
}

BOOST_AUTO_TEST_CASE( test_constructor_with_param ) {
  try {
    throw MyException("what!!!");
  } catch (const std::exception& ex) {
    BOOST_CHECK_EQUAL(ex.what(), "what!!!");
  }
}

BOOST_AUTO_TEST_SUITE_END()
