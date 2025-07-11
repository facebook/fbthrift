/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/interactions/src/shared.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/service_h.h>

#include "thrift/compiler/test/fixtures/interactions/gen-cpp2/InteractLocallyAsyncClient.h"
#include "thrift/compiler/test/fixtures/interactions/gen-cpp2/shared_types.h"
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/async/Sink.h>

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

namespace thrift::shared_interactions {
class InteractLocally;
class InteractLocallyAsyncProcessor;

class InteractLocallyServiceInfoHolder : public apache::thrift::ServiceInfoHolder {
  public:
   apache::thrift::ServiceRequestInfoMap const& requestInfoMap() const override;
   static apache::thrift::ServiceRequestInfoMap staticRequestInfoMap();
};
} // namespace thrift::shared_interactions

namespace apache::thrift {
template <>
class ServiceHandler<::thrift::shared_interactions::InteractLocally> : public apache::thrift::ServerInterface {
  static_assert(!folly::is_detected_v<::apache::thrift::detail::st::detect_complete, ::thrift::shared_interactions::InteractLocally>, "Definition collision with service tag. Either rename the Thrift service using @cpp.Name annotation or rename the conflicting C++ type.");

 public:
  std::string_view getGeneratedName() const override { return "InteractLocally"; }

  typedef ::thrift::shared_interactions::InteractLocallyAsyncProcessor ProcessorType;
  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override;
  CreateMethodMetadataResult createMethodMetadata() override;
  bool isThriftGenerated() const override final { return true; }
 private:
  std::optional<std::reference_wrapper<apache::thrift::ServiceRequestInfoMap const>> getServiceRequestInfoMap() const;
 public:
class SharedInteractionServiceInfoHolder : public apache::thrift::ServiceInfoHolder {
  public:
   apache::thrift::ServiceRequestInfoMap const& requestInfoMap() const override;
   static apache::thrift::ServiceRequestInfoMap staticRequestInfoMap();
};


class SharedInteractionIf : public apache::thrift::Tile, public apache::thrift::ServerInterface {
 public:
  std::string_view getGeneratedName() const override { return "SharedInteraction"; }

  typedef ::thrift::shared_interactions::InteractLocallyAsyncProcessor ProcessorType;
  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    std::terminate();
  }
  CreateMethodMetadataResult createMethodMetadata() override {
    std::terminate();
  }
  virtual ::std::int32_t sync_init();
  [[deprecated("Use sync_init instead")]] virtual ::std::int32_t init();
  virtual folly::SemiFuture<::std::int32_t> semifuture_init();
#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<::std::int32_t> co_init();
  virtual folly::coro::Task<::std::int32_t> co_init(apache::thrift::RequestParams params);
#endif
  virtual void async_tm_init(apache::thrift::HandlerCallbackPtr<::std::int32_t> callback);
  virtual void sync_do_something(::thrift::shared_interactions::DoSomethingResult& /*_return*/);
  [[deprecated("Use sync_do_something instead")]] virtual void do_something(::thrift::shared_interactions::DoSomethingResult& /*_return*/);
  virtual folly::SemiFuture<std::unique_ptr<::thrift::shared_interactions::DoSomethingResult>> semifuture_do_something();
#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<std::unique_ptr<::thrift::shared_interactions::DoSomethingResult>> co_do_something();
  virtual folly::coro::Task<std::unique_ptr<::thrift::shared_interactions::DoSomethingResult>> co_do_something(apache::thrift::RequestParams params);
#endif
  virtual void async_tm_do_something(apache::thrift::HandlerCallbackPtr<std::unique_ptr<::thrift::shared_interactions::DoSomethingResult>> callback);
  virtual void sync_tear_down();
  [[deprecated("Use sync_tear_down instead")]] virtual void tear_down();
  virtual folly::SemiFuture<folly::Unit> semifuture_tear_down();
#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<void> co_tear_down();
  virtual folly::coro::Task<void> co_tear_down(apache::thrift::RequestParams params);
#endif
  virtual void async_tm_tear_down(apache::thrift::HandlerCallbackPtr<void> callback);
 private:
  std::atomic<apache::thrift::detail::si::InvocationType> __fbthrift_invocation_init{apache::thrift::detail::si::InvocationType::AsyncTm};
  std::atomic<apache::thrift::detail::si::InvocationType> __fbthrift_invocation_do_something{apache::thrift::detail::si::InvocationType::AsyncTm};
  std::atomic<apache::thrift::detail::si::InvocationType> __fbthrift_invocation_tear_down{apache::thrift::detail::si::InvocationType::AsyncTm};
};
  virtual std::unique_ptr<SharedInteractionIf> createSharedInteraction();
 private:
  static ::thrift::shared_interactions::InteractLocallyServiceInfoHolder __fbthrift_serviceInfoHolder;
  std::atomic<apache::thrift::detail::si::InvocationType> __fbthrift_invocation_createSharedInteraction{apache::thrift::detail::si::InvocationType::AsyncTm};
};

