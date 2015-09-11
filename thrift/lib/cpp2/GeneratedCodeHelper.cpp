/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace apache { namespace thrift {

namespace detail { namespace ap {

template <typename ProtocolReader, typename ProtocolWriter>
IOBufQueue helper<ProtocolReader, ProtocolWriter>::write_exn(
    const char* method,
    ProtocolWriter* prot,
    int32_t protoSeqId,
    ContextStack* ctx,
    const TApplicationException& x) {
  IOBufQueue queue(IOBufQueue::cacheChainLength());
  size_t bufSize = x.serializedSizeZC(prot);
  bufSize += prot->serializedMessageSize(method);
  prot->setOutput(&queue, bufSize);
  if (ctx) {
    ctx->handlerErrorWrapped(exception_wrapper(x));
  }
  prot->writeMessageBegin(method, T_EXCEPTION, protoSeqId);
  x.write(prot);
  prot->writeMessageEnd();
  return queue;
}

template <typename ProtocolReader, typename ProtocolWriter>
void helper<ProtocolReader, ProtocolWriter>::process_exn(
    const char* func,
    const string& msg,
    unique_ptr<ResponseChannel::Request> req,
    Cpp2RequestContext* ctx,
    TEventBase* eb,
    int32_t protoSeqId) {
  ProtocolWriter oprot;
  if (req) {
    LOG(ERROR) << msg << " in function " << func;
    TApplicationException x(msg);
    IOBufQueue queue = helper_w<ProtocolWriter>::write_exn(
        func, &oprot, protoSeqId, nullptr, x);
    queue.append(THeader::transform(
          queue.move(),
          ctx->getHeader()->getWriteTransforms(),
          ctx->getHeader()->getMinCompressBytes()));
    auto queue_mw = makeMoveWrapper(move(queue));
    auto req_mw = makeMoveWrapper(move(req));
    eb->runInEventBaseThread([=]() mutable {
      (*req_mw)->sendReply(queue_mw->move());
    });
  } else {
    LOG(ERROR) << msg << " in oneway function " << func;
  }
}

template struct helper<BinaryProtocolReader, BinaryProtocolWriter>;
template struct helper<CompactProtocolReader, CompactProtocolWriter>;

template <class ProtocolReader>
static
Optional<string> get_cache_key(
    const IOBuf* buf,
    const unordered_map<string, int16_t>& cache_key_map) {
  string fname;
  MessageType mtype;
  int32_t protoSeqId = 0;
  string pname;
  TType ftype;
  int16_t fid;
  try {
    ProtocolReader iprot;
    iprot.setInput(buf);
    iprot.readMessageBegin(fname, mtype, protoSeqId);
    auto pfn = cache_key_map.find(fname);
    if (pfn == cache_key_map.end()) {
      return none;
    }
    auto cacheKeyParamId = pfn->second;
    uint32_t xfer = 0;
    xfer += iprot.readStructBegin(pname);
    while (true) {
      xfer += iprot.readFieldBegin(pname, ftype, fid);
      if (ftype == T_STOP) {
        break;
      }
      if (fid == cacheKeyParamId) {
        string cacheKey;
        iprot.readString(cacheKey);
        return Optional<string>(move(cacheKey));
      }
      xfer += iprot.skip(ftype);
      xfer += iprot.readFieldEnd();
    }
    return none;
  } catch( const exception& e) {
    LOG(ERROR) << "Caught an exception parsing buffer:" << e.what();
    return none;
  }
}

Optional<string> get_cache_key(
  const IOBuf* buf,
  const PROTOCOL_TYPES protType,
  const unordered_map<string, int16_t>& cache_key_map) {
  switch (protType) {
    case T_BINARY_PROTOCOL:
      return get_cache_key<BinaryProtocolReader>(buf, cache_key_map);
    case T_COMPACT_PROTOCOL:
      return get_cache_key<CompactProtocolReader>(buf, cache_key_map);
    default:
      return none;
  }
}

template <class ProtocolReader>
static
bool is_oneway_method(
    const IOBuf* buf,
    const unordered_set<string>& oneways) {
  string fname;
  MessageType mtype;
  int32_t protoSeqId = 0;
  ProtocolReader iprot;
  iprot.setInput(buf);
  try {
    iprot.readMessageBegin(fname, mtype, protoSeqId);
    auto it = oneways.find(fname);
    return it != oneways.end();
  } catch(const TException& ex) {
    LOG(ERROR) << "received invalid message from client: " << ex.what();
    return false;
  }
}

bool is_oneway_method(
    const IOBuf* buf,
    const THeader* header,
    const unordered_set<string>& oneways) {
  auto protType = static_cast<PROTOCOL_TYPES>(header->getProtocolId());
  switch (protType) {
    case T_BINARY_PROTOCOL:
      return is_oneway_method<BinaryProtocolReader>(buf, oneways);
    case T_COMPACT_PROTOCOL:
      return is_oneway_method<CompactProtocolReader>(buf, oneways);
    default:
      LOG(ERROR) << "invalid protType: " << protType;
      return false;
  }
}

}}

}}
