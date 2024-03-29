/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/adapter/src/module_no_uri.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include <thrift/lib/cpp2/gen/module_metadata_cpp.h>
#include "thrift/compiler/test/fixtures/adapter/gen-cpp2/module_no_uri_metadata.h"

// some of these functions can be so large that the compiler gives up optimizing
// them - and issues a warning which may be treated as an error!
//
// these functions are so rarely called that it is probably okay for them not to
// be optimized in practice
FOLLY_CLANG_DISABLE_WARNING("-Wignored-optimization-argument")

namespace apache {
namespace thrift {
namespace detail {
namespace md {
using ThriftMetadata = ::apache::thrift::metadata::ThriftMetadata;
using ThriftPrimitiveType = ::apache::thrift::metadata::ThriftPrimitiveType;
using ThriftType = ::apache::thrift::metadata::ThriftType;
using ThriftService = ::apache::thrift::metadata::ThriftService;
using ThriftServiceContext = ::apache::thrift::metadata::ThriftServiceContext;
using ThriftFunctionGenerator = void (*)(ThriftMetadata&, ThriftService&);


const ::apache::thrift::metadata::ThriftStruct&
StructMetadata<::cpp2::RefUnion>::gen(ThriftMetadata& metadata) {
  auto res = metadata.structs()->emplace("module_no_uri.RefUnion", ::apache::thrift::metadata::ThriftStruct{});
  if (!res.second) {
    return res.first->second;
  }
  ::apache::thrift::metadata::ThriftStruct& module_no_uri_RefUnion = res.first->second;
  module_no_uri_RefUnion.name() = "module_no_uri.RefUnion";
  module_no_uri_RefUnion.is_union() = true;
  static const auto* const
  module_no_uri_RefUnion_fields = new std::array<EncodedThriftField, 1>{ {
    { 1, "field1", false, std::make_unique<Primitive>(ThriftPrimitiveType::THRIFT_STRING_TYPE), std::vector<ThriftConstStruct>{ *cvStruct("cpp.Adapter", { {"name", cvString("::my::Adapter1") } }).cv_struct_ref(), *cvStruct("cpp.Ref", { {"type", cvInteger(2) } }).cv_struct_ref(), }},  }};
  for (const auto& f : *module_no_uri_RefUnion_fields) {
    ::apache::thrift::metadata::ThriftField field;
    field.id() = f.id;
    field.name() = f.name;
    field.is_optional() = f.is_optional;
    f.metadata_type_interface->writeAndGenType(*field.type(), metadata);
    field.structured_annotations() = f.structured_annotations;
    module_no_uri_RefUnion.fields()->push_back(std::move(field));
  }
  return res.first->second;
}

} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache
