/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/terse_write/src/terse_write.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <vector>

#include <thrift/lib/cpp2/gen/module_metadata_h.h>
#include "thrift/compiler/test/fixtures/terse_write/gen-cpp2/terse_write_types.h"


namespace apache {
namespace thrift {
namespace detail {
namespace md {

template <>
class EnumMetadata<::facebook::thrift::test::terse_write::MyEnum> {
 public:
  static void gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::MyStruct> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::MyUnion> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::MyStructWithCustomDefault> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::StructLevelTerseStruct> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::FieldLevelTerseStruct> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::AdaptedFields> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class StructMetadata<::facebook::thrift::test::terse_write::TerseException> {
 public:
  static const ::apache::thrift::metadata::ThriftStruct& gen(ThriftMetadata& metadata);
};
template <>
class ExceptionMetadata<::facebook::thrift::test::terse_write::TerseException> {
 public:
  static void gen(ThriftMetadata& metadata);
};
} // namespace md
} // namespace detail
} // namespace thrift
} // namespace apache
