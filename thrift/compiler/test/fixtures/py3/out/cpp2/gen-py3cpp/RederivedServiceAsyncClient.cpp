/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/py3/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/py3/gen-py3cpp/RederivedServiceAsyncClient.h"

#include <thrift/lib/cpp2/gen/client_cpp.h>

namespace py3::simple {
typedef apache::thrift::ThriftPresult<false> RederivedService_get_seven_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::integral, ::std::int32_t*>> RederivedService_get_seven_presult;
} // namespace py3::simple
template <typename RpcOptions>
void apache::thrift::Client<::py3::simple::RederivedService>::fbthrift_send_get_seven(apache::thrift::SerializedRequest&& request, RpcOptions&& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::RequestClientCallback::Ptr callback, std::unique_ptr<folly::IOBuf> interceptorFrameworkMetadata) {

  static ::apache::thrift::MethodMetadata::Data* methodMetadata =
        new ::apache::thrift::MethodMetadata::Data(
                "get_seven",
                ::apache::thrift::FunctionQualifier::Unspecified,
                "RederivedService");
  apache::thrift::clientSendT<apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE>(std::move(request), std::forward<RpcOptions>(rpcOptions), std::move(callback), std::move(header), channel_.get(), ::apache::thrift::MethodMetadata::from_static(methodMetadata), std::move(interceptorFrameworkMetadata));
}



void apache::thrift::Client<::py3::simple::RederivedService>::get_seven(std::unique_ptr<apache::thrift::RequestCallback> callback) {
  ::apache::thrift::RpcOptions rpcOptions;
  get_seven(rpcOptions, std::move(callback));
}

void apache::thrift::Client<::py3::simple::RederivedService>::get_seven(apache::thrift::RpcOptions& rpcOptions, std::unique_ptr<apache::thrift::RequestCallback> callback) {
  auto [ctx, header] = get_sevenCtx(&rpcOptions);
  auto [wrappedCallback, contextStack] = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(std::move(callback), std::move(ctx));
  fbthrift_serialize_and_send_get_seven(rpcOptions, std::move(header), contextStack, std::move(wrappedCallback));
}

apache::thrift::SerializedRequest apache::thrift::Client<::py3::simple::RederivedService>::fbthrift_serialize_get_seven(const RpcOptions& rpcOptions, apache::thrift::transport::THeader& header, apache::thrift::ContextStack* contextStack) {
  return apache::thrift::detail::ac::withProtocolWriter(apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId(), [&](auto&& prot) {
    using ProtocolWriter = std::decay_t<decltype(prot)>;
    ::py3::simple::RederivedService_get_seven_pargs args;
    const auto sizer = [&](ProtocolWriter* p) { return args.serializedSizeZC(p); };
    const auto writer = [&](ProtocolWriter* p) { args.write(p); };
    return apache::thrift::preprocessSendT<ProtocolWriter>(
        &prot,
        rpcOptions,
        contextStack,
        header,
        "get_seven",
        writer,
        sizer,
        channel_->getChecksumSamplingRate());
  });
}

void apache::thrift::Client<::py3::simple::RederivedService>::fbthrift_serialize_and_send_get_seven(apache::thrift::RpcOptions& rpcOptions, std::shared_ptr<apache::thrift::transport::THeader> header, apache::thrift::ContextStack* contextStack, apache::thrift::RequestClientCallback::Ptr callback, bool stealRpcOptions) {
  apache::thrift::SerializedRequest request = fbthrift_serialize_get_seven(rpcOptions, *header, contextStack);
  std::unique_ptr<folly::IOBuf> interceptorFrameworkMetadata = nullptr;
  if (contextStack != nullptr) {
    interceptorFrameworkMetadata = detail::ContextStackUnsafeAPI(*contextStack).getInterceptorFrameworkMetadata();
  }
  if (stealRpcOptions) {
    fbthrift_send_get_seven(std::move(request), std::move(rpcOptions), std::move(header), std::move(callback), std::move(interceptorFrameworkMetadata));
  } else {
    fbthrift_send_get_seven(std::move(request), rpcOptions, std::move(header), std::move(callback), std::move(interceptorFrameworkMetadata));
  }
}

std::pair<::apache::thrift::ContextStack::UniquePtr, std::shared_ptr<::apache::thrift::transport::THeader>> apache::thrift::Client<::py3::simple::RederivedService>::get_sevenCtx(apache::thrift::RpcOptions* rpcOptions) {
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
      "RederivedService.get_seven",
      *header);

  return {std::move(ctx), std::move(header)};
}

::std::int32_t apache::thrift::Client<::py3::simple::RederivedService>::sync_get_seven() {
  ::apache::thrift::RpcOptions rpcOptions;
  return sync_get_seven(rpcOptions);
}

::std::int32_t apache::thrift::Client<::py3::simple::RederivedService>::sync_get_seven(apache::thrift::RpcOptions& rpcOptions) {
  apache::thrift::ClientReceiveState returnState;
  apache::thrift::ClientSyncCallback<false> callback(&returnState);
  auto protocolId = apache::thrift::GeneratedAsyncClient::getChannel()->getProtocolId();
  auto evb = apache::thrift::GeneratedAsyncClient::getChannel()->getEventBase();
  auto ctxAndHeader = get_sevenCtx(&rpcOptions);
  auto wrappedCallback = apache::thrift::RequestClientCallback::Ptr(&callback);
  auto* contextStack  = ctxAndHeader.first.get();
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie();
    contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), ctxAndHeader.second.get()).throwUnlessValue();
  }
  callback.waitUntilDone(
    evb,
    [&] {
      fbthrift_serialize_and_send_get_seven(rpcOptions, ctxAndHeader.second, ctxAndHeader.first.get(), std::move(wrappedCallback));
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
      return recv_get_seven(returnState);
  });
}


