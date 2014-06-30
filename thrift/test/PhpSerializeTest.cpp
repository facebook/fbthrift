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

#include <stdlib.h>
#include <iostream>
#include "thrift/test/gen-cpp/PhpSerializeTest_types.h"
#include <boost/lexical_cast.hpp>
#include <memory>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/protocol/TPhpSerializeProtocol.h>

/**
 * This is a random test of the php serialization protocol
 * in thrift/lib/cpp/protocol/TPhpSerializeProtocol.*
 *
 * The resulting binary is intended to be run from the root
 * of the fbcode repo. All it does is generate a whole bunch
 * of random nested structures and call on php to deserialize
 * and verify the resulting data
 *
 * The only special case it is intended to test is
 * serialization of a map with a numeric string as key
 * array( "12" => 3 )
 * which is handled differently in phpland.
 */

using std::string;
using std::cout;
using std::endl;
using boost::lexical_cast;
using std::shared_ptr;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::protocol::TPhpSerializeProtocol;

const int kCollnSize = 6;
const string kStrConst = "hahababoon";

string generateMapKey() {
  static int keyType = 0;
  const int kSize = 20;
  string key;

  switch (keyType++ % 3) {
  case 0:
    {
      key.resize(kSize + 1);
      for (int i = 0 ; i < kSize; i++) {
        key[i] = rand() % 128;
      }
      key[kSize] = '\0';
      break;
    }
  case 1:
    {
      key = lexical_cast<string>(abs(rand()));
      break;
    }
    case 2:
    {
      key = string(kStrConst) + lexical_cast<string>(keyType);
      break;
    }
  };

  return key;
}

void createSimpleData(SimpleData* sData, int rnd) {
  sData->a = rnd;
  sData->b = rnd * 100;
  sData->c = lexical_cast<string>(rnd + 2);
}

void createSimpleData(SimpleData* sData) {
  createSimpleData(sData, rand());
}

void createSecondLevel(SecondLevel* slData) {
  for (int i = 0; i < kCollnSize; i++) {
    int rnd = rand();
    string key = generateMapKey();
    SimpleData val;
    createSimpleData(&val, rnd);

    slData->a[key] = val;
    slData->b[rnd] = key;
    slData->c.push_back(val);
    slData->d.push_back(key);
    slData->e.push_back(rnd);
  }
}

void createFirstLevel(FirstLevel* flData) {
  for (int i = 0; i < kCollnSize; i++) {
    SecondLevel slData;
    createSecondLevel(&slData);
    string key = lexical_cast<string>(i);
    flData->a[key] = slData;
    flData->b.push_back(key);
  }
  createSimpleData(&flData->c);
}

int main(int argc, char** argv) {
  srand(time(nullptr));
  bool success = true;
  for (int i = 0; i < 10; i++) {
    FirstLevel fl;
    createFirstLevel(&fl);
    shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
    shared_ptr<TPhpSerializeProtocol> prot(new TPhpSerializeProtocol(buffer));
    fl.write(prot.get());
    uint8_t* buf = nullptr;
    uint32_t size;
    buffer->getBuffer(&buf, &size);

    string fileName = "/tmp/." + lexical_cast<string>(i);
    FILE* outFile = fopen(fileName.c_str(), "w");
    fwrite(buf, sizeof(uint8_t), size, outFile);
    fclose(outFile);
    string cmd = string("thrift/test/php/TestNativeDeserialize.php ") +
                 fileName;
    int ret = system(cmd.c_str());
    if (0 != ret) {
      success = false;
      cout << "Test iteration " << i << " failed. Data is at "
           << fileName << endl;
    }
  }

  if (!success) {
    cout << "Test failed\n";
  } else {
    cout << "Test passed\n";
  }

  return 0;
}