namespace detail {
template <> struct TSchemaAssociation<::thrift::shared_interactions::InteractLocally, false> {
  static ::folly::Range<const ::std::string_view*>(*bundle)();
  static constexpr int64_t programId = -4220872492017656570;
  static constexpr ::std::string_view definitionKey = {"\x56\xd1\x04\xc3\x6f\x07\x7b\x82\x7b\x8a\x05\x38\xf1\xf1\x9b\xc8", 16};
};
}
} // namespace apache::thrift

namespace thrift::shared_interactions {
using InteractLocallySvIf [[deprecated("Use apache::thrift::ServiceHandler<InteractLocally> instead")]] = ::apache::thrift::ServiceHandler<InteractLocally>;
} // namespace thrift::shared_interactions

namespace thrift::shared_interactions {
class InteractLocallySvNull : public ::apache::thrift::ServiceHandler<InteractLocally> {
 public:
};

class InteractLocallyAsyncProcessor : public ::apache::thrift::GeneratedAsyncProcessorBase {
 public:
  std::string_view getServiceName() override;
  void getServiceMetadata(apache::thrift::metadata::ThriftServiceMetadataResponse& response) override;
  using BaseAsyncProcessor = void;
 protected:
  ::apache::thrift::ServiceHandler<::thrift::shared_interactions::InteractLocally>* iface_;
 public:
  void processSerializedCompressedRequestWithMetadata(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, const apache::thrift::AsyncProcessorFactory::MethodMetadata& methodMetadata, apache::thrift::protocol::PROTOCOL_TYPES protType, apache::thrift::Cpp2RequestContext* context, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm) override;
  void executeRequest(apache::thrift::ServerRequest&& serverRequest, const apache::thrift::AsyncProcessorFactory::MethodMetadata& methodMetadata) override;
 public:
  using ProcessFuncs = GeneratedAsyncProcessorBase::ProcessFuncs<InteractLocallyAsyncProcessor>;
  using ProcessMap = GeneratedAsyncProcessorBase::ProcessMap<ProcessFuncs>;
  using InteractionConstructor = GeneratedAsyncProcessorBase::InteractionConstructor<InteractLocallyAsyncProcessor>;
  using InteractionConstructorMap = GeneratedAsyncProcessorBase::InteractionConstructorMap<InteractionConstructor>;
  static const InteractLocallyAsyncProcessor::ProcessMap& getOwnProcessMap();
  static const InteractLocallyAsyncProcessor::InteractionConstructorMap& getInteractionConstructorMap();
  std::unique_ptr<apache::thrift::Tile> createInteractionImpl(const std::string& name, int16_t) override;
 private:
  static const InteractLocallyAsyncProcessor::ProcessMap kOwnProcessMap_;
  static const InteractLocallyAsyncProcessor::InteractionConstructorMap interactionConstructorMap_;
 private:
  std::unique_ptr<apache::thrift::Tile> createSharedInteraction() {
    return iface_->createSharedInteraction();
  }
  template <typename ProtocolIn_, typename ProtocolOut_>
  void setUpAndProcess_SharedInteraction_init(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void executeRequest_SharedInteraction_init(apache::thrift::ServerRequest&& serverRequest);
  template <class ProtocolIn_, class ProtocolOut_>
  static apache::thrift::SerializedResponse return_SharedInteraction_init(apache::thrift::ContextStack* ctx, ::std::int32_t const& _return);
  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped_SharedInteraction_init(apache::thrift::ResponseChannelRequest::UniquePtr req,int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void setUpAndProcess_SharedInteraction_do_something(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void executeRequest_SharedInteraction_do_something(apache::thrift::ServerRequest&& serverRequest);
  template <class ProtocolIn_, class ProtocolOut_>
  static apache::thrift::SerializedResponse return_SharedInteraction_do_something(apache::thrift::ContextStack* ctx, ::thrift::shared_interactions::DoSomethingResult const& _return);
  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped_SharedInteraction_do_something(apache::thrift::ResponseChannelRequest::UniquePtr req,int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void setUpAndProcess_SharedInteraction_tear_down(apache::thrift::ResponseChannelRequest::UniquePtr req, apache::thrift::SerializedCompressedRequest&& serializedRequest, apache::thrift::Cpp2RequestContext* ctx, folly::EventBase* eb, apache::thrift::concurrency::ThreadManager* tm);
  template <typename ProtocolIn_, typename ProtocolOut_>
  void executeRequest_SharedInteraction_tear_down(apache::thrift::ServerRequest&& serverRequest);
  template <class ProtocolIn_, class ProtocolOut_>
  static apache::thrift::SerializedResponse return_SharedInteraction_tear_down(apache::thrift::ContextStack* ctx);
  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped_SharedInteraction_tear_down(apache::thrift::ResponseChannelRequest::UniquePtr req,int32_t protoSeqId,apache::thrift::ContextStack* ctx,folly::exception_wrapper ew,apache::thrift::Cpp2RequestContext* reqCtx);
 public:
  InteractLocallyAsyncProcessor(::apache::thrift::ServiceHandler<::thrift::shared_interactions::InteractLocally>* iface) :
      iface_(iface) {}
  ~InteractLocallyAsyncProcessor() override {}
};

} // namespace thrift::shared_interactions
