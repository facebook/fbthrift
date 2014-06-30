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
#ifndef THRIFT_LIB_CPP2_TEST_UTILS_H_
#define THRIFT_LIB_CPP2_TEST_UTILS_H_

#include <memory>
#include <thread>

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>

struct Server {
  template <typename Function>
  static std::shared_ptr<apache::thrift::ThriftServer> get(
      Function&& constructServer) {
    static apache::thrift::util::ScopedServerThread st;
    if (!st.getServer().lock()) {
      st.start(constructServer());
      // ThriftServer unfortuntely has a bug where trying to join its
      // threads quickly after starting it up can cause it to block, so
      // sleep here for a short amount of time.
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return std::dynamic_pointer_cast<apache::thrift::ThriftServer>(
      st.getServer().lock());
  }
};

#endif
