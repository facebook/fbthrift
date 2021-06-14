/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <type_traits>
#include <utility>
#include "thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h"

#include <folly/Portability.h>

#include <fmt/core.h>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <folly/futures/Future.h>
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp/protocol/TBase64Utils.h>
#include <thrift/lib/cpp2/SerializationSwitch.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/ClientSinkBridge.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/detail/meta.h>
#include <thrift/lib/cpp2/frozen/Frozen.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/Traits.h>
#include <thrift/lib/cpp2/transport/core/RpcMetadataUtil.h>
#include <thrift/lib/cpp2/util/Frozen2ViewHelpers.h>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/FutureUtil.h>
#endif

namespace apache {
namespace thrift {

class BinaryProtocolReader;
class CompactProtocolReader;

namespace detail {

template <int N, int Size, class F, class Tuple>
struct ForEachImpl {
  static uint32_t forEach(Tuple&& tuple, F&& f) {
    uint32_t res = f(std::get<N>(tuple), N);
    res += ForEachImpl<N + 1, Size, F, Tuple>::forEach(
        std::forward<Tuple>(tuple), std::forward<F>(f));
    return res;
  }
};
template <int Size, class F, class Tuple>
struct ForEachImpl<Size, Size, F, Tuple> {
  static uint32_t forEach(Tuple&& /*tuple*/, F&& /*f*/) { return 0; }
};

template <int N = 0, class F, class Tuple>
uint32_t forEach(Tuple&& tuple, F&& f) {
  return ForEachImpl<
      N,
      std::tuple_size<typename std::remove_reference<Tuple>::type>::value,
      F,
      Tuple>::forEach(std::forward<Tuple>(tuple), std::forward<F>(f));
}

template <int N, int Size, class F, class Tuple>
struct ForEachVoidImpl {
  static void forEach(Tuple&& tuple, F&& f) {
    f(std::get<N>(tuple), N);
    ForEachVoidImpl<N + 1, Size, F, Tuple>::forEach(
        std::forward<Tuple>(tuple), std::forward<F>(f));
  }
};
template <int Size, class F, class Tuple>
struct ForEachVoidImpl<Size, Size, F, Tuple> {
  static void forEach(Tuple&& /*tuple*/, F&& /*f*/) {}
};

template <int N = 0, class F, class Tuple>
void forEachVoid(Tuple&& tuple, F&& f) {
  ForEachVoidImpl<
      N,
      std::tuple_size<typename std::remove_reference<Tuple>::type>::value,
      F,
      Tuple>::forEach(std::forward<Tuple>(tuple), std::forward<F>(f));
}

template <typename Protocol, typename IsSet>
struct Writer {
  Writer(Protocol* prot, const IsSet& isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    using Ops = Cpp2Ops<typename FieldData::ref_type>;

    if (!isset_.getIsSet(index)) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->writeFieldBegin("", Ops::thriftType(), fid);
    xfer += Ops::write(prot_, &ex);
    xfer += prot_->writeFieldEnd();
    return xfer;
  }

 private:
  Protocol* prot_;
  const IsSet& isset_;
};

template <typename Protocol, typename IsSet>
struct Sizer {
  Sizer(Protocol* prot, const IsSet& isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    using Ops = Cpp2Ops<typename FieldData::ref_type>;

    if (!isset_.getIsSet(index)) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->serializedFieldSize("", Ops::thriftType(), fid);
    xfer += Ops::serializedSize(prot_, &ex);
    return xfer;
  }

 private:
  Protocol* prot_;
  const IsSet& isset_;
};

template <typename Protocol, typename IsSet>
struct SizerZC {
  SizerZC(Protocol* prot, const IsSet& isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    using Ops = Cpp2Ops<typename FieldData::ref_type>;

    if (!isset_.getIsSet(index)) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->serializedFieldSize("", Ops::thriftType(), fid);
    xfer += Ops::serializedSizeZC(prot_, &ex);
    return xfer;
  }

 private:
  Protocol* prot_;
  const IsSet& isset_;
};

template <typename Protocol, typename IsSet>
struct Reader {
  Reader(
      Protocol* prot,
      IsSet& isset,
      int16_t fid,
      protocol::TType ftype,
      bool& success)
      : prot_(prot),
        isset_(isset),
        fid_(fid),
        ftype_(ftype),
        success_(success) {}
  template <typename FieldData>
  void operator()(FieldData& fieldData, int index) {
    using Ops = Cpp2Ops<typename FieldData::ref_type>;

    if (ftype_ != Ops::thriftType()) {
      return;
    }

    int16_t myfid = FieldData::fid;
    auto& ex = fieldData.ref();
    if (myfid != fid_) {
      return;
    }

    success_ = true;
    isset_.setIsSet(index);
    Ops::read(prot_, &ex);
  }

 private:
  Protocol* prot_;
  IsSet& isset_;
  int16_t fid_;
  protocol::TType ftype_;
  bool& success_;
};

template <typename T>
T& maybe_remove_pointer(T& x) {
  return x;
}

template <typename T>
T& maybe_remove_pointer(T* x) {
  return *x;
}

template <bool hasIsSet, size_t count>
struct IsSetHelper {
  void setIsSet(size_t /*index*/, bool /*value*/ = true) {}
  bool getIsSet(size_t /*index*/) const { return true; }
};

template <size_t count>
struct IsSetHelper<true, count> {
  void setIsSet(size_t index, bool value = true) { isset_[index] = value; }
  bool getIsSet(size_t index) const { return isset_[index]; }

 private:
  std::array<bool, count> isset_ = {};
};

} // namespace detail

template <int16_t Fid, typename TC, typename T>
struct FieldData {
  static const constexpr int16_t fid = Fid;
  static const constexpr protocol::TType ttype = protocol_type_v<TC, T>;
  typedef TC type_class;
  typedef T type;
  typedef typename std::remove_pointer<T>::type ref_type;
  T value;
  ref_type& ref() {
    return apache::thrift::detail::maybe_remove_pointer(value);
  }
  const ref_type& ref() const {
    return apache::thrift::detail::maybe_remove_pointer(value);
  }
};

template <bool hasIsSet, typename... Field>
class ThriftPresult
    : private std::tuple<Field...>,
      public apache::thrift::detail::IsSetHelper<hasIsSet, sizeof...(Field)> {
  // The fields tuple and IsSetHelper are base classes (rather than members)
  // to employ the empty base class optimization when they are empty
  typedef std::tuple<Field...> Fields;
  typedef apache::thrift::detail::IsSetHelper<hasIsSet, sizeof...(Field)>
      CurIsSetHelper;

 public:
  using size = std::tuple_size<Fields>;

  CurIsSetHelper& isSet() { return *this; }
  const CurIsSetHelper& isSet() const { return *this; }
  Fields& fields() { return *this; }
  const Fields& fields() const { return *this; }

  // returns lvalue ref to the appropriate FieldData
  template <size_t index>
  auto get() -> decltype(std::get<index>(this->fields())) {
    return std::get<index>(this->fields());
  }

  template <size_t index>
  auto get() const -> decltype(std::get<index>(this->fields())) {
    return std::get<index>(this->fields());
  }

  template <class Protocol>
  uint32_t read(Protocol* prot) {
    auto xfer = prot->getCursorPosition();
    std::string fname;
    apache::thrift::protocol::TType ftype;
    int16_t fid;

    prot->readStructBegin(fname);

    while (true) {
      prot->readFieldBegin(fname, ftype, fid);
      if (ftype == apache::thrift::protocol::T_STOP) {
        break;
      }
      bool readSomething = false;
      apache::thrift::detail::forEachVoid(
          fields(),
          apache::thrift::detail::Reader<Protocol, CurIsSetHelper>(
              prot, isSet(), fid, ftype, readSomething));
      if (!readSomething) {
        prot->skip(ftype);
      }
      prot->readFieldEnd();
    }
    prot->readStructEnd();

    return folly::to_narrow(prot->getCursorPosition() - xfer);
  }

  template <class Protocol>
  uint32_t serializedSize(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->serializedStructSize("");
    xfer += apache::thrift::detail::forEach(
        fields(),
        apache::thrift::detail::Sizer<Protocol, CurIsSetHelper>(prot, isSet()));
    xfer += prot->serializedSizeStop();
    return xfer;
  }

  template <class Protocol>
  uint32_t serializedSizeZC(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->serializedStructSize("");
    xfer += apache::thrift::detail::forEach(
        fields(),
        apache::thrift::detail::SizerZC<Protocol, CurIsSetHelper>(
            prot, isSet()));
    xfer += prot->serializedSizeStop();
    return xfer;
  }

  template <class Protocol>
  uint32_t write(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->writeStructBegin("");
    xfer += apache::thrift::detail::forEach(
        fields(),
        apache::thrift::detail::Writer<Protocol, CurIsSetHelper>(
            prot, isSet()));
    xfer += prot->writeFieldStop();
    xfer += prot->writeStructEnd();
    return xfer;
  }
};

template <typename PResults, typename StreamPresult>
struct ThriftPResultStream {
  using StreamPResultType = StreamPresult;
  using FieldsType = PResults;

  PResults fields;
  StreamPresult stream;
};

template <
    typename PResults,
    typename SinkPresult,
    typename FinalResponsePresult>
struct ThriftPResultSink {
  using SinkPResultType = SinkPresult;
  using FieldsType = PResults;
  using FinalResponsePResultType = FinalResponsePresult;

