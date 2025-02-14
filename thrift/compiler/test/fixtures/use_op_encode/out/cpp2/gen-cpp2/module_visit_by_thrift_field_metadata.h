/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/use_op_encode/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/visitation/visit_by_thrift_field_metadata.h>
#include "thrift/compiler/test/fixtures/use_op_encode/gen-cpp2/module_metadata.h"

namespace apache {
namespace thrift {
namespace detail {

template <>
struct VisitByFieldId<::facebook::thrift::compiler::test::MyStruct> {
  template <typename F, typename T>
  void operator()([[maybe_unused]] F&& f, int32_t fieldId, [[maybe_unused]] T&& t) const {
    switch (fieldId) {
    case 1:
      return f(0, static_cast<T&&>(t).def_field_ref());
    case 2:
      return f(1, static_cast<T&&>(t).opt_field_ref());
    case 3:
      return f(2, static_cast<T&&>(t).req_field_ref());
    case 4:
      return f(3, static_cast<T&&>(t).terse_field_ref());
    default:
      throwInvalidThriftId(fieldId, "::facebook::thrift::compiler::test::MyStruct");
    }
  }
};
} // namespace detail
} // namespace thrift
} // namespace apache
