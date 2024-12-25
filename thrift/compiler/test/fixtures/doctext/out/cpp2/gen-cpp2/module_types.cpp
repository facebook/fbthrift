/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/doctext/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/doctext/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/doctext/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/doctext/gen-cpp2/module_data.h"
[[maybe_unused]] static constexpr std::string_view kModuleName = "module";


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::cpp2::B>::__fbthrift_module_name_internal_do_not_use = kModuleName;
folly::Range<::cpp2::B const*> const TEnumTraits<::cpp2::B>::values = folly::range(TEnumDataStorage<::cpp2::B>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::B>::names = folly::range(TEnumDataStorage<::cpp2::B>::names);

bool TEnumTraits<::cpp2::B>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::B>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::A>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::A>;
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

std::string_view A::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<A>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view A::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<A>::name;
}


A::A(apache::thrift::FragileConstructor, ::std::int32_t useless_field__arg) :
    __fbthrift_field_useless_field(std::move(useless_field__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void A::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_useless_field = ::std::int32_t();
  __isset = {};
}

void A::__fbthrift_clear_terse_fields() {
}

bool A::__fbthrift_is_empty() const {
  return false;
}

bool A::operator==([[maybe_unused]] const A& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool A::operator<([[maybe_unused]] const A& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


::std::int32_t A::get_useless_field() const {
  return __fbthrift_field_useless_field;
}

::std::int32_t& A::set_useless_field(::std::int32_t useless_field_) {
  useless_field_ref() = useless_field_;
  return __fbthrift_field_useless_field;
}

void swap([[maybe_unused]] A& a, [[maybe_unused]] A& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_useless_field, b.__fbthrift_field_useless_field);
  swap(a.__isset, b.__isset);
}

template void A::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t A::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t A::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t A::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void A::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t A::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t A::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t A::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::U>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::U>;
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

namespace apache { namespace thrift {

folly::Range<::cpp2::U::Type const*> const TEnumTraits<::cpp2::U::Type>::values = folly::range(TEnumDataStorage<::cpp2::U::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::U::Type>::names = folly::range(TEnumDataStorage<::cpp2::U::Type>::names);

bool TEnumTraits<::cpp2::U::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::U::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view U::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<U>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view U::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<U>::name;
}

void U::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::i:
      ::std::destroy_at(::std::addressof(value_.i));
      break;
    case Type::s:
      ::std::destroy_at(::std::addressof(value_.s));
      break;
    default:
      assert(false);
      break;
  }
}

void U::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  U::~U() {
    __fbthrift_destruct();
  }

bool U::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  U::U(const U& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::s:
        set_s(rhs.value_.s);
        break;
      default:
        assert(false);
    }
  }

    U&U::operator=(const U& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::s:
        set_s(rhs.value_.s);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool U::operator==(const U& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool U::operator<([[maybe_unused]] const U& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(U& a, U& b) {
  U temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void U::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t U::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t U::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t U::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void U::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t U::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t U::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t U::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Bang>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Bang>;
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

std::string_view Bang::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Bang>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view Bang::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Bang>::name;
}

Bang::Bang(const Bang&) = default;
Bang& Bang::operator=(const Bang&) = default;
Bang::Bang() {
}


Bang::~Bang() {}

Bang::Bang([[maybe_unused]] Bang&& other) noexcept :
    __fbthrift_field_message(std::move(other.__fbthrift_field_message)),
    __isset(other.__isset) {
}

Bang& Bang::operator=([[maybe_unused]] Bang&& other) noexcept {
    this->__fbthrift_field_message = std::move(other.__fbthrift_field_message);
    __isset = other.__isset;
    return *this;
}


Bang::Bang(apache::thrift::FragileConstructor, ::std::string message__arg) :
    __fbthrift_field_message(std::move(message__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void Bang::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_message = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void Bang::__fbthrift_clear_terse_fields() {
}

bool Bang::__fbthrift_is_empty() const {
  return false;
}

bool Bang::operator==([[maybe_unused]] const Bang& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool Bang::operator<([[maybe_unused]] const Bang& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


const ::std::string& Bang::get_message() const& {
  return __fbthrift_field_message;
}

::std::string Bang::get_message() && {
  return static_cast<::std::string&&>(__fbthrift_field_message);
}

void swap([[maybe_unused]] Bang& a, [[maybe_unused]] Bang& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_message, b.__fbthrift_field_message);
  swap(a.__isset, b.__isset);
}

template void Bang::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Bang::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Bang::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Bang::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Bang::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Bang::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Bang::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Bang::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace cpp2
namespace apache::thrift::detail::annotation {
}
