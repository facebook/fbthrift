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
#ifndef THRIFT_TEST_PERF_CLIENTLOADCONFIG_H_
#define THRIFT_TEST_PERF_CLIENTLOADCONFIG_H_ 1

#include "thrift/lib/cpp/test/loadgen/WeightedLoadConfig.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"

namespace apache { namespace thrift {

namespace test {

class ClientLoadConfig : public loadgen::WeightedLoadConfig {
 public:
   enum OperationEnum {
     OP_NOOP = 0,
     OP_ONEWAY_NOOP,
     OP_ASYNC_NOOP,
     OP_SLEEP,
     OP_ONEWAY_SLEEP,
     OP_BURN,
     OP_ONEWAY_BURN,
     OP_BAD_SLEEP,
     OP_BAD_BURN,
     OP_THROW_ERROR,
     OP_THROW_UNEXPECTED,
     OP_ONEWAY_THROW,
     OP_SEND,
     OP_ONEWAY_SEND,
     OP_RECV,
     OP_SENDRECV,
     OP_ECHO,
     OP_ADD,
     NUM_OPS
   };

  ClientLoadConfig();

  virtual uint32_t pickOpsPerConnection();
  virtual uint32_t getNumWorkerThreads() const;
  virtual uint64_t getDesiredQPS() const;

  virtual uint32_t getAsyncClients() const;
  virtual uint32_t getAsyncOpsPerClient() const;
  /**
   * Pick a number of microseconds to use for a sleep operation.
   */
  uint32_t pickSleepUsec();

  /**
   * Pick a number of microseconds to use for a burn operation.
   */
  uint32_t pickBurnUsec();

  /**
   * Pick a number of bytes for a send request
   */
  uint32_t pickSendSize();

  /**
   * Pick a number of bytes for a receive request
   */
  uint32_t pickRecvSize();

  const transport::TSocketAddress* getAddress() const {
    return &address_;
  }

  std::string getAddressHostname() const {
    return addressHostname_;
  }

  bool useFramedTransport() const;

  bool useHeaderProtocol() const;

  bool useAsync() const;

  bool useCpp2() const;

  bool useSSL() const;

  bool useSR() const;

  std::string srTier() const;

  std::string SASLPolicy() const;
  std::string SASLServiceTier() const;

 private:
  uint32_t pickLogNormal(double mean, double sigma);

  transport::TSocketAddress address_;
  std::string addressHostname_;
};

}}} // apache::thrift::test

#endif // THRIFT_TEST_PERF_CLIENTLOADCONFIG_H_
