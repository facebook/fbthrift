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

#include <thrift/lib/cpp/transport/TMemPagedTransport.tcc>

#include <folly/portability/Unistd.h>

using std::cout;
using std::cerr;
using std::endl;

namespace apache { namespace thrift { namespace transport {

static void testFixedMemoryPagedTransport() {
  srand(time(0));
  const size_t kPageSize = 1<<10;
  const size_t kMaxMemSize = 1<<28;
  const size_t kCacheSize = 1<<18;
  const size_t kBufferSize = 1<<16;

  std::shared_ptr<FixedSizeMemoryPageFactory>
    fsFactory(new FixedSizeMemoryPageFactory(kPageSize,
                                             kCacheSize,
                                             kMaxMemSize));
  TMemPagedTransport fsTransport(fsFactory);

  size_t writeBytes = 0;
  fsTransport.resetForWrite();
  uint8_t* buffer = (uint8_t*)malloc(kBufferSize);
  const size_t maxRounds = 1000;
  size_t rounds = maxRounds;
  while (rounds--) {
    size_t length = rand() % kBufferSize;
    cout << "About to write " << length << " bytes" << endl;
    try {
      fsTransport.write(buffer, length);
      cout << "Written " << length << " bytes" << endl;
    } catch (const TTransportException& x) {
      cerr << "Can not write bytes" << endl;
      //return -1;
      break;
    }
    writeBytes += length;
  }

  // get total written bytes
  size_t writtenBytes = fsTransport.writeEnd();

  // read
  size_t readBytes = 0;
  rounds = maxRounds;
  while (true) {
    size_t length = rand() % kBufferSize;
    cout << "Abount to read " << length << " bytes" << endl;
    size_t processed = fsTransport.read(buffer, length);
    cout << "Read " << processed << " bytes" << endl;
    readBytes += processed;
    if (processed != length) {
      break; // read all
    }
  }

  size_t readDoneBytes = fsTransport.readEnd();
  cout << "Total has written " << writtenBytes << " bytes, was written "
       << writeBytes << " bytes" << endl;
  cout << "Total has read " << readDoneBytes << " bytes, was read "
       << readBytes << " bytes" << endl;
  free(buffer);
}


}}} // apache::thrift::transport

int main(int /*argc*/, char** /*argv*/) {
  apache::thrift::transport::testFixedMemoryPagedTransport();
  return 0;
}
