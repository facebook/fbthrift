/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/types/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/types/gen-cpp2/SomeServiceAsyncClient.h"

#include <thrift/lib/cpp2/gen/client_cpp.h>

namespace apache::thrift::fixtures::types {
typedef apache::thrift::ThriftPresult<false, apache::thrift::FieldData<1, ::apache::thrift::type_class::map<::apache::thrift::type_class::integral, ::apache::thrift::type_class::string>, ::apache::thrift::fixtures::types::SomeMap*>> SomeService_bounce_map_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::map<::apache::thrift::type_class::integral, ::apache::thrift::type_class::string>, ::apache::thrift::fixtures::types::SomeMap*>> SomeService_bounce_map_presult;
typedef apache::thrift::ThriftPresult<false, apache::thrift::FieldData<1, ::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, ::std::vector<::std::int64_t>*>> SomeService_binary_keyed_map_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::map<::apache::thrift::type_class::binary, ::apache::thrift::type_class::integral>, ::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>*>> SomeService_binary_keyed_map_presult;
} // namespace apache::thrift::fixtures::types
template <typename RpcOptions>
void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_send_bounce_map(apache::thrift::SerializedRequest&& request, RpcOptions&& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::RequestClientCallback::Ptr callback) {

  static ::apache::thrift::MethodMetadata::Data* methodMetadata =
        new ::apache::thrift::MethodMetadata::Data(
                "bounce_map",
                ::apache::thrift::FunctionQualifier::Unspecified,
                "apache.org/thrift/fixtures/types/SomeService");
  apache::thrift::clientSendT<apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(std::move(request), std::forward<RpcOptions>(rpcOptions), std::move(callback), std::move(header), channel_.get(), ::apache::thrift::MethodMetadata::from_static(methodMetadata), nullptr);
}

template <typename RpcOptions>
void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_send_binary_keyed_map(apache::thrift::SerializedRequest&& request, RpcOptions&& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::RequestClientCallback::Ptr callback) {

  static ::apache::thrift::MethodMetadata::Data* methodMetadata =
        new ::apache::thrift::MethodMetadata::Data(
                "binary_keyed_map",
                ::apache::thrift::FunctionQualifier::Unspecified,
                "apache.org/thrift/fixtures/types/SomeService");
  apache::thrift::clientSendT<apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(std::move(request), std::forward<RpcOptions>(rpcOptions), std::move(callback), std::move(header), channel_.get(), ::apache::thrift::MethodMetadata::from_static(methodMetadata), nullptr);
}



void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::bounce_map(std::unique_ptr<apache::thrift::RequestCallback> callback, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  ::apache::thrift::RpcOptions rpcOptions;
  bounce_map(rpcOptions, std::move(callback), p_m);
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::bounce_map(apache::thrift::RpcOptions& rpcOptions, std::unique_ptr<apache::thrift::RequestCallback> callback, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  auto [ctx, header] = bounce_mapCtx(&rpcOptions);
  auto [wrappedCallback, contextStack] = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(std::move(callback), std::move(ctx));
  fbthrift_serialize_and_send_bounce_map(rpcOptions, std::move(header), contextStack, std::move(wrappedCallback), p_m);
}

apache::thrift::SerializedRequest apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_serialize_bounce_map(const RpcOptions& rpcOptions, apache::thrift::transport::THeader& header, apache::thrift::ContextStack* contextStack, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  return apache::thrift::detail::ac::withProtocolWriter(apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId(), [&](auto&& prot) {
    using ProtocolWriter = std::decay_t<decltype(prot)>;
    ::apache::thrift::fixtures::types::SomeService_bounce_map_pargs args;
    args.get<0>().value = const_cast<::apache::thrift::fixtures::types::SomeMap*>(&p_m);
    const auto sizer = [&](ProtocolWriter* p) { return args.serializedSizeZC(p); };
    const auto writer = [&](ProtocolWriter* p) { args.write(p); };
    return apache::thrift::preprocessSendT<ProtocolWriter>(
        &prot,
        rpcOptions,
        contextStack,
        header,
        "bounce_map",
        writer,
        sizer,
        channel_->getChecksumSamplingRate());
  });
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_serialize_and_send_bounce_map(apache::thrift::RpcOptions& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::ContextStack* contextStack, apache::thrift::RequestClientCallback::Ptr callback, const ::apache::thrift::fixtures::types::SomeMap& p_m, bool stealRpcOptions) {
  apache::thrift::SerializedRequest request = fbthrift_serialize_bounce_map(rpcOptions, *header, contextStack, p_m);
  if (stealRpcOptions) {
    fbthrift_send_bounce_map(std::move(request), std::move(rpcOptions), std::move(header), std::move(callback));
  } else {
    fbthrift_send_bounce_map(std::move(request), rpcOptions, std::move(header), std::move(callback));
  }
}

std::pair<::apache::thrift::ContextStack::UniquePtr, std::shared_ptr<::apache::thrift::transport::THeader>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::bounce_mapCtx(apache::thrift::RpcOptions* rpcOptions) {
  auto header = std::make_shared<apache::thrift::transport::THeader>(
      apache::thrift::transport::THeader::ALLOW_BIG_FRAMES);
  header->setProtocolId(channel_->getProtocolId());
  if (rpcOptions) {
    header->setHeaders(rpcOptions->releaseWriteHeaders());
  }

  auto ctx = apache::thrift::ContextStack::createWithClientContext(
      handlers_,
      interceptors_,
      getServiceName(),
      "SomeService.bounce_map",
      *header);

  return {std::move(ctx), std::move(header)};
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::sync_bounce_map(::apache::thrift::fixtures::types::SomeMap& _return, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  ::apache::thrift::RpcOptions rpcOptions;
  sync_bounce_map(rpcOptions, _return, p_m);
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::sync_bounce_map(apache::thrift::RpcOptions& rpcOptions, ::apache::thrift::fixtures::types::SomeMap& _return, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  apache::thrift::ClientReceiveState returnState;
  apache::thrift::ClientSyncCallback<false> callback(&returnState);
  auto protocolId = apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId();
  auto evb = apache::thrift::GeneratedAsyncClient::getChannel()->getEventBase();
  auto ctxAndHeader = bounce_mapCtx(&rpcOptions);
  auto wrappedCallback = apache::thrift::RequestClientCallback::Ptr(&callback);
  auto* contextStack  = ctxAndHeader.first.get();
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie(p_m);
    contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), ctxAndHeader.second.get()).throwUnlessValue();
  }
  callback.waitUntilDone(
    evb,
    [&] {
      fbthrift_serialize_and_send_bounce_map(rpcOptions, ctxAndHeader.second, ctxAndHeader.first.get(), std::move(wrappedCallback), p_m);
    });
  if (contextStack != nullptr) {
    contextStack->processClientInterceptorsOnResponse(returnState.header()).throwUnlessValue();
  }
  if (returnState.isException()) {
    returnState.exception().throw_exception();
  }
  returnState.resetProtocolId(protocolId);
  returnState.resetCtx(std::move(ctxAndHeader.first));
  SCOPE_EXIT {
    if (returnState.header() && !returnState.header()->getHeaders().empty()) {
      rpcOptions.setReadHeaders(returnState.header()->releaseHeaders());
    }
  };
  return folly::fibers::runInMainContext([&] {
      recv_bounce_map(_return, returnState);
  });
}


template <typename CallbackType>
folly::SemiFuture<::apache::thrift::fixtures::types::SomeMap> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_semifuture_bounce_map(apache::thrift::RpcOptions& rpcOptions, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  using CallbackHelper = apache::thrift::detail::FutureCallbackHelper<::apache::thrift::fixtures::types::SomeMap>;
  folly::Promise<CallbackHelper::PromiseResult> promise;
  auto semifuture = promise.getSemiFuture();
  auto ctxAndHeader = bounce_mapCtx(&rpcOptions);
  auto wrappedCallbackAndContextStack = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(
    std::make_unique<CallbackType>(std::move(promise), recv_wrapped_bounce_map, channel_),
    std::move(ctxAndHeader.first));
  auto header = std::move(ctxAndHeader.second);
  auto* contextStack = wrappedCallbackAndContextStack.second;
  auto wrappedCallback = std::move(wrappedCallbackAndContextStack.first);
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie(p_m);
    if (auto exTry = contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), header.get());
        exTry.hasException()) {
      return folly::makeSemiFuture<::apache::thrift::fixtures::types::SomeMap>(std::move(exTry).exception());
    }
  }
  apache::thrift::SerializedRequest request = fbthrift_serialize_bounce_map(rpcOptions, *header, contextStack, p_m);
  fbthrift_send_bounce_map(std::move(request), rpcOptions, std::move(header), std::move(wrappedCallback));
  return std::move(semifuture).deferValue(CallbackHelper::processClientInterceptorsAndExtractResult);
}

