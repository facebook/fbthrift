/**
 * Autogenerated by Thrift for src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#include "thrift/compiler/test/fixtures/adapter/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/adapter/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/adapter/gen-cpp2/module_data.h"


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Foo>::translateFieldName(
    folly::StringPiece _fname,
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

THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
Foo::Foo() :
      intField(my::Adapter1::fromThrift(0)),
      optionalIntField(my::Adapter1::fromThrift(0)),
      intFieldWithDefault(my::Adapter1::fromThrift(13)) {}

THRIFT_IGNORE_ISSET_USE_WARNING_END

Foo::~Foo() {}

THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
Foo::Foo(Foo&& other) noexcept  :
    intField(std::move(other.intField)),
    optionalIntField(std::move(other.optionalIntField)),
    intFieldWithDefault(std::move(other.intFieldWithDefault)),
    setField(std::move(other.setField)),
    optionalSetField(std::move(other.optionalSetField)),
    mapField(std::move(other.mapField)),
    optionalMapField(std::move(other.optionalMapField)),
    __isset(other.__isset) {}
THRIFT_IGNORE_ISSET_USE_WARNING_END


THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
Foo::Foo(apache::thrift::FragileConstructor, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> intField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> optionalIntField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> intFieldWithDefault__arg, ::cpp2::SetWithAdapter setField__arg, ::cpp2::SetWithAdapter optionalSetField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>> mapField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>> optionalMapField__arg) :
    intField(std::move(intField__arg)),
    optionalIntField(std::move(optionalIntField__arg)),
    intFieldWithDefault(std::move(intFieldWithDefault__arg)),
    setField(std::move(setField__arg)),
    optionalSetField(std::move(optionalSetField__arg)),
    mapField(std::move(mapField__arg)),
    optionalMapField(std::move(optionalMapField__arg)) {
  __isset.intField = true;
  __isset.optionalIntField = true;
  __isset.intFieldWithDefault = true;
  __isset.setField = true;
  __isset.optionalSetField = true;
  __isset.mapField = true;
  __isset.optionalMapField = true;
}
THRIFT_IGNORE_ISSET_USE_WARNING_END
void Foo::__clear() {
  // clear all fields
  this->intField = my::Adapter1::fromThrift(0);
  this->optionalIntField = my::Adapter1::fromThrift(0);
  this->intFieldWithDefault = my::Adapter1::fromThrift(13);
  this->setField.clear();
  this->optionalSetField.clear();
  this->mapField.clear();
  this->optionalMapField.clear();
THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
  __isset = {};
THRIFT_IGNORE_ISSET_USE_WARNING_END
}

bool Foo::operator==(const Foo& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (!(lhs.intField == rhs.intField)) {
    return false;
  }
  if (lhs.optionalIntField_ref() != rhs.optionalIntField_ref()) {
    return false;
  }
  if (!(lhs.intFieldWithDefault == rhs.intFieldWithDefault)) {
    return false;
  }
  if (!(lhs.setField == rhs.setField)) {
    return false;
  }
  if (lhs.optionalSetField_ref() != rhs.optionalSetField_ref()) {
    return false;
  }
  if (!(lhs.mapField == rhs.mapField)) {
    return false;
  }
  if (lhs.optionalMapField_ref() != rhs.optionalMapField_ref()) {
    return false;
  }
  return true;
}

bool Foo::operator<(const Foo& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (!(lhs.intField == rhs.intField)) {
    return lhs.intField < rhs.intField;
  }
  if (lhs.optionalIntField_ref() != rhs.optionalIntField_ref()) {
    return lhs.optionalIntField_ref() < rhs.optionalIntField_ref();
  }
  if (!(lhs.intFieldWithDefault == rhs.intFieldWithDefault)) {
    return lhs.intFieldWithDefault < rhs.intFieldWithDefault;
  }
  if (!(lhs.setField == rhs.setField)) {
    return lhs.setField < rhs.setField;
  }
  if (lhs.optionalSetField_ref() != rhs.optionalSetField_ref()) {
    return lhs.optionalSetField_ref() < rhs.optionalSetField_ref();
  }
  if (!(lhs.mapField == rhs.mapField)) {
    return lhs.mapField < rhs.mapField;
  }
  if (lhs.optionalMapField_ref() != rhs.optionalMapField_ref()) {
    return lhs.optionalMapField_ref() < rhs.optionalMapField_ref();
  }
  return false;
}

const ::cpp2::SetWithAdapter& Foo::get_setField() const& {
  return setField;
}

::cpp2::SetWithAdapter Foo::get_setField() && {
  return std::move(setField);
}

const ::cpp2::SetWithAdapter* Foo::get_optionalSetField() const& {
  return optionalSetField_ref().has_value() ? std::addressof(optionalSetField) : nullptr;
}

::cpp2::SetWithAdapter* Foo::get_optionalSetField() & {
  return optionalSetField_ref().has_value() ? std::addressof(optionalSetField) : nullptr;
}

const ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>>& Foo::get_mapField() const& {
  return mapField;
}

::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>> Foo::get_mapField() && {
  return std::move(mapField);
}

const ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>>* Foo::get_optionalMapField() const& {
  return optionalMapField_ref().has_value() ? std::addressof(optionalMapField) : nullptr;
}

::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>>* Foo::get_optionalMapField() & {
  return optionalMapField_ref().has_value() ? std::addressof(optionalMapField) : nullptr;
}


void swap(Foo& a, Foo& b) {
  using ::std::swap;
  swap(a.intField_ref().value(), b.intField_ref().value());
  swap(a.optionalIntField_ref().value_unchecked(), b.optionalIntField_ref().value_unchecked());
  swap(a.intFieldWithDefault_ref().value(), b.intFieldWithDefault_ref().value());
  swap(a.setField_ref().value(), b.setField_ref().value());
  swap(a.optionalSetField_ref().value_unchecked(), b.optionalSetField_ref().value_unchecked());
  swap(a.mapField_ref().value(), b.mapField_ref().value());
  swap(a.optionalMapField_ref().value_unchecked(), b.optionalMapField_ref().value_unchecked());
THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
  swap(a.__isset, b.__isset);
THRIFT_IGNORE_ISSET_USE_WARNING_END
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

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Bar>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Bar>;
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

THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
Bar::Bar(Bar&& other) noexcept  :
    structField(std::move(other.structField)),
    optionalStructField(std::move(other.optionalStructField)),
    structListField(std::move(other.structListField)),
    optionalStructListField(std::move(other.optionalStructListField)),
    __isset(other.__isset) {}
THRIFT_IGNORE_ISSET_USE_WARNING_END


THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
Bar::Bar(apache::thrift::FragileConstructor, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo> structField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo> optionalStructField__arg, ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> structListField__arg, ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> optionalStructListField__arg) :
    structField(std::move(structField__arg)),
    optionalStructField(std::move(optionalStructField__arg)),
    structListField(std::move(structListField__arg)),
    optionalStructListField(std::move(optionalStructListField__arg)) {
  __isset.structField = true;
  __isset.optionalStructField = true;
  __isset.structListField = true;
  __isset.optionalStructListField = true;
}
THRIFT_IGNORE_ISSET_USE_WARNING_END
void Bar::__clear() {
  // clear all fields
  this->structField.__clear();
  this->optionalStructField.__clear();
  this->structListField.clear();
  this->optionalStructListField.clear();
THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
  __isset = {};
THRIFT_IGNORE_ISSET_USE_WARNING_END
}

bool Bar::operator==(const Bar& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (!(lhs.structField == rhs.structField)) {
    return false;
  }
  if (lhs.optionalStructField_ref() != rhs.optionalStructField_ref()) {
    return false;
  }
  if (!(lhs.structListField == rhs.structListField)) {
    return false;
  }
  if (lhs.optionalStructListField_ref() != rhs.optionalStructListField_ref()) {
    return false;
  }
  return true;
}

bool Bar::operator<(const Bar& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (!(lhs.structField == rhs.structField)) {
    return lhs.structField < rhs.structField;
  }
  if (lhs.optionalStructField_ref() != rhs.optionalStructField_ref()) {
    return lhs.optionalStructField_ref() < rhs.optionalStructField_ref();
  }
  if (!(lhs.structListField == rhs.structListField)) {
    return lhs.structListField < rhs.structListField;
  }
  if (lhs.optionalStructListField_ref() != rhs.optionalStructListField_ref()) {
    return lhs.optionalStructListField_ref() < rhs.optionalStructListField_ref();
  }
  return false;
}

const ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>& Bar::get_structField() const& {
  return structField;
}

::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo> Bar::get_structField() && {
  return std::move(structField);
}

const ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>* Bar::get_optionalStructField() const& {
  return optionalStructField_ref().has_value() ? std::addressof(optionalStructField) : nullptr;
}

::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>* Bar::get_optionalStructField() & {
  return optionalStructField_ref().has_value() ? std::addressof(optionalStructField) : nullptr;
}

const ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>& Bar::get_structListField() const& {
  return structListField;
}

::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> Bar::get_structListField() && {
  return std::move(structListField);
}

const ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>* Bar::get_optionalStructListField() const& {
  return optionalStructListField_ref().has_value() ? std::addressof(optionalStructListField) : nullptr;
}

::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>* Bar::get_optionalStructListField() & {
  return optionalStructListField_ref().has_value() ? std::addressof(optionalStructListField) : nullptr;
}


void swap(Bar& a, Bar& b) {
  using ::std::swap;
  swap(a.structField_ref().value(), b.structField_ref().value());
  swap(a.optionalStructField_ref().value_unchecked(), b.optionalStructField_ref().value_unchecked());
  swap(a.structListField_ref().value(), b.structListField_ref().value());
  swap(a.optionalStructListField_ref().value_unchecked(), b.optionalStructListField_ref().value_unchecked());
THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
  swap(a.__isset, b.__isset);
THRIFT_IGNORE_ISSET_USE_WARNING_END
}

template void Bar::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Bar::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Bar::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Bar::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Bar::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Bar::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Bar::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Bar::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        Bar,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>,
    "inconsistent use of json option");
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        Bar,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>,
    "inconsistent use of json option");

static_assert(
    ::apache::thrift::detail::st::gen_check_nimble<
        Bar,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>,
    "inconsistent use of nimble option");
static_assert(
    ::apache::thrift::detail::st::gen_check_nimble<
        Bar,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>,
    "inconsistent use of nimble option");

} // cpp2
