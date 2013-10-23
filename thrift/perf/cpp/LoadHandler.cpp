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
#include "thrift/perf/cpp/LoadHandler.h"

#include "thrift/lib/cpp/concurrency/Util.h"

#include <unistd.h>

using apache::thrift::concurrency::Util;

namespace apache { namespace thrift { namespace test {

void LoadHandler::noop() {
}

void LoadHandler::onewayNoop() {
}

void LoadHandler::asyncNoop() {
}

void LoadHandler::sleep(const int64_t microseconds) {
  usleep(microseconds);
}

void LoadHandler::onewaySleep(const int64_t microseconds) {
  usleep(microseconds);
}

void LoadHandler::burn(const int64_t microseconds) {
  burnImpl(microseconds);
}

void LoadHandler::onewayBurn(const int64_t microseconds) {
  burnImpl(microseconds);
}

void LoadHandler::badSleep(const int64_t microseconds) {
  burnImpl(microseconds);
}

void LoadHandler::badBurn(const int64_t microseconds) {
  burnImpl(microseconds);
}

void LoadHandler::throwError(const int32_t code) {
  throwImpl(code);
}

void LoadHandler::throwUnexpected(const int32_t code) {
  throwImpl(code);
}

void LoadHandler::onewayThrow(const int32_t code) {
  throwImpl(code);
}

void LoadHandler::burnImpl(int64_t microseconds) {
  int64_t end = Util::currentTimeUsec() + microseconds;
  while (Util::currentTimeUsec() < end) {
  }
}

void LoadHandler::throwImpl(int32_t code) {
  LoadError error;
  error.code = code;
  throw error;
}

void LoadHandler::send(const std::string& data) {
}

void LoadHandler::onewaySend(const std::string& data) {
}

void LoadHandler::recv(std::string& _return, const int64_t bytes) {
  _return.resize(bytes, 'a');
}

void LoadHandler::sendrecv(std::string& _return,
                           const std::string& data,
                           const int64_t recvBytes) {
  _return.resize(recvBytes, 'a');
}

void LoadHandler::echo(std::string& _return, const std::string& data) {
  _return = data;
}

int64_t LoadHandler::add(int64_t a, int64_t b) {
  return a + b;
}

}}} // apache::thrift::test
