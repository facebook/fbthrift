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

#include <iostream>
#include <cmath>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/test/gen-cpp/DebugProtoTest_types.h>
#include <time.h>
#include <sys/time.h>

class Timer {
public:
  timeval vStart;

  Timer() {
    gettimeofday(&vStart, 0);
  }
  void start() {
    gettimeofday(&vStart, 0);
  }

  double frame() {
    timeval vEnd;
    gettimeofday(&vEnd, 0);
    double dstart = vStart.tv_sec + ((double)vStart.tv_usec / 1000000.0);
    double dend = vEnd.tv_sec + ((double)vEnd.tv_usec / 1000000.0);
    return dend - dstart;
  }

};

using std::cout;
using std::endl;
using std::string;
using namespace thrift::test::debug;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace boost;

int num = 10000;

template <typename TBufferType_>
void testWrite(OneOfEach ooe, std::shared_ptr<TMemoryBuffer>& buf,
               string type_name) {
  Timer timer;

  for (int i = 0; i < num; i ++) {
    buf->resetBuffer();
    TBufferType_ prot(buf);
    ooe.write(&prot);
    prot.getTransport()->flush();
  }
  cout << type_name << " Write: " << num / (1000 * timer.frame())
       << " kHz" << endl;
}

template <typename TBufferType_>
void testRead(std::shared_ptr<TMemoryBuffer>& buf, string type_name) {
  uint8_t* data;
  uint32_t datasize;

  buf->getBuffer(&data, &datasize);

  Timer timer;

  for (int i = 0; i < num; i ++) {
    OneOfEach ooe2;
    std::shared_ptr<TMemoryBuffer> buf2(new TMemoryBuffer(data, datasize));
    TBufferType_ prot(buf2);
    ooe2.read(&prot);
  }
  cout << type_name << " Read: " << num / (1000 * timer.frame())
       << " kHz" << endl;
}

template <typename TBufferType_>
void runTest(OneOfEach ooe, std::shared_ptr<TMemoryBuffer>& buf, string type_name) {
  testWrite<TBufferType_>(ooe, buf, type_name);
  testRead<TBufferType_>(buf, type_name);
}

int main() {

  OneOfEach ooe;
  ooe.im_true   = true;
  ooe.im_false  = false;
  ooe.a_bite    = 0xd6;
  ooe.integer16 = 27000;
  ooe.integer32 = 1 << 24;
  ooe.integer64 = (uint64_t)6000 * 1000 * 1000;
  ooe.double_precision = M_PI;
  ooe.some_characters  = "JSON THIS! \"\1";
  ooe.zomg_unicode     = "\xd7\n\a\t";
  ooe.base64 = "\1\2\3\255";
  ooe.string_string_map["one"] = "two";
  ooe.string_string_hash_map["three"] = "four";
  ooe.float_precision = (float)12.345;
  ooe.rank_map[567419810] = (float)0.211184;
  ooe.rank_map[507959914] = (float)0.080382;

  std::shared_ptr<TMemoryBuffer> buf(new TMemoryBuffer());

  uint8_t* data;
  uint32_t datasize;

  runTest<TBinaryProtocolT<TBufferBase> >(ooe, buf,
                                          "TBinaryProtocolT<TBufferBase>");
  runTest<TBinaryProtocol >(ooe, buf, "TBinaryProtocol");
  runTest<THeaderProtocol>(ooe, buf, "THeaderProtocol");

  return 0;
}