folly::Future<::apache::thrift::fixtures::types::SomeMap> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::future_bounce_map(const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  ::apache::thrift::RpcOptions rpcOptions;
  return future_bounce_map(rpcOptions, p_m);
}

folly::SemiFuture<::apache::thrift::fixtures::types::SomeMap> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::semifuture_bounce_map(const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  ::apache::thrift::RpcOptions rpcOptions;
  return semifuture_bounce_map(rpcOptions, p_m);
}

folly::Future<::apache::thrift::fixtures::types::SomeMap> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::future_bounce_map(apache::thrift::RpcOptions& rpcOptions, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  using CallbackType = apache::thrift::FutureCallback<::apache::thrift::fixtures::types::SomeMap>;
  return fbthrift_semifuture_bounce_map<CallbackType>(rpcOptions, p_m).toUnsafeFuture();
}

folly::SemiFuture<::apache::thrift::fixtures::types::SomeMap> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::semifuture_bounce_map(apache::thrift::RpcOptions& rpcOptions, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  using CallbackType = apache::thrift::SemiFutureCallback<::apache::thrift::fixtures::types::SomeMap>;
  return fbthrift_semifuture_bounce_map<CallbackType>(rpcOptions, p_m);
}


void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::bounce_map(folly::Function<void (::apache::thrift::ClientReceiveState&&)> callback, const ::apache::thrift::fixtures::types::SomeMap& p_m) {
  bounce_map(std::make_unique<apache::thrift::FunctionReplyCallback>(std::move(callback)), p_m);
}

