/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include "thrift/compiler/test/fixtures/serialization_field_order/gen-py3/thrift/metadata.h"

namespace facebook {
namespace thrift {
namespace annotation {
::apache::thrift::metadata::ThriftMetadata thrift_getThriftModuleMetadata() {
  ::apache::thrift::metadata::ThriftServiceMetadataResponse response;
  ::apache::thrift::metadata::ThriftMetadata& metadata = *response.metadata_ref();
  ::apache::thrift::detail::md::EnumMetadata<RpcPriority>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Experimental>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ReserveIds>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<RequiresBackwardCompatibility>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<TerseWrite>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Box>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Mixin>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<SerializeInFieldIdOrder>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<BitmaskEnum>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<ExceptionMessage>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<GenerateRuntimeSchema>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<InternBox>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Serial>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Uri>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<Priority>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<DeprecatedUnvalidatedAnnotations>::gen(metadata);
  return metadata;
}
} // namespace facebook
} // namespace thrift
} // namespace annotation
