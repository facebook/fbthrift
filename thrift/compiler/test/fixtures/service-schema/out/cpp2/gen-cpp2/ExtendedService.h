/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/service-schema/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/service_h.h>

#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/ExtendedServiceAsyncClient.h"
#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/module_types.h"
#if __has_include("thrift/compiler/test/fixtures/service-schema/gen-cpp2/BaseService.h")
#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/BaseService.h"
#else
#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/extend_handlers.h"
#endif
#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/extend_types.h"
#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/include_types.h"

namespace folly {
  class IOBuf;
  class IOBufQueue;
}
namespace apache { namespace thrift {
  class Cpp2RequestContext;
  class BinaryProtocolReader;
  class CompactProtocolReader;
  namespace transport { class THeader; }
}}

namespace facebook::thrift::test {
class ExtendedService;
class ExtendedServiceAsyncProcessor;

class ExtendedServiceServiceInfoHolder : public apache::thrift::ServiceInfoHolder {
  public:
   apache::thrift::ServiceRequestInfoMap const& requestInfoMap() const override;
   static apache::thrift::ServiceRequestInfoMap staticRequestInfoMap();
};
} // namespace facebook::thrift::test

namespace apache::thrift {
template <>
class ServiceHandler<::facebook::thrift::test::ExtendedService> : virtual public ::facebook::thrift::test::BaseServiceSvIf {
  static_assert(!folly::is_detected_v<::apache::thrift::detail::st::detect_complete, ::facebook::thrift::test::ExtendedService>, "Definition collision with service tag. Either rename the Thrift service using @cpp.Name annotation or rename the conflicting C++ type.");

 public:
  std::string_view getGeneratedName() const override { return "ExtendedService"; }

  static std::string_view __fbthrift_thrift_uri() {
    return "facebook.com/thrift/test/ExtendedService";
  }

  typedef ::facebook::thrift::test::ExtendedServiceAsyncProcessor ProcessorType;
  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override;
  CreateMethodMetadataResult createMethodMetadata() override;
  #if defined(THRIFT_SCHEMA_AVAILABLE)
  std::optional<schema::DefinitionsSchema> getServiceSchema() override;
  std::vector<folly::not_null<const syntax_graph::ServiceNode*>> getServiceSchemaNodes() override;
  #endif
 private:
  std::optional<std::reference_wrapper<apache::thrift::ServiceRequestInfoMap const>> getServiceRequestInfoMap() const;
 public:

  virtual ::std::int64_t sync_init(::std::int64_t /*param0*/, ::std::int64_t /*param1*/);
  [[deprecated("Use sync_init instead")]] virtual ::std::int64_t init(::std::int64_t /*param0*/, ::std::int64_t /*param1*/);
  virtual folly::Future<::std::int64_t> future_init(::std::int64_t p_param0, ::std::int64_t p_param1);
  virtual folly::SemiFuture<::std::int64_t> semifuture_init(::std::int64_t p_param0, ::std::int64_t p_param1);
#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<::std::int64_t> co_init(::std::int64_t p_param0, ::std::int64_t p_param1);
  virtual folly::coro::Task<::std::int64_t> co_init(apache::thrift::RequestParams params, ::std::int64_t p_param0, ::std::int64_t p_param1);
#endif
  virtual void async_tm_init(apache::thrift::HandlerCallbackPtr<::std::int64_t> callback, ::std::int64_t p_param0, ::std::int64_t p_param1);
 private:
  static ::facebook::thrift::test::ExtendedServiceServiceInfoHolder __fbthrift_serviceInfoHolder;
  std::atomic<apache::thrift::detail::si::InvocationType> __fbthrift_invocation_init{apache::thrift::detail::si::InvocationType::AsyncTm};
};

namespace detail {
template <> struct TSchemaAssociation<::facebook::thrift::test::ExtendedService, false> {
  static ::folly::Range<const ::std::string_view*>(*bundle)();
  static constexpr int64_t programId = -4897237288056697529;
  static constexpr ::std::string_view definitionKey = {"\x2f\x39\x63\x5e\x7a\x62\x4d\xa7\x6e\x69\x78\xae\x7e\x49\xe3\x79", 16};
};
}
} // namespace apache::thrift

namespace facebook::thrift::test {
using ExtendedServiceSvIf [[deprecated("Use apache::thrift::ServiceHandler<ExtendedService> instead")]] = ::apache::thrift::ServiceHandler<ExtendedService>;
} // namespace facebook::thrift::test

namespace facebook::thrift::test {
class ExtendedServiceSvNull : public ::apache::thrift::ServiceHandler<ExtendedService>, virtual public ::apache::thrift::ServiceHandler<::facebook::thrift::test::BaseService> {
 public:
  ::std::int64_t init(::std::int64_t /*param0*/, ::std::int64_t /*param1*/) override;
};

class ExtendedServiceAsyncProcessor : public ::facebook::thrift::test::BaseServiceAsyncProcessor {
 public:
  std::string_view getServiceName() override;
  void getServiceMetadata(apache::thrift::metadata::ThriftServiceMetadataResponse& response) override;
  using BaseAsyncProcessor = ::facebook::thrift::test::BaseServiceAsyncProcessor;
 protected:
  ::apache::thrift::ServiceHandler<::facebook::thrift::test::ExtendedService>* iface_;
 public:
  void processSerializedCompressedRequestWithMetadata(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, const apache::thrift::AsyncProcessorFactory::MethodMetadata& methodMetadata, apache::thrift::protocol::PROTOCOL_TYPES protType, apache::thrift::Cpp2RequestContext* context, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm) override;
  void executeRequest(apache::thrift::ServerRequest&& serverRequest, const apache::thrift::AsyncProcessorFactory::MethodMetadata& methodMetadata) override;
 public:
  using ProcessFuncs = GeneratedAsyncProcessorBase::ProcessFuncs<ExtendedServiceAsyncProcessor>;
  using ProcessMap = GeneratedAsyncProcessorBase::ProcessMap<ProcessFuncs>;
  static const ExtendedServiceAsyncProcessor::ProcessMap& getOwnProcessMap();
 private:
  static const ExtendedServiceAsyncProcessor::ProcessMap kOwnProcessMap_;
 private:
  template <typename ProtocolIn_, typename ProtocolOut_>
  void setUpAndProcess_init(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void executeRequest_init(apache::thrift::ServerRequest&& serverRequest);
  template <class ProtocolIn_, class ProtocolOut_>
  static apache::thrift::SerializedResponse return_init(apache::thrift::ContextStack* ctx, ::std::int64_t const& _return);
  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped_init(apache::thrift::ResponseChannelRequest::UniquePtr req,int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx);
 public:
  ExtendedServiceAsyncProcessor(::apache::thrift::ServiceHandler<::facebook::thrift::test::ExtendedService>* iface) :
      ::facebook::thrift::test::BaseServiceAsyncProcessor(iface),
      iface_(iface) {}
  ~ExtendedServiceAsyncProcessor() override {}
};

} // namespace facebook::thrift::test