#if FOLLY_HAS_COROUTINES
#endif // FOLLY_HAS_COROUTINES
folly::exception_wrapper apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_wrapped_bounce_map(::apache::thrift::fixtures::types::SomeMap& _return, ::apache::thrift::ClientReceiveState& state) {
  if (state.isException()) {
    return std::move(state.exception());
  }
  if (!state.hasResponseBuffer()) {
    return folly::make_exception_wrapper<apache::thrift::TApplicationException>("recv_ called without result");
  }

  using result = ::apache::thrift::fixtures::types::SomeService_bounce_map_presult;
  switch (state.protocolId()) {
    case apache::thrift::protocol::T_BINARY_PROTOCOL:
    {
      apache::thrift::BinaryProtocolReader reader;
      return apache::thrift::detail::ac::recv_wrapped<result>(
          &reader, state, _return);
    }
    case apache::thrift::protocol::T_COMPACT_PROTOCOL:
    {
      apache::thrift::CompactProtocolReader reader;
      return apache::thrift::detail::ac::recv_wrapped<result>(
          &reader, state, _return);
    }
    default:
    {
    }
  }
  return folly::make_exception_wrapper<apache::thrift::TApplicationException>("Could not find Protocol");
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_bounce_map(::apache::thrift::fixtures::types::SomeMap& _return, ::apache::thrift::ClientReceiveState& state) {
  auto ew = recv_wrapped_bounce_map(_return, state);
  if (ew) {
    ew.throw_exception();
  }
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_instance_bounce_map(::apache::thrift::fixtures::types::SomeMap& _return, ::apache::thrift::ClientReceiveState& state) {
  return recv_bounce_map(_return, state);
}

folly::exception_wrapper apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_instance_wrapped_bounce_map(::apache::thrift::fixtures::types::SomeMap& _return, ::apache::thrift::ClientReceiveState& state) {
  return recv_wrapped_bounce_map(_return, state);
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::binary_keyed_map(std::unique_ptr<apache::thrift::RequestCallback> callback, const ::std::vector<::std::int64_t>& p_r) {
  ::apache::thrift::RpcOptions rpcOptions;
  binary_keyed_map(rpcOptions, std::move(callback), p_r);
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, std::unique_ptr<apache::thrift::RequestCallback> callback, const ::std::vector<::std::int64_t>& p_r) {
  auto [ctx, header] = binary_keyed_mapCtx(&rpcOptions);
  auto [wrappedCallback, contextStack] = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(std::move(callback), std::move(ctx));
  fbthrift_serialize_and_send_binary_keyed_map(rpcOptions, std::move(header), contextStack, std::move(wrappedCallback), p_r);
}

apache::thrift::SerializedRequest apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_serialize_binary_keyed_map(const RpcOptions& rpcOptions, apache::thrift::transport::THeader& header, apache::thrift::ContextStack* contextStack, const ::std::vector<::std::int64_t>& p_r) {
  return apache::thrift::detail::ac::withProtocolWriter(apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId(), [&](auto&& prot) {
    using ProtocolWriter = std::decay_t<decltype(prot)>;
    ::apache::thrift::fixtures::types::SomeService_binary_keyed_map_pargs args;
    args.get<0>().value = const_cast<::std::vector<::std::int64_t>*>(&p_r);
    const auto sizer = [&](ProtocolWriter* p) { return args.serializedSizeZC(p); };
    const auto writer = [&](ProtocolWriter* p) { args.write(p); };
    return apache::thrift::preprocessSendT<ProtocolWriter>(
        &prot,
        rpcOptions,
        contextStack,
        header,
        "binary_keyed_map",
        writer,
        sizer,
        channel_->getChecksumSamplingRate());
  });
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_serialize_and_send_binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::ContextStack* contextStack, apache::thrift::RequestClientCallback::Ptr callback, const ::std::vector<::std::int64_t>& p_r, bool stealRpcOptions) {
  apache::thrift::SerializedRequest request = fbthrift_serialize_binary_keyed_map(rpcOptions, *header, contextStack, p_r);
  if (stealRpcOptions) {
    fbthrift_send_binary_keyed_map(std::move(request), std::move(rpcOptions), std::move(header), std::move(callback));
  } else {
    fbthrift_send_binary_keyed_map(std::move(request), rpcOptions, std::move(header), std::move(callback));
  }
}

std::pair<::apache::thrift::ContextStack::UniquePtr, std::shared_ptr<::apache::thrift::transport::THeader>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::binary_keyed_mapCtx(apache::thrift::RpcOptions* rpcOptions) {
  auto header = std::make_shared<apache::thrift::transport::THeader>(
      apache::thrift::transport::THeader::ALLOW_BIG_FRAMES);
  header->setProtocolId(channel_->getProtocolId());
  if (rpcOptions) {
    header->setHeaders(rpcOptions->releaseWriteHeaders());
  }

  auto ctx = apache::thrift::ContextStack::createWithClientContext(
      handlers_,
      interceptors_,
      getServiceName(),
      "SomeService.binary_keyed_map",
      *header);

  return {std::move(ctx), std::move(header)};
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::sync_binary_keyed_map(::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, const ::std::vector<::std::int64_t>& p_r) {
  ::apache::thrift::RpcOptions rpcOptions;
  sync_binary_keyed_map(rpcOptions, _return, p_r);
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::sync_binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, ::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, const ::std::vector<::std::int64_t>& p_r) {
  apache::thrift::ClientReceiveState returnState;
  apache::thrift::ClientSyncCallback<false> callback(&returnState);
  auto protocolId = apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId();
  auto evb = apache::thrift::GeneratedAsyncClient::getChannel()->getEventBase();
  auto ctxAndHeader = binary_keyed_mapCtx(&rpcOptions);
  auto wrappedCallback = apache::thrift::RequestClientCallback::Ptr(&callback);
  auto* contextStack  = ctxAndHeader.first.get();
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie(p_r);
    contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), ctxAndHeader.second.get()).throwUnlessValue();
  }
  callback.waitUntilDone(
    evb,
    [&] {
      fbthrift_serialize_and_send_binary_keyed_map(rpcOptions, ctxAndHeader.second, ctxAndHeader.first.get(), std::move(wrappedCallback), p_r);
    });
  if (contextStack != nullptr) {
    contextStack->processClientInterceptorsOnResponse(returnState.header()).throwUnlessValue();
  }
  if (returnState.isException()) {
    returnState.exception().throw_exception();
  }
  returnState.resetProtocolId(protocolId);
  returnState.resetCtx(std::move(ctxAndHeader.first));
  SCOPE_EXIT {
    if (returnState.header() && !returnState.header()->getHeaders().empty()) {
      rpcOptions.setReadHeaders(returnState.header()->releaseHeaders());
    }
  };
  return folly::fibers::runInMainContext([&] {
      recv_binary_keyed_map(_return, returnState);
  });
}


template <typename CallbackType>
folly::SemiFuture<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::fbthrift_semifuture_binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, const ::std::vector<::std::int64_t>& p_r) {
  using CallbackHelper = apache::thrift::detail::FutureCallbackHelper<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>>;
  folly::Promise<CallbackHelper::PromiseResult> promise;
  auto semifuture = promise.getSemiFuture();
  auto ctxAndHeader = binary_keyed_mapCtx(&rpcOptions);
  auto wrappedCallbackAndContextStack = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(
    std::make_unique<CallbackType>(std::move(promise), recv_wrapped_binary_keyed_map, channel_),
    std::move(ctxAndHeader.first));
  auto header = std::move(ctxAndHeader.second);
  auto* contextStack = wrappedCallbackAndContextStack.second;
  auto wrappedCallback = std::move(wrappedCallbackAndContextStack.first);
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie(p_r);
    if (auto exTry = contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), header.get());
        exTry.hasException()) {
      return folly::makeSemiFuture<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>>(std::move(exTry).exception());
    }
  }
  apache::thrift::SerializedRequest request = fbthrift_serialize_binary_keyed_map(rpcOptions, *header, contextStack, p_r);
  fbthrift_send_binary_keyed_map(std::move(request), rpcOptions, std::move(header), std::move(wrappedCallback));
  return std::move(semifuture).deferValue(CallbackHelper::processClientInterceptorsAndExtractResult);
}

