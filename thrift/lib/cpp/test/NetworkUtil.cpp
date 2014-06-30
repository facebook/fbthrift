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
#include <thrift/lib/cpp/test/NetworkUtil.h>

#include <thrift/lib/cpp/transport/TSocketAddress.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

#include <sys/types.h>
#include <errno.h>
#include <ifaddrs.h>

using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::TTransportException;

namespace apache { namespace thrift { namespace test {

void getLocalAddresses(std::vector<TSocketAddress>* results) {
  struct ifaddrs* addrs = nullptr;
  if (getifaddrs(&addrs) != 0) {
    throw TLibraryException("getifaddrs() failed", errno);
  }

  try {
    for (struct ifaddrs* cur = addrs; cur != nullptr; cur = cur->ifa_next) {
      TSocketAddress addr;
      try {
        addr.setFromSockaddr(cur->ifa_addr);
      } catch (const TTransportException& ex) {
        // Most likely an unsupported address family.  Just skip this address.
        continue;
      }

      results->push_back(addr);
    }
  } catch (...) {
    freeifaddrs(addrs);
    throw;
  }

  freeifaddrs(addrs);
}

}}} // apache::thrift::test
