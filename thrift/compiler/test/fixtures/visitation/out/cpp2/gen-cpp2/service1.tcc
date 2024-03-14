/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/visitation/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include "thrift/compiler/test/fixtures/visitation/gen-cpp2/service1.h"

#include <thrift/lib/cpp2/gen/service_tcc.h>

namespace test_cpp2 { namespace cpp_reflection {
typedef apache::thrift::ThriftPresult<false> service1_method1_pargs;
typedef apache::thrift::ThriftPresult<true> service1_method1_presult;
typedef apache::thrift::ThriftPresult<false, apache::thrift::FieldData<1, ::apache::thrift::type_class::integral, ::std::int32_t*>, apache::thrift::FieldData<2, ::apache::thrift::type_class::structure, ::test_cpp2::cpp_reflection::struct1*>, apache::thrift::FieldData<3, ::apache::thrift::type_class::floating_point, double*>> service1_method2_pargs;
typedef apache::thrift::ThriftPresult<true> service1_method2_presult;
typedef apache::thrift::ThriftPresult<false> service1_method3_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::integral, ::std::int32_t*>> service1_method3_presult;
typedef apache::thrift::ThriftPresult<false, apache::thrift::FieldData<1, ::apache::thrift::type_class::integral, ::std::int32_t*>, apache::thrift::FieldData<2, ::apache::thrift::type_class::structure, ::test_cpp2::cpp_reflection::struct1*>, apache::thrift::FieldData<3, ::apache::thrift::type_class::floating_point, double*>> service1_method4_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::integral, ::std::int32_t*>> service1_method4_presult;
typedef apache::thrift::ThriftPresult<false> service1_method5_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::structure, ::test_cpp2::cpp_reflection::struct2*>> service1_method5_presult;
typedef apache::thrift::ThriftPresult<false, apache::thrift::FieldData<1, ::apache::thrift::type_class::integral, ::std::int32_t*>, apache::thrift::FieldData<2, ::apache::thrift::type_class::structure, ::test_cpp2::cpp_reflection::struct1*>, apache::thrift::FieldData<3, ::apache::thrift::type_class::floating_point, double*>> service1_method6_pargs;
typedef apache::thrift::ThriftPresult<true, apache::thrift::FieldData<0, ::apache::thrift::type_class::structure, ::test_cpp2::cpp_reflection::struct2*>> service1_method6_presult;
template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method1(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method1<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method1(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method1_pargs args;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method1",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method1", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method1");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<void>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method1<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method1<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method1(std::move(callback));
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method1(apache::thrift::ContextStack* ctx) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method1_presult result;
  return serializeResponse("method1", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method1(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method1");
    return;
  }
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method2(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method2<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method2(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method2_pargs args;
  ::std::int32_t uarg_x{0};
  args.get<0>().value = &uarg_x;
  auto uarg_y = std::make_unique<::test_cpp2::cpp_reflection::struct1>();
  args.get<1>().value = uarg_y.get();
  double uarg_z{0};
  args.get<2>().value = &uarg_z;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method2",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method2", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method2");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<void>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method2<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method2<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method2(std::move(callback), args.get<0>().ref(), std::move(uarg_y), args.get<2>().ref());
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method2(apache::thrift::ContextStack* ctx) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method2_presult result;
  return serializeResponse("method2", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method2(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method2");
    return;
  }
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method3(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method3<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method3(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method3_pargs args;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method3",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method3", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method3");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<::std::int32_t>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method3<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method3<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method3(std::move(callback));
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method3(apache::thrift::ContextStack* ctx, ::std::int32_t const& _return) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method3_presult result;
  result.get<0>().value = const_cast<::std::int32_t*>(&_return);
  result.setIsSet(0, true);
  return serializeResponse("method3", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method3(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method3");
    return;
  }
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method4(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method4<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method4(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method4_pargs args;
  ::std::int32_t uarg_i{0};
  args.get<0>().value = &uarg_i;
  auto uarg_j = std::make_unique<::test_cpp2::cpp_reflection::struct1>();
  args.get<1>().value = uarg_j.get();
  double uarg_k{0};
  args.get<2>().value = &uarg_k;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method4",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method4", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method4");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<::std::int32_t>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method4<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method4<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method4(std::move(callback), args.get<0>().ref(), std::move(uarg_j), args.get<2>().ref());
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method4(apache::thrift::ContextStack* ctx, ::std::int32_t const& _return) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method4_presult result;
  result.get<0>().value = const_cast<::std::int32_t*>(&_return);
  result.setIsSet(0, true);
  return serializeResponse("method4", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method4(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method4");
    return;
  }
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method5(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method5<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method5(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method5_pargs args;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method5",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method5", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method5");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<std::unique_ptr<::test_cpp2::cpp_reflection::struct2>>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method5<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method5<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method5(std::move(callback));
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method5(apache::thrift::ContextStack* ctx, ::test_cpp2::cpp_reflection::struct2 const& _return) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method5_presult result;
  result.get<0>().value = const_cast<::test_cpp2::cpp_reflection::struct2*>(&_return);
  result.setIsSet(0, true);
  return serializeResponse("method5", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method5(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method5");
    return;
  }
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::setUpAndProcess_method6(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, [[maybe_unused]] apache::thrift::concurrency::ThreadManager* tm) {
  if (!setUpRequestProcessing(req, ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, iface_)) {
    return;
  }
  auto scope = iface_->getRequestExecutionScope(ctx, apache::thrift::concurrency::NORMAL);
  ctx->setRequestExecutionScope(std::move(scope));
  processInThread(std::move(req), std::move(serializedRequest), ctx, eb, tm, apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE, &service1AsyncProcessor::executeRequest_method6<ProtocolIn_, ProtocolOut_>, this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void service1AsyncProcessor::executeRequest_method6(apache::thrift::ServerRequest&& serverRequest) {
  // make sure getRequestContext is null
  // so async calls don't accidentally use it
  iface_->setRequestContext(nullptr);
  ::test_cpp2::cpp_reflection::service1_method6_pargs args;
  ::std::int32_t uarg_l{0};
  args.get<0>().value = &uarg_l;
  auto uarg_m = std::make_unique<::test_cpp2::cpp_reflection::struct1>();
  args.get<1>().value = uarg_m.get();
  double uarg_n{0};
  args.get<2>().value = &uarg_n;
  auto ctxStack = apache::thrift::ContextStack::create(
    this->getEventHandlersSharedPtr(),
    this->getServiceName(),
    "service1.method6",
    serverRequest.requestContext());
  try {
    deserializeRequest<ProtocolIn_>(args, "method6", apache::thrift::detail::ServerRequestHelper::compressedRequest(std::move(serverRequest)).uncompress(), ctxStack.get());
  }
  catch (...) {
    folly::exception_wrapper ew(std::current_exception());
    apache::thrift::detail::ap::process_handle_exn_deserialization<ProtocolOut_>(
        ew
        , apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
        , serverRequest.requestContext()
        , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
        , "method6");
    return;
  }
  auto requestPileNotification = apache::thrift::detail::ServerRequestHelper::moveRequestPileNotification(serverRequest);
  auto concurrencyControllerNotification = apache::thrift::detail::ServerRequestHelper::moveConcurrencyControllerNotification(serverRequest);
  auto callback = std::make_unique<apache::thrift::HandlerCallback<std::unique_ptr<::test_cpp2::cpp_reflection::struct2>>>(
    apache::thrift::detail::ServerRequestHelper::request(std::move(serverRequest))
    , std::move(ctxStack)
    , return_method6<ProtocolIn_,ProtocolOut_>
    , throw_wrapped_method6<ProtocolIn_, ProtocolOut_>
    , serverRequest.requestContext()->getProtoSeqId()
    , apache::thrift::detail::ServerRequestHelper::eventBase(serverRequest)
    , apache::thrift::detail::ServerRequestHelper::executor(serverRequest)
    , serverRequest.requestContext()
    , requestPileNotification
    , concurrencyControllerNotification, std::move(serverRequest.requestData())
    );
  iface_->async_tm_method6(std::move(callback), args.get<0>().ref(), std::move(uarg_m), args.get<2>().ref());
}

template <class ProtocolIn_, class ProtocolOut_>
apache::thrift::SerializedResponse service1AsyncProcessor::return_method6(apache::thrift::ContextStack* ctx, ::test_cpp2::cpp_reflection::struct2 const& _return) {
  ProtocolOut_ prot;
  ::test_cpp2::cpp_reflection::service1_method6_presult result;
  result.get<0>().value = const_cast<::test_cpp2::cpp_reflection::struct2*>(&_return);
  result.setIsSet(0, true);
  return serializeResponse("method6", &prot, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void service1AsyncProcessor::throw_wrapped_method6(apache::thrift::ResponseChannelRequest::UniquePtr req,[[maybe_unused]] int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  {
    apache::thrift::detail::ap::process_throw_wrapped_handler_error<ProtocolOut_>(
        ew, std::move(req), reqCtx, ctx, "method6");
    return;
  }
}


}} // test_cpp2::cpp_reflection