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
#ifndef THRIFT_LIB_CPP_UTIL_FROZENTESTUTIL_H_
#define THRIFT_LIB_CPP_UTIL_FROZENTESTUTIL_H_

#include <thrift/lib/cpp/util/FrozenUtil.h>
#include <folly/experimental/TestUtil.h>

namespace apache { namespace thrift { namespace util {

template<class T>
folly::test::TemporaryFile freezeToTempFile(const T& value) {
  folly::test::TemporaryFile tmp;
  freezeToFile(value, tmp.fd());
  return tmp;
}

}}} // !apache::thrift::test

#endif // include guard
