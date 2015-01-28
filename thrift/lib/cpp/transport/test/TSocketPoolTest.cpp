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

#include <thrift/lib/cpp/transport/TSocketPool.h>
#include <thrift/lib/cpp/transport/TServerSocket.h>

#include <gtest/gtest.h>

using namespace std;
using apache::thrift::transport::TSocketPool;
using apache::thrift::transport::TServerSocket;

/**
 * Test whether TSocketPool returns the port and host for the service
 * that it connects to.
 */
TEST(Main, TestGetPortAndHost) {

    int listeningPort = 3002;
    TServerSocket serverSocket(listeningPort);
    serverSocket.listen();

    vector<string> hosts;
    vector<int> ports;
    string loopback = "127.0.0.1";
    for (int i = 0; i < 5; i++) {
      hosts.push_back(loopback);
      ports.push_back(3000 + i);
    }
    TSocketPool socketPool(hosts, ports);
    socketPool.open();

    EXPECT_EQ(socketPool.getCurrentServerPort(), listeningPort);
    EXPECT_EQ(socketPool.getCurrentServerHost(), loopback);

};
