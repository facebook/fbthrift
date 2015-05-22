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

#include <folly/Benchmark.h>

#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/THeaderProtocol.h>

#include "thrift/test/gen-cpp/DebugProtoTest_types.h"

#include <gflags/gflags.h>

using namespace boost;
using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;
using namespace thrift::test::debug;

/** An adaptor class that maps the Duplex THeaderProtocolFactory back to a
 * TProtocolFactory
 */
class TSingleHeaderProtocolFactory : public TProtocolFactory {
 public:
  TSingleHeaderProtocolFactory() {}

  ~TSingleHeaderProtocolFactory() override {}

  std::shared_ptr<TProtocol> getProtocol(
      std::shared_ptr<TTransport> trans) override {
    TProtocolPair protocols = factory_.getProtocol(trans);
    return protocols.first;
  }

 private:
  THeaderProtocolFactory factory_;
};

template <class Protocol_>
void WriteOneOfEach(TProtocolFactory& prot_factory, int iters) {
  OneOfEach ooe;
  ooe.im_true   = true;
  ooe.im_false  = false;
  ooe.a_bite    = 0xd6;
  ooe.integer16 = 27000;
  ooe.integer32 = 1<<24;
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
  std::shared_ptr<TProtocol> generic_prot(prot_factory.getProtocol(buf));

  Protocol_* prot = dynamic_cast<Protocol_*>(generic_prot.get());
  if (!prot) {
    abort();
  }

  for (int i = 0; i < iters; ++i) {
    buf->resetBuffer();
    ooe.write(prot);
    prot->getTransport()->flush();
  }
}

template <class Protocol_>
void ReadOneOfEach(TProtocolFactory& prot_factory, int iters) {
  OneOfEach ooe;
  ooe.im_true   = true;
  ooe.im_false  = false;
  ooe.a_bite    = 0xd6;
  ooe.integer16 = 27000;
  ooe.integer32 = 1<<24;
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

  std::shared_ptr<TMemoryBuffer> rbuf(new TMemoryBuffer());
  std::shared_ptr<TMemoryBuffer> wbuf(new TMemoryBuffer());
  std::shared_ptr<TProtocol> generic_iprot(prot_factory.getProtocol(rbuf));
  std::shared_ptr<TProtocol> generic_oprot(prot_factory.getProtocol(wbuf));

  Protocol_* iprot = dynamic_cast<Protocol_*>(generic_iprot.get());
  Protocol_* oprot = dynamic_cast<Protocol_*>(generic_oprot.get());
  if (!iprot || !oprot) {
    abort();
  }

  ooe.write(oprot);
  oprot->getTransport()->flush();
  uint8_t* data;
  uint32_t datasize;
  wbuf->getBuffer(&data, &datasize);

  // Don't construct a new OneOfEach object each time around the loop.
  // The vector insert operations in the constructor take up a significant
  // fraction of the time.
  OneOfEach ooe2;
  for (int i = 0; i < iters; ++i) {
      rbuf->resetBuffer(data, datasize);
      ooe2.read(iprot);
  }
}

BENCHMARK(BM_WriteFullyTemplatized, iters) {
  // Test with TBinaryProtocolT<TBufferBase>,
  // and OneOfEach.write< TBinaryProtocolT<TBufferBase> >
  TBinaryProtocolFactoryT<TBufferBase> prot_factory;
  WriteOneOfEach< TBinaryProtocolT<TBufferBase> >(prot_factory, iters);
}

BENCHMARK(BM_WriteTransportTemplatized, iters) {
  // Test with TBinaryProtocolT<TBufferBase>,
  // but OneOfEach.write<TProtocol>
  TBinaryProtocolFactoryT<TBufferBase> prot_factory;
  WriteOneOfEach<TProtocol>(prot_factory, iters);
}

BENCHMARK(BM_WriteProtocolTemplatized, iters) {
  // Test with TBinaryProtocol and OneOfEach.write<TBinaryProtocol>
  TBinaryProtocolFactory prot_factory;
  WriteOneOfEach<TBinaryProtocol>(prot_factory, iters);
}

BENCHMARK(BM_WriteGeneric, iters) {
  // Test with TBinaryProtocol and OneOfEach.write<TProtocol>
  TBinaryProtocolFactory prot_factory;
  WriteOneOfEach<TProtocol>(prot_factory, iters);
}

BENCHMARK(BM_WriteHeaderGeneric, iters) {
  // Test with TBinaryProtocol and OneOfEach.write<TProtocol>
  TSingleHeaderProtocolFactory prot_factory;
  WriteOneOfEach<THeaderProtocol>(prot_factory, iters);
}

BENCHMARK(BM_ReadFullyTemplatized, iters) {
  // Test with TBinaryProtocolT<TBufferBase>,
  // and OneOfEach.write< TBinaryProtocolT<TBufferBase> >
  TBinaryProtocolFactoryT<TBufferBase> prot_factory;
  ReadOneOfEach< TBinaryProtocolT<TBufferBase> >(prot_factory, iters);
}

BENCHMARK(BM_ReadTransportTemplatized, iters) {
  // Test with TBinaryProtocolT<TBufferBase>,
  // but OneOfEach.write<TProtocol>
  TBinaryProtocolFactoryT<TBufferBase> prot_factory;
  ReadOneOfEach<TProtocol>(prot_factory, iters);
}

BENCHMARK(BM_ReadProtocolTemplatized, iters) {
  // Test with TBinaryProtocol and OneOfEach.write<TBinaryProtocol>
  TBinaryProtocolFactory prot_factory;
  ReadOneOfEach<TBinaryProtocol>(prot_factory, iters);
}

BENCHMARK(BM_ReadGeneric, iters) {
  // Test with TBinaryProtocolT<TTransport> and OneOfEach.write<TProtocol>
  TBinaryProtocolFactory prot_factory;
  ReadOneOfEach<TProtocol>(prot_factory, iters);
}

BENCHMARK(BM_ReadHeaderGeneric, iters) {
  // Test with TBinaryProtocolT<TTransport> and OneOfEach.write<TProtocol>
  TSingleHeaderProtocolFactory prot_factory;
  ReadOneOfEach<THeaderProtocol>(prot_factory, iters);
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  // Run the benchmarks
  folly::runBenchmarksOnFlag();

  return 0;
}
