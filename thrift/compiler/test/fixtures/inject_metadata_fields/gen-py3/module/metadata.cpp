/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#include "thrift/compiler/test/fixtures/inject_metadata_fields/gen-py3/module/metadata.h"

namespace cpp2 {
::apache::thrift::metadata::ThriftMetadata module_getThriftModuleMetadata() {
  ::apache::thrift::metadata::ThriftServiceMetadataResponse response;
  ::apache::thrift::metadata::ThriftMetadata& metadata = *response.metadata_ref();
  ::apache::thrift::detail::md::StructMetadata<Fields>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<FieldsInjectedToEmptyStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<FieldsInjectedToStruct>::gen(metadata);
  ::apache::thrift::detail::md::StructMetadata<FieldsInjectedWithIncludedStruct>::gen(metadata);
  return metadata;
}
} // namespace cpp2
