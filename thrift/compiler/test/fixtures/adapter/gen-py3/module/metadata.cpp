/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include "thrift/compiler/test/fixtures/adapter/gen-py3/module/metadata.h"

namespace facebook {
namespace thrift {
namespace test {
::apache::thrift::metadata::ThriftMetadata module_getThriftModuleMetadata() {
  ::apache::thrift::metadata::ThriftServiceMetadataResponse response;
  ::apache::thrift::metadata::ThriftMetadata& metadata = *response.metadata_ref();
  ::apache::thrift::detail::md::EnumMetadata<Color>::gen(metadata);
  ::apache::thrift::detail::md::EnumMetadata<ThriftAdaptedEnum>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MyAnnotation>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Foo>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Baz>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Bar>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<StructWithFieldAdapter>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<TerseAdaptedFields>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<B>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<A>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Config>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MyStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptTestStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptTemplatedTestStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptTemplatedNestedTestStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptTestUnion>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AdaptedStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<StructFieldAdaptedStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CircularAdaptee>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CircularStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ReorderedStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<MoveOnly>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<AlsoMoveOnly>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ApplyAdapter>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<CountingStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Person>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Person2>::gen(metadata);
  ::apache::thrift::detail::md::ServiceMetadata<::apache::thrift::ServiceHandler<::facebook::thrift::test::Service>>::gen(response);
  ::apache::thrift::detail::md::ServiceMetadata<::apache::thrift::ServiceHandler<::facebook::thrift::test::AdapterService>>::gen(response);
  return metadata;
}
} // namespace facebook
} // namespace thrift
} // namespace test
