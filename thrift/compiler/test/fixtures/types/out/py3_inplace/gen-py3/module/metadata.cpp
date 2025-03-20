/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/types/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include "thrift/compiler/test/fixtures/types/gen-py3/module/metadata.h"

namespace apache {
namespace thrift {
namespace fixtures {
namespace types {
::apache::thrift::metadata::ThriftMetadata module_getThriftModuleMetadata() {
  ::apache::thrift::metadata::ThriftServiceMetadataResponse response;
  ::apache::thrift::metadata::ThriftMetadata& metadata = *response.metadata_ref();
  ::apache::thrift::detail::md::EnumMetadata<has_bitwise_ops>::gen(metadata);
  ::apache::thrift::detail::md::EnumMetadata<is_unscoped>::gen(metadata);
  ::apache::thrift::detail::md::EnumMetadata<MyForwardRefEnum>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<empty_struct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<decorated_struct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ContainerStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CppTypeStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<VirtualStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MyStructWithForwardRefEnum>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<TrivialNumeric>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<TrivialNestedWithDefault>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ComplexString>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ComplexNestedWithDefault>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MinPadding>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MinPaddingWithCustomType>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MyStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MyDataItem>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Renaming>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AnnotatedTypes>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ForwardUsageRoot>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ForwardUsageStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ForwardUsageByRef>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<IncompleteMap>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<IncompleteMapDep>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CompleteMap>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CompleteMapDep>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<IncompleteList>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<IncompleteListDep>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CompleteList>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CompleteListDep>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptedList>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<DependentAdaptedList>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AllocatorAware>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AllocatorAware2>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<TypedefStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<StructWithDoubleUnderscores>::gen(metadata);
  ::apache::thrift::detail::md::ServiceMetadata<::apache::thrift::ServiceHandler<::apache::thrift::fixtures::types::SomeService>>::gen(response);
  return metadata;
}
} // namespace apache
} // namespace thrift
} // namespace fixtures
} // namespace types