  PResults fields;
  SinkPresult stream;
  FinalResponsePresult finalResponse;
};

template <bool hasIsSet, class... Args>
class Cpp2Ops<ThriftPresult<hasIsSet, Args...>> {
 public:
  typedef ThriftPresult<hasIsSet, Args...> Presult;
  static constexpr protocol::TType thriftType() { return protocol::T_STRUCT; }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Presult* value) {
    return value->write(prot);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Presult* value) {
    return value->read(prot);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Presult* value) {
    return value->serializedSize(prot);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Presult* value) {
    return value->serializedSizeZC(prot);
  }
};

// Forward declaration
namespace detail {
namespace ap {

template <
    typename Protocol,
    typename PResult,
    typename T,
    typename ErrorMapFunc>
folly::Try<StreamPayload> encode_stream_element(folly::Try<T>&& val);

template <typename Protocol, typename PResult, typename T>
folly::Try<T> decode_stream_element(
    folly::Try<apache::thrift::StreamPayload>&& payload);

template <typename Protocol, typename PResult, typename T>
apache::thrift::ClientBufferedStream<T> decode_client_buffered_stream(
    apache::thrift::detail::ClientStreamBridge::ClientPtr streamBridge,
    const BufferOptions& bufferOptions);

template <typename Protocol, typename PResult, typename T>
std::unique_ptr<folly::IOBuf> encode_stream_payload(T&& _item);

template <typename Protocol, typename PResult>
std::unique_ptr<folly::IOBuf> encode_stream_payload(folly::IOBuf&& _item);

template <typename Protocol, typename PResult, typename ErrorMapFunc>
EncodedStreamError encode_stream_exception(folly::exception_wrapper ew);

template <typename Protocol, typename PResult, typename T>
T decode_stream_payload_impl(folly::IOBuf& payload, folly::tag_t<T>);

template <typename Protocol, typename PResult, typename T>
folly::IOBuf decode_stream_payload_impl(
    folly::IOBuf& payload, folly::tag_t<folly::IOBuf>);

template <typename Protocol, typename PResult, typename T>
T decode_stream_payload(folly::IOBuf& payload);

template <typename Protocol, typename PResult, typename T>
folly::exception_wrapper decode_stream_exception(folly::exception_wrapper ew);

struct EmptyExMapType {
  template <typename PResult>
  bool operator()(PResult&, folly::exception_wrapper) {
    return false;
  }
};

} // namespace ap
} // namespace detail

//  AsyncClient helpers
namespace detail {
namespace ac {

template <bool HasReturnType, typename PResult>
folly::exception_wrapper extract_exn(PResult& result) {
  using base = std::integral_constant<std::size_t, HasReturnType ? 1 : 0>;
  auto ew = folly::exception_wrapper();
  if (HasReturnType && result.getIsSet(0)) {
    return ew;
  }
  foreach_index<PResult::size::value - base::value>([&](auto index) {
    if (!ew && result.getIsSet(index.value + base::value)) {
      auto& fdata = result.template get<index.value + base::value>();
      ew = folly::exception_wrapper(std::move(fdata.ref()));
    }
  });
  if (!ew && HasReturnType) {
    ew = folly::make_exception_wrapper<TApplicationException>(
        TApplicationException::TApplicationExceptionType::MISSING_RESULT,
        "failed: unknown result");
  }
  return ew;
}

template <typename Protocol, typename PResult>
folly::exception_wrapper recv_wrapped_helper(
    Protocol* prot, ClientReceiveState& state, PResult& result) {
  ContextStack* ctx = state.ctx();
  MessageType mtype = state.messageType();
  if (ctx) {
    ctx->preRead();
  }
  try {
    const folly::IOBuf& buffer = *state.serializedResponse().buffer;
    // TODO: re-enable checksumming after we properly adjust checksum on the
    // server to exclude the envelope.
    // if (state.header() && state.header()->getCrc32c().has_value() &&
    //     checksum::crc32c(buffer) != *state.header()->getCrc32c()) {
    //   return folly::make_exception_wrapper<TApplicationException>(
    //       TApplicationException::TApplicationExceptionType::CHECKSUM_MISMATCH,
    //       "corrupted response");
    // }
    if (mtype == MessageType::T_EXCEPTION) {
      TApplicationException x;
      apache::thrift::detail::deserializeExceptionBody(prot, &x);
      return folly::exception_wrapper(std::move(x));
    }
    if (mtype != MessageType::T_REPLY) {
      prot->skip(protocol::T_STRUCT);
      return folly::make_exception_wrapper<TApplicationException>(
          TApplicationException::TApplicationExceptionType::
              INVALID_MESSAGE_TYPE);
    }
    SerializedMessage smsg;
    smsg.protocolType = prot->protocolType();
    smsg.buffer = &buffer;
    if (ctx) {
      ctx->onReadData(smsg);
    }
    apache::thrift::detail::deserializeRequestBody(prot, &result);
    if (ctx) {
      ctx->postRead(
          state.header(), folly::to_narrow(buffer.computeChainDataLength()));
    }
    return folly::exception_wrapper();
  } catch (std::exception const& e) {
    return folly::exception_wrapper(std::current_exception(), e);
  } catch (...) {
    return folly::exception_wrapper(std::current_exception());
  }
}

template <typename PResult, typename Protocol, typename... ReturnTs>
folly::exception_wrapper recv_wrapped(
    Protocol* prot, ClientReceiveState& state, ReturnTs&... _returns) {
  prot->setInput(state.serializedResponse().buffer.get());
  auto guard = folly::makeGuard([&] { prot->setInput(nullptr); });
  apache::thrift::ContextStack* ctx = state.ctx();
  PResult result;
  foreach(
      [&](auto index, auto& obj) {
        result.template get<index.value>().value = &obj;
      },
      _returns...);
  auto ew = recv_wrapped_helper(prot, state, result);
  if (!ew) {
    constexpr auto const kHasReturnType = sizeof...(_returns) != 0;
    ew = apache::thrift::detail::ac::extract_exn<kHasReturnType>(result);
  }
  if (ctx && ew) {
    ctx->handlerErrorWrapped(ew);
  }
  return ew;
}

template <typename PResult, typename Protocol, typename Response, typename Item>
folly::exception_wrapper recv_wrapped(
    Protocol* prot,
    ClientReceiveState& state,
    apache::thrift::ResponseAndClientBufferedStream<Response, Item>& _return) {
  prot->setInput(state.serializedResponse().buffer.get());
  auto guard = folly::makeGuard([&] { prot->setInput(nullptr); });
  apache::thrift::ContextStack* ctx = state.ctx();

  typename PResult::FieldsType result;
  result.template get<0>().value = &_return.response;

  auto ew = recv_wrapped_helper(prot, state, result);
  if (!ew) {
    ew = apache::thrift::detail::ac::extract_exn<true>(result);
  }
  if (ctx && ew) {
    ctx->handlerErrorWrapped(ew);
  }

  if (!ew) {
    _return.stream = apache::thrift::detail::ap::decode_client_buffered_stream<
        Protocol,
        typename PResult::StreamPResultType,
        Item>(state.extractStreamBridge(), state.bufferOptions());
  }
  return ew;
}

template <typename PResult, typename Protocol, typename Item>
folly::exception_wrapper recv_wrapped(
    Protocol* prot,
    ClientReceiveState& state,
    apache::thrift::ClientBufferedStream<Item>& _return) {
  prot->setInput(state.serializedResponse().buffer.get());
  auto guard = folly::makeGuard([&] { prot->setInput(nullptr); });
  apache::thrift::ContextStack* ctx = state.ctx();

  typename PResult::FieldsType result;

  auto ew = recv_wrapped_helper(prot, state, result);
  if (!ew) {
    ew = apache::thrift::detail::ac::extract_exn<false>(result);
  }
  if (ctx && ew) {
    ctx->handlerErrorWrapped(ew);
  }

  if (!ew) {
    _return = apache::thrift::detail::ap::decode_client_buffered_stream<
        Protocol,
        typename PResult::StreamPResultType,
        Item>(state.extractStreamBridge(), state.bufferOptions());
  }
  return ew;
}

#if FOLLY_HAS_COROUTINES

template <
    typename ProtocolReader,
    typename ProtocolWriter,
    typename SinkPResult,
    typename SinkType,
    typename FinalResponsePResult,
    typename FinalResponseType,
    typename ErrorMapFunc>
ClientSink<SinkType, FinalResponseType> createSink(
    apache::thrift::detail::ClientSinkBridge::Ptr impl) {
  return ClientSink<SinkType, FinalResponseType>(
      std::move(impl),
      apache::thrift::detail::ap::encode_stream_element<
          ProtocolWriter,
          SinkPResult,
          SinkType,
          std::decay_t<ErrorMapFunc>>,
      apache::thrift::detail::ap::decode_stream_element<
          ProtocolReader,
          FinalResponsePResult,
          FinalResponseType>);
}
#endif

template <
    typename PResult,
    typename ErrorMapFunc,
    typename ProtocolWriter,
    typename ProtocolReader,
    typename Response,
    typename Item,
    typename FinalResponse>
folly::exception_wrapper recv_wrapped(
    ProtocolReader* prot,
    ClientReceiveState& state,
    apache::thrift::detail::ClientSinkBridge::Ptr impl,
    apache::thrift::ResponseAndClientSink<Response, Item, FinalResponse>&
        _return) {
#if FOLLY_HAS_COROUTINES
  prot->setInput(state.serializedResponse().buffer.get());
  auto guard = folly::makeGuard([&] { prot->setInput(nullptr); });
  apache::thrift::ContextStack* ctx = state.ctx();

  typename PResult::FieldsType result;
  result.template get<0>().value = &_return.response;

  auto ew = recv_wrapped_helper(prot, state, result);
  if (!ew) {
    ew = apache::thrift::detail::ac::extract_exn<true>(result);
  }
  if (ctx && ew) {
    ctx->handlerErrorWrapped(ew);
  }

  if (!ew) {
    _return.sink = createSink<
        ProtocolReader,
        ProtocolWriter,
        typename PResult::SinkPResultType,
        Item,
        typename PResult::FinalResponsePResultType,
        FinalResponse,
        std::decay_t<ErrorMapFunc>>(std::move(impl));
  }
  return ew;
#else
  (void)prot;
  (void)state;
  (void)impl;
  (void)_return;
  std::terminate();
#endif
}

template <
    typename PResult,
    typename ErrorMapFunc,
    typename ProtocolWriter,
    typename ProtocolReader,
    typename Item,
    typename FinalResponse>
folly::exception_wrapper recv_wrapped(
    ProtocolReader* prot,
    ClientReceiveState& state,
    apache::thrift::detail::ClientSinkBridge::Ptr impl,
    apache::thrift::ClientSink<Item, FinalResponse>& _return) {
#if FOLLY_HAS_COROUTINES
  prot->setInput(state.serializedResponse().buffer.get());
  auto guard = folly::makeGuard([&] { prot->setInput(nullptr); });
  apache::thrift::ContextStack* ctx = state.ctx();

  typename PResult::FieldsType result;

  auto ew = recv_wrapped_helper(prot, state, result);
  if (!ew) {
    ew = apache::thrift::detail::ac::extract_exn<false>(result);
  }
  if (ctx && ew) {
    ctx->handlerErrorWrapped(ew);
  }

  if (!ew) {
    _return = createSink<
        ProtocolReader,
        ProtocolWriter,
        typename PResult::SinkPResultType,
        Item,
        typename PResult::FinalResponsePResultType,
        FinalResponse,
        std::decay_t<ErrorMapFunc>>(std::move(impl));
  }
  return ew;
#else
  (void)prot;
  (void)state;
  (void)impl;
  (void)_return;
  std::terminate();
#endif
}

[[noreturn]] void throw_app_exn(char const* msg);
} // namespace ac
} // namespace detail

//  AsyncProcessor helpers
namespace detail {
namespace ap {

//  Everything templated on only protocol goes here. The corresponding .cpp file
//  explicitly instantiates this struct for each supported protocol.
template <typename ProtocolReader, typename ProtocolWriter>
struct helper {
  static std::unique_ptr<folly::IOBuf> write_exn(
      const char* method,
      ProtocolWriter* prot,
      int32_t protoSeqId,
      ContextStack* ctx,
      const TApplicationException& x);