folly::Future<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::future_binary_keyed_map(const ::std::vector<::std::int64_t>& p_r) {
  ::apache::thrift::RpcOptions rpcOptions;
  return future_binary_keyed_map(rpcOptions, p_r);
}

folly::SemiFuture<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::semifuture_binary_keyed_map(const ::std::vector<::std::int64_t>& p_r) {
  ::apache::thrift::RpcOptions rpcOptions;
  return semifuture_binary_keyed_map(rpcOptions, p_r);
}

folly::Future<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::future_binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, const ::std::vector<::std::int64_t>& p_r) {
  using CallbackType = apache::thrift::FutureCallback<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>>;
  return fbthrift_semifuture_binary_keyed_map<CallbackType>(rpcOptions, p_r).toUnsafeFuture();
}

folly::SemiFuture<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>> apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::semifuture_binary_keyed_map(apache::thrift::RpcOptions& rpcOptions, const ::std::vector<::std::int64_t>& p_r) {
  using CallbackType = apache::thrift::SemiFutureCallback<::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>>;
  return fbthrift_semifuture_binary_keyed_map<CallbackType>(rpcOptions, p_r);
}


void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::binary_keyed_map(folly::Function<void (::apache::thrift::ClientReceiveState&&)> callback, const ::std::vector<::std::int64_t>& p_r) {
  binary_keyed_map(std::make_unique<apache::thrift::FunctionReplyCallback>(std::move(callback)), p_r);
}

#if FOLLY_HAS_COROUTINES
#endif // FOLLY_HAS_COROUTINES
folly::exception_wrapper apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_wrapped_binary_keyed_map(::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, ::apache::thrift::ClientReceiveState& state) {
  if (state.isException()) {
    return std::move(state.exception());
  }
  if (!state.hasResponseBuffer()) {
    return folly::make_exception_wrapper<apache::thrift::TApplicationException>("recv_ called without result");
  }

  using result = ::apache::thrift::fixtures::types::SomeService_binary_keyed_map_presult;
  switch (state.protocolId()) {
    case apache::thrift::protocol::T_BINARY_PROTOCOL:
    {
      apache::thrift::BinaryProtocolReader reader;
      return apache::thrift::detail::ac::recv_wrapped<result>(
          &reader, state, _return);
    }
    case apache::thrift::protocol::T_COMPACT_PROTOCOL:
    {
      apache::thrift::CompactProtocolReader reader;
      return apache::thrift::detail::ac::recv_wrapped<result>(
          &reader, state, _return);
    }
    default:
    {
    }
  }
  return folly::make_exception_wrapper<apache::thrift::TApplicationException>("Could not find Protocol");
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_binary_keyed_map(::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, ::apache::thrift::ClientReceiveState& state) {
  auto ew = recv_wrapped_binary_keyed_map(_return, state);
  if (ew) {
    ew.throw_exception();
  }
}

void apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_instance_binary_keyed_map(::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, ::apache::thrift::ClientReceiveState& state) {
  return recv_binary_keyed_map(_return, state);
}

folly::exception_wrapper apache::thrift::Client<::apache::thrift::fixtures::types::SomeService>::recv_instance_wrapped_binary_keyed_map(::std::map<::apache::thrift::fixtures::types::TBinary, ::std::int64_t>& _return, ::apache::thrift::ClientReceiveState& state) {
  return recv_wrapped_binary_keyed_map(_return, state);
}