template <typename CallbackType>
folly::SemiFuture<::std::int32_t> apache::thrift::Client<::py3::simple::RederivedService>::fbthrift_semifuture_get_seven(apache::thrift::RpcOptions& rpcOptions) {
  using CallbackHelper = apache::thrift::detail::FutureCallbackHelper<::std::int32_t>;
  folly::Promise<CallbackHelper::PromiseResult> promise;
  auto semifuture = promise.getSemiFuture();
  auto ctxAndHeader = get_sevenCtx(&rpcOptions);
  auto wrappedCallbackAndContextStack = apache::thrift::GeneratedAsyncClient::template prepareRequestClientCallback<false /* kIsOneWay */>(
    std::make_unique<CallbackType>(std::move(promise), recv_wrapped_get_seven, channel_),
    std::move(ctxAndHeader.first));
  auto header = std::move(ctxAndHeader.second);
  auto* contextStack = wrappedCallbackAndContextStack.second;
  auto wrappedCallback = std::move(wrappedCallbackAndContextStack.first);
  std::unique_ptr<folly::IOBuf> interceptorFrameworkMetadata = nullptr;
  if (contextStack != nullptr) {
    auto argsAsRefs = std::tie();
    if (auto exTry = contextStack->processClientInterceptorsOnRequest(apache::thrift::ClientInterceptorOnRequestArguments(argsAsRefs), header.get());
        exTry.hasException()) {
      return folly::makeSemiFuture<::std::int32_t>(std::move(exTry).exception());
    }
    interceptorFrameworkMetadata = detail::ContextStackUnsafeAPI(*contextStack).getInterceptorFrameworkMetadata();
  }
  apache::thrift::SerializedRequest request = fbthrift_serialize_get_seven(rpcOptions, *header, contextStack);
  fbthrift_send_get_seven(std::move(request), rpcOptions, std::move(header), std::move(wrappedCallback), std::move(interceptorFrameworkMetadata));
  return std::move(semifuture).deferValue(CallbackHelper::processClientInterceptorsAndExtractResult);
}

folly::Future<::std::int32_t> apache::thrift::Client<::py3::simple::RederivedService>::future_get_seven() {
  ::apache::thrift::RpcOptions rpcOptions;
  return future_get_seven(rpcOptions);
}

folly::SemiFuture<::std::int32_t> apache::thrift::Client<::py3::simple::RederivedService>::semifuture_get_seven() {
  ::apache::thrift::RpcOptions rpcOptions;
  return semifuture_get_seven(rpcOptions);
}

folly::Future<::std::int32_t> apache::thrift::Client<::py3::simple::RederivedService>::future_get_seven(apache::thrift::RpcOptions& rpcOptions) {
  using CallbackType = apache::thrift::FutureCallback<::std::int32_t>;
  return fbthrift_semifuture_get_seven<CallbackType>(rpcOptions).toUnsafeFuture();
}

folly::SemiFuture<::std::int32_t> apache::thrift::Client<::py3::simple::RederivedService>::semifuture_get_seven(apache::thrift::RpcOptions& rpcOptions) {
  using CallbackType = apache::thrift::SemiFutureCallback<::std::int32_t>;
  return fbthrift_semifuture_get_seven<CallbackType>(rpcOptions);
}


void apache::thrift::Client<::py3::simple::RederivedService>::get_seven(folly::Function<void (::apache::thrift::ClientReceiveState&&)> callback) {
  get_seven(std::make_unique<apache::thrift::FunctionReplyCallback>(std::move(callback)));
}

#if FOLLY_HAS_COROUTINES
#endif // FOLLY_HAS_COROUTINES
folly::exception_wrapper apache::thrift::Client<::py3::simple::RederivedService>::recv_wrapped_get_seven(::std::int32_t& _return, ::apache::thrift::ClientReceiveState& state) {
  if (state.isException()) {
    return std::move(state.exception());
  }
  if (!state.hasResponseBuffer()) {
    return folly::make_exception_wrapper<apache::thrift::TApplicationException>("recv_ called without result");
  }

  using result = ::py3::simple::RederivedService_get_seven_presult;
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

::std::int32_t apache::thrift::Client<::py3::simple::RederivedService>::recv_get_seven(::apache::thrift::ClientReceiveState& state) {
  ::std::int32_t _return;
  auto ew = recv_wrapped_get_seven(_return, state);
  if (ew) {
    ew.throw_exception();
  }
  return _return;
}

::std::int32_t apache::thrift::Client<::py3::simple::RederivedService>::recv_instance_get_seven(::apache::thrift::ClientReceiveState& state) {
  return recv_get_seven(state);
}

folly::exception_wrapper apache::thrift::Client<::py3::simple::RederivedService>::recv_instance_wrapped_get_seven(::std::int32_t& _return, ::apache::thrift::ClientReceiveState& state) {
  return recv_wrapped_get_seven(_return, state);
}


