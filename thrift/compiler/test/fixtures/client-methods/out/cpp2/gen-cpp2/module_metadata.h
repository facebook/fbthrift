/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/client-methods/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <vector>

#include <thrift/lib/cpp2/gen/module_metadata_h.h>
#include "thrift/compiler/test/fixtures/client-methods/gen-cpp2/module_types.h"

namespace cpp2 {
class HeaderClientMethodsAnnotationOnService;
} // namespace cpp2
namespace cpp2 {
class HeaderClientMethodsAnnotationOnFunction;
} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {
namespace md {

template <>
class StructMetadata<::cpp2::EchoRequest> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::cpp2::EchoResponse> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class ServiceMetadata<::apache::thrift::ServiceHandler<::cpp2::HeaderClientMethodsAnnotationOnService>> {
 public:
  static void gen(ThriftServiceMetadataResponse& response);
 private:
  static const ThriftServiceContextRef* genRecurse(ThriftMetadata& metadata, std::vector<ThriftServiceContextRef>& services);

  template <typename T>
  friend class ServiceMetadata;

  static void gen_echo(ThriftMetadata& metadata, ThriftService& context);
  static void gen_echo_2(ThriftMetadata& metadata, ThriftService& context);
};
template <>
class ServiceMetadata<::apache::thrift::ServiceHandler<::cpp2::HeaderClientMethodsAnnotationOnFunction>> {
 public:
  static void gen(ThriftServiceMetadataResponse& response);
 private:
  static const ThriftServiceContextRef* genRecurse(ThriftMetadata& metadata, std::vector<ThriftServiceContextRef>& services);

  template <typename T>
  friend class ServiceMetadata;

  static void gen_echo(ThriftMetadata& metadata, ThriftService& context);
  static void gen_echo_2(ThriftMetadata& metadata, ThriftService& context);
};
} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache
