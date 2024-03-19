/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/deprecated-public-required-fields/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/deprecated-public-required-fields/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/deprecated-public-required-fields/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/deprecated-public-required-fields/gen-cpp2/module_data.h"


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Foo>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Foo>;
  static const st::translate_field_name_table table{
      data::fields_size,
      data::fields_names.data(),
      data::fields_ids.data(),
      data::fields_types.data()};
  st::translate_field_name(_fname, fid, _ftype, table);
}

} // namespace detail
} // namespace thrift
} // namespace apache

namespace cpp2 {

std::string_view Foo::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Foo>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view Foo::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Foo>::name;
}


Foo::Foo(apache::thrift::FragileConstructor, ::std::int32_t field__arg) :
    field(std::move(field__arg)) {
}


void Foo::__fbthrift_clear() {
  // clear all fields
  this->field = ::std::int32_t();
}

void Foo::__fbthrift_clear_terse_fields() {
}

bool Foo::__fbthrift_is_empty() const {
  return false;
}

bool Foo::operator==([[maybe_unused]] const Foo& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool Foo::operator<([[maybe_unused]] const Foo& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] Foo& a, [[maybe_unused]] Foo& b) {
  using ::std::swap;
  swap(a.field, b.field);
}

template void Foo::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Foo::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Foo::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Foo::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Foo::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Foo::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Foo::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Foo::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // cpp2
namespace apache::thrift::detail::annotation {
}
