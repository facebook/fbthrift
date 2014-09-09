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

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/protocol/TDebugProtocol.h>
#include <thrift/lib/cpp/protocol/TProtocolTap.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/transport/TFDTransport.h>

using std::shared_ptr;
using std::cout;
using std::endl;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

void usage() {
  fprintf(stderr,
      "usage: thrift_dump [-c] {-b|-f|-s} < input > output\n"
      "  -c use TCompactProtocol instead of TBinaryProtocol\n"
      "  -b TBufferedTransport messages\n"
      "  -f TFramedTransport messages\n"
      "  -s Raw structures\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    usage();
  }

  shared_ptr<TTransport> stdin_trans(new TFDTransport(STDIN_FILENO));
  shared_ptr<TTransport> itrans;

  bool buffered = false;
  bool rawStructures = false;
  bool framed = false;
  bool compact = false;
  for (int i = 1; i < argc; i++) {
    buffered |= argv[i] == std::string("-b");
    rawStructures |= argv[i] == std::string("-s");
    framed |= argv[i] == std::string("-f");
    compact |= argv[i] == std::string("-c");
  }

  if (buffered || rawStructures) {
    itrans = std::make_shared<TBufferedTransport>(stdin_trans);
  } else if (framed) {
    itrans = std::make_shared<TFramedTransport>(stdin_trans);
  } else {
    usage();
  }

  shared_ptr<TProtocol> iprot;
  if (compact) {
    iprot = std::make_shared<TCompactProtocol>(itrans);
  } else {
    iprot = std::make_shared<TBinaryProtocol>(itrans);
  }

  auto oprot = std::make_shared<TDebugProtocol>(
    std::make_shared<TBufferedTransport>(
      std::make_shared<TFDTransport>(STDOUT_FILENO)));

  TProtocolTap tap(iprot, oprot);

  try {
    if (rawStructures) {
      for (;;) {
        tap.skip(T_STRUCT);
      }
    } else {
      std::string name;
      TMessageType messageType;
      int32_t seqid;
      for (;;) {
        tap.readMessageBegin(name, messageType, seqid);
        tap.skip(T_STRUCT);
        tap.readMessageEnd();
      }
    }
  } catch (const TProtocolException &exn) {
    cout << "Protocol Exception: " << exn.what() << endl;
  } catch (...) {
    oprot->getTransport()->flush();
  }

  cout << endl;

  return 0;
}