  static void process_exn(
      const char* func,
      const TApplicationException::TApplicationExceptionType type,
      const std::string& msg,
      ResponseChannelRequest::UniquePtr req,
      Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      int32_t protoSeqId);
};

template <typename ProtocolReader>
using writer_of = typename ProtocolReader::ProtocolWriter;
template <typename ProtocolWriter>
using reader_of = typename ProtocolWriter::ProtocolReader;

template <typename ProtocolReader>
using helper_r = helper<ProtocolReader, writer_of<ProtocolReader>>;
template <typename ProtocolWriter>
using helper_w = helper<reader_of<ProtocolWriter>, ProtocolWriter>;

template <typename T>
inline constexpr bool is_root_async_processor =
    std::is_void_v<typename T::BaseAsyncProcessor>;

template <typename Derived>
GeneratedAsyncProcessor::ProcessFunc<Derived> getProcessFuncFromProtocol(
    folly::tag_t<CompactProtocolReader> /* unused */,
    const GeneratedAsyncProcessor::ProcessFuncs<Derived>& funcs) {
  return funcs.compact;
}
template <typename Derived>
GeneratedAsyncProcessor::ProcessFunc<Derived> getProcessFuncFromProtocol(
    folly::tag_t<BinaryProtocolReader> /* unused */,
    const GeneratedAsyncProcessor::ProcessFuncs<Derived>& funcs) {
  return funcs.binary;
}

template <class ProtocolReader, class Processor>
void processMissing(
    Processor* processor,
    const std::string& fname,
    ResponseChannelRequest::UniquePtr req,
    apache::thrift::SerializedCompressedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm);

template <class ProtocolReader, class Processor>
void processPmap(
    Processor* proc,
    const typename Processor::ProcessMap& pmap,
    ResponseChannelRequest::UniquePtr req,
    apache::thrift::SerializedCompressedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm) {
  const auto& fname = ctx->getMethodName();
  auto processFuncs = pmap.find(fname);
  if (processFuncs == pmap.end()) {
    processMissing<ProtocolReader>(
        proc, fname, std::move(req), std::move(serializedRequest), ctx, eb, tm);
    return;
  }

  auto pfn = getProcessFuncFromProtocol(
      folly::tag<ProtocolReader>, processFuncs->second);
  (proc->*pfn)(std::move(req), std::move(serializedRequest), ctx, eb, tm);
}

template <class ProtocolReader, class Processor>
void processMissing(
    Processor* processor,
    const std::string& fname,
    ResponseChannelRequest::UniquePtr req,
    apache::thrift::SerializedCompressedRequest&& serializedRequest,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm) {
  if constexpr (is_root_async_processor<Processor>) {
    if (req) {
      eb->runInEventBaseThread(
          [request = move(req),
           msg = fmt::format("Method name {} not found", fname)]() {
            request->sendErrorWrapped(
                folly::make_exception_wrapper<TApplicationException>(
                    TApplicationException::TApplicationExceptionType::
                        UNKNOWN_METHOD,
                    msg),
                kMethodUnknownErrorCode);
          });
    }
  } else {
    using BaseAsyncProcessor = typename Processor::BaseAsyncProcessor;
    processPmap<ProtocolReader, BaseAsyncProcessor>(
        processor,
        BaseAsyncProcessor::getOwnProcessMap(),
        std::move(req),
        std::move(serializedRequest),
        ctx,
        eb,
        tm);
  }
}

//  Generated AsyncProcessor::process just calls this.
template <class Processor>
void process(
    Processor* processor,
    ResponseChannelRequest::UniquePtr req,
    apache::thrift::SerializedCompressedRequest&& serializedRequest,
    protocol::PROTOCOL_TYPES protType,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    concurrency::ThreadManager* tm) {
  const auto& pmap = Processor::getOwnProcessMap();
  switch (protType) {
    case protocol::T_BINARY_PROTOCOL: {
      return processPmap<BinaryProtocolReader>(
          processor,
          pmap,
          std::move(req),
          std::move(serializedRequest),
          ctx,
          eb,
          tm);
    }
    case protocol::T_COMPACT_PROTOCOL: {
      return processPmap<CompactProtocolReader>(
          processor,
          pmap,
          std::move(req),
          std::move(serializedRequest),
          ctx,
          eb,
          tm);
    }
    default:
      LOG(ERROR) << "invalid protType: " << folly::to_underlying(protType);
      return;
  }
}

struct MessageBegin : folly::MoveOnly {
  std::string methodName;
  struct Metadata {
    std::string errMessage;
    size_t size{0};
    int32_t seqId{0};
    MessageType msgType{};
    bool isValid{true};
  } metadata;
};

bool setupRequestContextWithMessageBegin(
    const MessageBegin::Metadata& msgBegin,
    protocol::PROTOCOL_TYPES protType,
    ResponseChannelRequest::UniquePtr& req,
    Cpp2RequestContext* ctx,
    folly::EventBase* eb);

MessageBegin deserializeMessageBegin(
    const folly::IOBuf& buf, protocol::PROTOCOL_TYPES protType);

template <typename Protocol, typename PResult, typename T>
std::unique_ptr<folly::IOBuf> encode_stream_payload(T&& _item) {
  PResult res;
  res.template get<0>().value = const_cast<T*>(&_item);
  res.setIsSet(0);

  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  Protocol prot;
  prot.setOutput(&queue, res.serializedSizeZC(&prot));

  res.write(&prot);
  return std::move(queue).move();
}

template <typename Protocol, typename PResult>
std::unique_ptr<folly::IOBuf> encode_stream_payload(folly::IOBuf&& _item) {
  return std::make_unique<folly::IOBuf>(std::move(_item));
}

template <typename Protocol, typename PResult, typename ErrorMapFunc>
EncodedStreamError encode_stream_exception(folly::exception_wrapper ew) {
  ErrorMapFunc mapException;
  Protocol prot;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  PResult res;

  PayloadExceptionMetadata exceptionMetadata;
  PayloadExceptionMetadataBase exceptionMetadataBase;
  if (mapException(res, ew)) {
    prot.setOutput(&queue, res.serializedSizeZC(&prot));
    res.write(&prot);
    exceptionMetadata.set_declaredException(PayloadDeclaredExceptionMetadata());
  } else {
    constexpr size_t kQueueAppenderGrowth = 4096;
    prot.setOutput(&queue, kQueueAppenderGrowth);
    TApplicationException ex(ew.what().toStdString());
    exceptionMetadataBase.what_utf8_ref() = ex.what();
    apache::thrift::detail::serializeExceptionBody(&prot, &ex);
    PayloadAppUnknownExceptionMetdata aue;
    aue.errorClassification_ref().ensure().blame_ref() = ErrorBlame::SERVER;
    exceptionMetadata.set_appUnknownException(std::move(aue));
  }

  exceptionMetadataBase.metadata_ref() = std::move(exceptionMetadata);
  StreamPayloadMetadata streamPayloadMetadata;
  PayloadMetadata payloadMetadata;
  payloadMetadata.set_exceptionMetadata(std::move(exceptionMetadataBase));
  streamPayloadMetadata.payloadMetadata_ref() = std::move(payloadMetadata);
  return EncodedStreamError(
      StreamPayload(std::move(queue).move(), std::move(streamPayloadMetadata)));
}

template <
    typename Protocol,
    typename PResult,
    typename T,
    typename ErrorMapFunc>
folly::Try<StreamPayload> encode_stream_element(folly::Try<T>&& val) {
  if (val.hasValue()) {
    StreamPayloadMetadata streamPayloadMetadata;
    PayloadMetadata payloadMetadata;
    payloadMetadata.responseMetadata_ref().ensure();
    streamPayloadMetadata.payloadMetadata_ref() = std::move(payloadMetadata);
    return folly::Try<StreamPayload>(
        {encode_stream_payload<Protocol, PResult>(std::move(*val)),
         std::move(streamPayloadMetadata)});
  } else if (val.hasException()) {
    return folly::Try<StreamPayload>(folly::exception_wrapper(
        encode_stream_exception<Protocol, PResult, ErrorMapFunc>(
            val.exception())));
  } else {
    return folly::Try<StreamPayload>();
  }
}

template <typename Protocol, typename PResult, typename T>
T decode_stream_payload_impl(folly::IOBuf& payload, folly::tag_t<T>) {
  PResult args;
  T res{};
  args.template get<0>().value = &res;

  Protocol prot;
  prot.setInput(&payload);
  args.read(&prot);
  return res;
}

template <typename Protocol, typename PResult, typename T>
folly::IOBuf decode_stream_payload_impl(
    folly::IOBuf& payload, folly::tag_t<folly::IOBuf>) {
  return std::move(payload);
}

template <typename Protocol, typename PResult, typename T>
T decode_stream_payload(folly::IOBuf& payload) {
  return decode_stream_payload_impl<Protocol, PResult, T>(
      payload, folly::tag_t<T>{});
}

template <typename Protocol, typename PResult, typename T>
folly::exception_wrapper decode_stream_exception(folly::exception_wrapper ew) {
  folly::exception_wrapper hijacked;
  ew.handle(
      [&hijacked](apache::thrift::detail::EncodedError& err) {
        PResult result;
        T res{};
        result.template get<0>().value = &res;
        Protocol prot;
        prot.setInput(err.encoded.get());
        result.read(&prot);

        CHECK(!result.getIsSet(0));

        foreach_index<PResult::size::value - 1>([&](auto index) {
          if (!hijacked && result.getIsSet(index.value + 1)) {
            auto& fdata = result.template get<index.value + 1>();
            hijacked = folly::exception_wrapper(std::move(fdata.ref()));
          }
        });

        if (!hijacked) {
          // Could not decode the error. It may be a TApplicationException
          TApplicationException x;
          prot.setInput(err.encoded.get());
          apache::thrift::detail::deserializeExceptionBody(&prot, &x);
          hijacked = folly::exception_wrapper(std::move(x));
        }
      },
      [&hijacked](apache::thrift::detail::EncodedStreamError& err) {
        auto& payload = err.encoded;
        DCHECK_EQ(payload.metadata.payloadMetadata_ref().has_value(), true);
        DCHECK_EQ(
            payload.metadata.payloadMetadata_ref()->getType(),
            PayloadMetadata::exceptionMetadata);
        auto& exceptionMetadataBase =
            payload.metadata.payloadMetadata_ref()->get_exceptionMetadata();
        if (auto exceptionMetadataRef = exceptionMetadataBase.metadata_ref()) {
          if (exceptionMetadataRef->getType() ==
              PayloadExceptionMetadata::declaredException) {
            PResult result;
            T res{};
            Protocol prot;
            result.template get<0>().value = &res;
            prot.setInput(payload.payload.get());
            result.read(&prot);
            CHECK(!result.getIsSet(0));
            foreach_index<PResult::size::value - 1>([&](auto index) {
              if (!hijacked && result.getIsSet(index.value + 1)) {
                auto& fdata = result.template get<index.value + 1>();
                hijacked = folly::exception_wrapper(std::move(fdata.ref()));
              }
            });

            if (!hijacked) {
              hijacked =
                  TApplicationException("Failed to parse declared exception");
            }
          } else {
            hijacked = TApplicationException(
                exceptionMetadataBase.what_utf8_ref().value_or(""));
          }
        } else {
          hijacked =
              TApplicationException("Missing payload exception metadata");
        }
      },
      [&hijacked](apache::thrift::detail::EncodedStreamRpcError& err) {
        StreamRpcError streamRpcError;
        CompactProtocolReader reader;
        reader.setInput(err.encoded.get());
        streamRpcError.read(&reader);
        TApplicationException::TApplicationExceptionType exType{
            TApplicationException::UNKNOWN};
        auto code = streamRpcError.code_ref();
        if (code &&
            (code.value() == StreamRpcErrorCode::CREDIT_TIMEOUT ||
             code.value() == StreamRpcErrorCode::CHUNK_TIMEOUT)) {
          exType = TApplicationException::TIMEOUT;
        }
        hijacked = TApplicationException(
            exType, streamRpcError.what_utf8_ref().value_or(""));
      },
      [](...) {});

  if (hijacked) {
    return hijacked;
  }
  return ew;
}

template <
    typename Protocol,
    typename PResult,
    typename ErrorMapFunc,
    typename T>
ServerStreamFactory encode_server_stream(
    apache::thrift::ServerStream<T>&& stream,
    folly::Executor::KeepAlive<> serverExecutor) {
  return stream(
      std::move(serverExecutor),
      encode_stream_element<Protocol, PResult, T, ErrorMapFunc>);
}

template <typename Protocol, typename PResult, typename T>
folly::Try<T> decode_stream_element(
    folly::Try<apache::thrift::StreamPayload>&& payload) {
  if (payload.hasValue()) {
    return folly::Try<T>(
        decode_stream_payload<Protocol, PResult, T>(*payload->payload));
  } else if (payload.hasException()) {
    return folly::Try<T>(decode_stream_exception<Protocol, PResult, T>(
        std::move(payload).exception()));
  } else {
    return folly::Try<T>();
  }
}

template <typename Protocol, typename PResult, typename T>
apache::thrift::ClientBufferedStream<T> decode_client_buffered_stream(
    apache::thrift::detail::ClientStreamBridge::ClientPtr streamBridge,
    const BufferOptions& bufferOptions) {
  return apache::thrift::ClientBufferedStream<T>(
      std::move(streamBridge),
      decode_stream_element<Protocol, PResult, T>,
      bufferOptions);
}

template <
    typename ProtocolReader,
    typename ProtocolWriter,
    typename SinkPResult,
    typename FinalResponsePResult,
    typename ErrorMapFunc,
    typename SinkType,
    typename FinalResponseType>
apache::thrift::detail::SinkConsumerImpl toSinkConsumerImpl(
    FOLLY_MAYBE_UNUSED SinkConsumer<SinkType, FinalResponseType>&& sinkConsumer,
    FOLLY_MAYBE_UNUSED folly::Executor::KeepAlive<> executor) {
#if FOLLY_HAS_COROUTINES
  auto consumer =
      [innerConsumer = std::move(sinkConsumer.consumer)](
          folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> gen) mutable
      -> folly::coro::Task<folly::Try<StreamPayload>> {
    folly::exception_wrapper ew;
    try {
      FinalResponseType finalResponse = co_await innerConsumer(
          [](folly::coro::AsyncGenerator<folly::Try<StreamPayload>&&> gen_)
              -> folly::coro::AsyncGenerator<SinkType&&> {
            while (auto item = co_await gen_.next()) {
              auto payload = std::move(*item);
              co_yield folly::coro::co_result(ap::decode_stream_element<
                                              ProtocolReader,
                                              SinkPResult,
                                              SinkType>(std::move(payload)));
            }
          }(std::move(gen)));
      co_return folly::Try<StreamPayload>(StreamPayload(
          ap::encode_stream_payload<ProtocolWriter, FinalResponsePResult>(
              std::move(finalResponse)),
          {}));
// This causes clang internal error on Windows.
#if !(defined(_WIN32) && defined(__clang__))
    } catch (std::exception& e) {
      ew = folly::exception_wrapper(std::current_exception(), e);
#endif
    } catch (...) {
      ew = folly::exception_wrapper(std::current_exception());
    }
    co_return folly::Try<StreamPayload>(ap::encode_stream_exception<
                                        ProtocolWriter,
                                        FinalResponsePResult,
                                        ErrorMapFunc>(std::move(ew)));
  };
  return apache::thrift::detail::SinkConsumerImpl{
      std::move(consumer),
      sinkConsumer.bufferSize,
      sinkConsumer.sinkOptions.chunkTimeout,
      std::move(executor)};
#else
  std::terminate();
#endif
}

} // namespace ap
} // namespace detail

//  ServerInterface helpers
namespace detail {
namespace si {
template <typename T>
folly::Future<T> future(
    folly::SemiFuture<T>&& future, folly::Executor::KeepAlive<> keepAlive) {
  if (future.isReady()) {
    return std::move(future).toUnsafeFuture();
  }
  return std::move(future).via(keepAlive);
}

using CallbackBase = HandlerCallbackBase;
using CallbackBasePtr = std::unique_ptr<CallbackBase>;
template <typename T>
using Callback = HandlerCallback<T>;
template <typename T>
using CallbackPtr = std::unique_ptr<Callback<T>>;

inline void async_tm_prep(ServerInterface* si, CallbackBase* callback) {
  si->setEventBase(callback->getEventBase());
  si->setThreadManager(callback->getThreadManager());
  si->setRequestContext(callback->getRequestContext());
}

inline void async_tm_future_oneway(
    CallbackBasePtr callback, folly::Future<folly::Unit>&& fut) {
  if (!fut.isReady()) {
    auto ka = callback->getInternalKeepAlive();
    std::move(fut)
        .via(std::move(ka))
        .thenValueInline([cb = std::move(callback)](auto&&) {});
  }
}

template <typename T>
void async_tm_future(
    CallbackPtr<T> callback, folly::Future<folly::lift_unit_t<T>>&& fut) {
  if (!fut.isReady()) {
    auto ka = callback->getInternalKeepAlive();
    std::move(fut)
        .via(std::move(ka))
        .thenTryInline([cb = std::move(callback)](
                           folly::Try<folly::lift_unit_t<T>>&& ret) {
          cb->complete(std::move(ret));
        });
  } else {
    callback->complete(std::move(fut).result());
  }
}

inline void async_tm_semifuture_oneway(
    CallbackBasePtr callback, folly::SemiFuture<folly::Unit>&& fut) {
  if (!fut.isReady()) {
    auto ka = callback->getInternalKeepAlive();
    std::move(fut)
        .via(std::move(ka))
        .thenValueInline([cb = std::move(callback)](auto&&) {});
  }
}

template <typename T>
void async_tm_semifuture(
    CallbackPtr<T> callback, folly::SemiFuture<folly::lift_unit_t<T>>&& fut) {
  if (!fut.isReady()) {
    auto ka = callback->getInternalKeepAlive();
    std::move(fut)
        .via(std::move(ka))
        .thenTryInline([cb = std::move(callback)](
                           folly::Try<folly::lift_unit_t<T>>&& ret) {
          cb->complete(std::move(ret));
        });
  } else {
    callback->complete(std::move(fut).result());
  }
}

#if FOLLY_HAS_COROUTINES
inline void async_tm_coro_oneway(
    CallbackBasePtr callback, folly::coro::Task<void>&& task) {
  auto ka = callback->getInternalKeepAlive();
  std::move(task)
      .scheduleOn(std::move(ka))
      .startInlineUnsafe([callback = std::move(callback)](auto&&) {});
}

template <typename T>
void async_tm_coro(CallbackPtr<T> callback, folly::coro::Task<T>&& task) {
  auto ka = callback->getInternalKeepAlive();
  std::move(task)
      .scheduleOn(std::move(ka))
      .startInlineUnsafe([callback = std::move(callback)](
                             folly::Try<folly::lift_unit_t<T>>&& tryResult) {
        callback->complete(std::move(tryResult));
      });
}
#endif

[[noreturn]] void throw_app_exn_unimplemented(char const* name);
} // namespace si
} // namespace detail

namespace util {

namespace detail {

constexpr ErrorKind fromExceptionKind(ExceptionKind kind) {
  switch (kind) {
    case ExceptionKind::TRANSIENT:
      return ErrorKind::TRANSIENT;

    case ExceptionKind::STATEFUL:
      return ErrorKind::STATEFUL;

    case ExceptionKind::PERMANENT:
      return ErrorKind::PERMANENT;

    default:
      return ErrorKind::UNSPECIFIED;
  }
}

constexpr ErrorBlame fromExceptionBlame(ExceptionBlame blame) {
  switch (blame) {
    case ExceptionBlame::SERVER:
      return ErrorBlame::SERVER;

    case ExceptionBlame::CLIENT:
      return ErrorBlame::CLIENT;

    default:
      return ErrorBlame::UNSPECIFIED;
  }
}

constexpr ErrorSafety fromExceptionSafety(ExceptionSafety safety) {
  switch (safety) {
    case ExceptionSafety::SAFE:
      return ErrorSafety::SAFE;

    default:
      return ErrorSafety::UNSPECIFIED;
  }
}

template <typename T>
std::string serializeExceptionMeta() {
  ErrorClassification errorClassification;

  constexpr auto errorKind = apache::thrift::detail::st::struct_private_access::
      __fbthrift_cpp2_gen_exception_kind<T>();
  errorClassification.kind_ref() = fromExceptionKind(errorKind);
  constexpr auto errorBlame = apache::thrift::detail::st::
      struct_private_access::__fbthrift_cpp2_gen_exception_blame<T>();
  errorClassification.blame_ref() = fromExceptionBlame(errorBlame);
  constexpr auto errorSafety = apache::thrift::detail::st::
      struct_private_access::__fbthrift_cpp2_gen_exception_safety<T>();
  errorClassification.safety_ref() = fromExceptionSafety(errorSafety);

  return apache::thrift::detail::serializeErrorClassification(
      errorClassification);
}

} // namespace detail

void appendExceptionToHeader(
    const folly::exception_wrapper& ew, Cpp2RequestContext& ctx);

template <typename T>
void appendErrorClassificationToHeader(Cpp2RequestContext& ctx) {
  auto header = ctx.getHeader();
  if (!header) {
    return;
  }
  auto exMeta = detail::serializeExceptionMeta<T>();
  header->setHeader(
      std::string(apache::thrift::detail::kHeaderExMeta), std::move(exMeta));
}

TApplicationException toTApplicationException(
    const folly::exception_wrapper& ew);

bool includeInRecentRequestsCount(const std::string_view methodName);

} // namespace util

} // namespace thrift
} // namespace apache
