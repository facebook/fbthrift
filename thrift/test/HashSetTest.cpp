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

#define BOOST_TEST_MODULE HashSetTest
#include <boost/test/unit_test.hpp>
#include "thrift/test/gen-cpp/HashSetTest_types.h"

BOOST_AUTO_TEST_SUITE( HashSetTest )

BOOST_AUTO_TEST_CASE( test_hashset ) {
  foo f;
  f.bar.insert(5);
  BOOST_CHECK_EQUAL(f.bar.count(5), 1);
  f.bar.insert(6);
  BOOST_CHECK_EQUAL(f.bar.count(6), 1);

  f.bar.erase(5);
  BOOST_CHECK_EQUAL(f.bar.count(5), 0);

  f.baz.insert("cool");
  BOOST_CHECK_EQUAL(f.baz.count("cool"), 1);

  f.baz.erase("cool");
  BOOST_CHECK_EQUAL(f.baz.count("cool"), 0);
}

BOOST_AUTO_TEST_SUITE_END()
