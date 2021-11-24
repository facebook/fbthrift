/**
 * Autogenerated by Thrift for src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
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

Foo::Foo(const Foo& srcObj) {
  __fbthrift_field_intField = srcObj.__fbthrift_field_intField;
  __isset.set(0,srcObj.__isset.get(0));
  __fbthrift_field_optionalIntField = srcObj.__fbthrift_field_optionalIntField;
  __isset.set(1,srcObj.__isset.get(1));
  __fbthrift_field_intFieldWithDefault = srcObj.__fbthrift_field_intFieldWithDefault;
  __isset.set(2,srcObj.__isset.get(2));
  __fbthrift_field_setField = srcObj.__fbthrift_field_setField;
  __isset.set(3,srcObj.__isset.get(3));
  __fbthrift_field_optionalSetField = srcObj.__fbthrift_field_optionalSetField;
  __isset.set(4,srcObj.__isset.get(4));
  __fbthrift_field_mapField = srcObj.__fbthrift_field_mapField;
  __isset.set(5,srcObj.__isset.get(5));
  __fbthrift_field_optionalMapField = srcObj.__fbthrift_field_optionalMapField;
  __isset.set(6,srcObj.__isset.get(6));
  __fbthrift_field_binaryField = srcObj.__fbthrift_field_binaryField;
  __isset.set(7,srcObj.__isset.get(7));
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_intField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalIntField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 3>(__fbthrift_field_intFieldWithDefault, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 4>(__fbthrift_field_setField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 5>(__fbthrift_field_optionalSetField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 6>(__fbthrift_field_mapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 7>(__fbthrift_field_optionalMapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 8>(__fbthrift_field_binaryField, *this);
}

Foo& Foo::operator=(const Foo& src) {
  Foo tmp(src);
  swap(*this, tmp);
  return *this;
}

Foo::Foo() :
      __fbthrift_field_intField(),
      __fbthrift_field_optionalIntField(),
      __fbthrift_field_intFieldWithDefault(::apache::thrift::adapt_detail::fromThriftField<my::Adapter1, 3>(static_cast<::std::int32_t>(13), *this)) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_intField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalIntField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 3>(__fbthrift_field_intFieldWithDefault, *this);
}


Foo::~Foo() {}

Foo::Foo(Foo&& other) noexcept  :
    __fbthrift_field_intField(std::move(other.__fbthrift_field_intField)),
    __fbthrift_field_optionalIntField(std::move(other.__fbthrift_field_optionalIntField)),
    __fbthrift_field_intFieldWithDefault(std::move(other.__fbthrift_field_intFieldWithDefault)),
    __fbthrift_field_setField(std::move(other.__fbthrift_field_setField)),
    __fbthrift_field_optionalSetField(std::move(other.__fbthrift_field_optionalSetField)),
    __fbthrift_field_mapField(std::move(other.__fbthrift_field_mapField)),
    __fbthrift_field_optionalMapField(std::move(other.__fbthrift_field_optionalMapField)),
    __fbthrift_field_binaryField(std::move(other.__fbthrift_field_binaryField)),
    __isset(other.__isset) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_intField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalIntField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 3>(__fbthrift_field_intFieldWithDefault, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 4>(__fbthrift_field_setField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 5>(__fbthrift_field_optionalSetField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 6>(__fbthrift_field_mapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 7>(__fbthrift_field_optionalMapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 8>(__fbthrift_field_binaryField, *this);
}

Foo& Foo::operator=(FOLLY_MAYBE_UNUSED Foo&& other) noexcept {
    this->__fbthrift_field_intField = std::move(other.__fbthrift_field_intField);
    this->__fbthrift_field_optionalIntField = std::move(other.__fbthrift_field_optionalIntField);
    this->__fbthrift_field_intFieldWithDefault = std::move(other.__fbthrift_field_intFieldWithDefault);
    this->__fbthrift_field_setField = std::move(other.__fbthrift_field_setField);
    this->__fbthrift_field_optionalSetField = std::move(other.__fbthrift_field_optionalSetField);
    this->__fbthrift_field_mapField = std::move(other.__fbthrift_field_mapField);
    this->__fbthrift_field_optionalMapField = std::move(other.__fbthrift_field_optionalMapField);
    this->__fbthrift_field_binaryField = std::move(other.__fbthrift_field_binaryField);
    __isset = other.__isset;
    return *this;
}


Foo::Foo(apache::thrift::FragileConstructor, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> intField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> optionalIntField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::int32_t> intFieldWithDefault__arg, ::cpp2::SetWithAdapter setField__arg, ::cpp2::SetWithAdapter optionalSetField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>> mapField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter3, ::std::map<::std::string, ::apache::thrift::adapt_detail::adapted_t<my::Adapter2, ::cpp2::ListWithElemAdapter>>> optionalMapField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::std::string> binaryField__arg) :
    __fbthrift_field_intField(std::move(intField__arg)),
    __fbthrift_field_optionalIntField(std::move(optionalIntField__arg)),
    __fbthrift_field_intFieldWithDefault(std::move(intFieldWithDefault__arg)),
    __fbthrift_field_setField(std::move(setField__arg)),
    __fbthrift_field_optionalSetField(std::move(optionalSetField__arg)),
    __fbthrift_field_mapField(std::move(mapField__arg)),
    __fbthrift_field_optionalMapField(std::move(optionalMapField__arg)),
    __fbthrift_field_binaryField(std::move(binaryField__arg)) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_intField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalIntField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 3>(__fbthrift_field_intFieldWithDefault, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 4>(__fbthrift_field_setField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter2, 5>(__fbthrift_field_optionalSetField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 6>(__fbthrift_field_mapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter3, 7>(__fbthrift_field_optionalMapField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 8>(__fbthrift_field_binaryField, *this);
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
  __isset.set(folly::index_constant<6>(), true);
  __isset.set(folly::index_constant<7>(), true);
}


void Foo::__clear() {
  // clear all fields
  this->__fbthrift_field_intField = decltype(this->__fbthrift_field_intField)();
  this->__fbthrift_field_optionalIntField = decltype(this->__fbthrift_field_optionalIntField)();
  this->__fbthrift_field_intFieldWithDefault = decltype(this->__fbthrift_field_intFieldWithDefault)();
  this->__fbthrift_field_setField.clear();
  this->__fbthrift_field_optionalSetField.clear();
  this->__fbthrift_field_mapField.clear();
  this->__fbthrift_field_optionalMapField.clear();
  this->__fbthrift_field_binaryField = decltype(this->__fbthrift_field_binaryField)();
  __isset = {};
}

bool Foo::operator==(const Foo& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_intField, rhs.__fbthrift_field_intField)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalIntField_ref(), rhs.optionalIntField_ref())) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_intFieldWithDefault, rhs.__fbthrift_field_intFieldWithDefault)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter2>(lhs.__fbthrift_field_setField, rhs.__fbthrift_field_setField)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter2>(lhs.optionalSetField_ref(), rhs.optionalSetField_ref())) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter3>(lhs.__fbthrift_field_mapField, rhs.__fbthrift_field_mapField)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter3>(lhs.optionalMapField_ref(), rhs.optionalMapField_ref())) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_binaryField, rhs.__fbthrift_field_binaryField)) {
    return false;
  }
  return true;
}

bool Foo::operator<(const Foo& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_intField, rhs.__fbthrift_field_intField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_intField, rhs.__fbthrift_field_intField);
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalIntField_ref(), rhs.optionalIntField_ref())) {
    return ::apache::thrift::adapt_detail::neq_less_opt<my::Adapter1>(lhs.optionalIntField_ref(), rhs.optionalIntField_ref());
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_intFieldWithDefault, rhs.__fbthrift_field_intFieldWithDefault)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_intFieldWithDefault, rhs.__fbthrift_field_intFieldWithDefault);
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter2>(lhs.__fbthrift_field_setField, rhs.__fbthrift_field_setField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter2>(lhs.__fbthrift_field_setField, rhs.__fbthrift_field_setField);
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter2>(lhs.optionalSetField_ref(), rhs.optionalSetField_ref())) {
    return ::apache::thrift::adapt_detail::neq_less_opt<my::Adapter2>(lhs.optionalSetField_ref(), rhs.optionalSetField_ref());
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter3>(lhs.__fbthrift_field_mapField, rhs.__fbthrift_field_mapField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter3>(lhs.__fbthrift_field_mapField, rhs.__fbthrift_field_mapField);
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter3>(lhs.optionalMapField_ref(), rhs.optionalMapField_ref())) {
    return ::apache::thrift::adapt_detail::neq_less_opt<my::Adapter3>(lhs.optionalMapField_ref(), rhs.optionalMapField_ref());
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_binaryField, rhs.__fbthrift_field_binaryField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_binaryField, rhs.__fbthrift_field_binaryField);
  }
  return false;
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
  swap(a.binaryField_ref().value(), b.binaryField_ref().value());
  swap(a.__isset, b.__isset);
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

void TccStructTraits<::cpp2::Baz>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Baz>;
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

constexpr std::size_t const TEnumTraits<::cpp2::Baz::Type>::size;
folly::Range<::cpp2::Baz::Type const*> const TEnumTraits<::cpp2::Baz::Type>::values = folly::range(TEnumDataStorage<::cpp2::Baz::Type>::values);
folly::Range<folly::StringPiece const*> const TEnumTraits<::cpp2::Baz::Type>::names = folly::range(TEnumDataStorage<::cpp2::Baz::Type>::names);

char const* TEnumTraits<::cpp2::Baz::Type>::findName(type value) {
  using factory = detail::TEnumMapFactory<::cpp2::Baz::Type>;
  static folly::Indestructible<factory::ValuesToNamesMapType> const map{
      factory::makeValuesToNamesMap()};
  auto found = map->find(value);
  return found == map->end() ? nullptr : found->second;
}

bool TEnumTraits<::cpp2::Baz::Type>::findValue(char const* name, type* out) {
  using factory = detail::TEnumMapFactory<::cpp2::Baz::Type>;
  static folly::Indestructible<factory::NamesToValuesMapType> const map{
      factory::makeNamesToValuesMap()};
  auto found = map->find(name);
  return found == map->end() ? false : (*out = found->second, true);
}
}} // apache::thrift
namespace cpp2 {

void Baz::__clear() {
  // clear all fields
  if (type_ == Type::__EMPTY__) { return; }
  switch(type_) {
    case Type::intField:
      destruct(value_.intField);
      break;
    case Type::setField:
      destruct(value_.setField);
      break;
    case Type::mapField:
      destruct(value_.mapField);
      break;
    case Type::binaryField:
      destruct(value_.binaryField);
      break;
    default:
      assert(false);
      break;
  }
  type_ = Type::__EMPTY__;
}

bool Baz::operator==(const Baz& rhs) const {
  if (type_ != rhs.type_) { return false; }
  switch(type_) {
    case Type::intField:
      return value_.intField == rhs.value_.intField;
    case Type::setField:
      return value_.setField == rhs.value_.setField;
    case Type::mapField:
      return value_.mapField == rhs.value_.mapField;
    case Type::binaryField:
      return value_.binaryField == rhs.value_.binaryField;
    default:
      return true;
  }
}

bool Baz::operator<(const Baz& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (lhs.type_ != rhs.type_) {
    return lhs.type_ < rhs.type_;
  }
  switch (lhs.type_) {
    case Type::intField:
      return lhs.value_.intField < rhs.value_.intField;
    case Type::setField:
      return lhs.value_.setField < rhs.value_.setField;
    case Type::mapField:
      return lhs.value_.mapField < rhs.value_.mapField;
    case Type::binaryField:
      return lhs.value_.binaryField < rhs.value_.binaryField;
    default:
      return false;
  }
}

void swap(Baz& a, Baz& b) {
  Baz temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void Baz::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Baz::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Baz::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Baz::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Baz::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Baz::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Baz::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Baz::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;



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

Bar::Bar(const Bar& srcObj) {
  __fbthrift_field_structField = srcObj.__fbthrift_field_structField;
  __isset.set(0,srcObj.__isset.get(0));
  __fbthrift_field_optionalStructField = srcObj.__fbthrift_field_optionalStructField;
  __isset.set(1,srcObj.__isset.get(1));
  __fbthrift_field_structListField = srcObj.__fbthrift_field_structListField;
  __isset.set(2,srcObj.__isset.get(2));
  __fbthrift_field_optionalStructListField = srcObj.__fbthrift_field_optionalStructListField;
  __isset.set(3,srcObj.__isset.get(3));
  __fbthrift_field_unionField = srcObj.__fbthrift_field_unionField;
  __isset.set(4,srcObj.__isset.get(4));
  __fbthrift_field_optionalUnionField = srcObj.__fbthrift_field_optionalUnionField;
  __isset.set(5,srcObj.__isset.get(5));
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_structField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalStructField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 5>(__fbthrift_field_unionField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 6>(__fbthrift_field_optionalUnionField, *this);
}

Bar& Bar::operator=(const Bar& src) {
  Bar tmp(src);
  swap(*this, tmp);
  return *this;
}

Bar::Bar() {
}


Bar::~Bar() {}

Bar::Bar(Bar&& other) noexcept  :
    __fbthrift_field_structField(std::move(other.__fbthrift_field_structField)),
    __fbthrift_field_optionalStructField(std::move(other.__fbthrift_field_optionalStructField)),
    __fbthrift_field_structListField(std::move(other.__fbthrift_field_structListField)),
    __fbthrift_field_optionalStructListField(std::move(other.__fbthrift_field_optionalStructListField)),
    __fbthrift_field_unionField(std::move(other.__fbthrift_field_unionField)),
    __fbthrift_field_optionalUnionField(std::move(other.__fbthrift_field_optionalUnionField)),
    __isset(other.__isset) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_structField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalStructField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 5>(__fbthrift_field_unionField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 6>(__fbthrift_field_optionalUnionField, *this);
}

Bar& Bar::operator=(FOLLY_MAYBE_UNUSED Bar&& other) noexcept {
    this->__fbthrift_field_structField = std::move(other.__fbthrift_field_structField);
    this->__fbthrift_field_optionalStructField = std::move(other.__fbthrift_field_optionalStructField);
    this->__fbthrift_field_structListField = std::move(other.__fbthrift_field_structListField);
    this->__fbthrift_field_optionalStructListField = std::move(other.__fbthrift_field_optionalStructListField);
    this->__fbthrift_field_unionField = std::move(other.__fbthrift_field_unionField);
    this->__fbthrift_field_optionalUnionField = std::move(other.__fbthrift_field_optionalUnionField);
    __isset = other.__isset;
    return *this;
}


Bar::Bar(apache::thrift::FragileConstructor, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo> structField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo> optionalStructField__arg, ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> structListField__arg, ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> optionalStructListField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz> unionField__arg, ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz> optionalUnionField__arg) :
    __fbthrift_field_structField(std::move(structField__arg)),
    __fbthrift_field_optionalStructField(std::move(optionalStructField__arg)),
    __fbthrift_field_structListField(std::move(structListField__arg)),
    __fbthrift_field_optionalStructListField(std::move(optionalStructListField__arg)),
    __fbthrift_field_unionField(std::move(unionField__arg)),
    __fbthrift_field_optionalUnionField(std::move(optionalUnionField__arg)) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_structField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 2>(__fbthrift_field_optionalStructField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 5>(__fbthrift_field_unionField, *this);
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 6>(__fbthrift_field_optionalUnionField, *this);
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
}


void Bar::__clear() {
  // clear all fields
  this->__fbthrift_field_structField = decltype(this->__fbthrift_field_structField)();
  this->__fbthrift_field_optionalStructField = decltype(this->__fbthrift_field_optionalStructField)();
  this->__fbthrift_field_structListField.clear();
  this->__fbthrift_field_optionalStructListField.clear();
  this->__fbthrift_field_unionField = decltype(this->__fbthrift_field_unionField)();
  this->__fbthrift_field_optionalUnionField = decltype(this->__fbthrift_field_optionalUnionField)();
  __isset = {};
}

bool Bar::operator==(const Bar& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_structField, rhs.__fbthrift_field_structField)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalStructField_ref(), rhs.optionalStructField_ref())) {
    return false;
  }
  if (!(lhs.structListField_ref() == rhs.structListField_ref())) {
    return false;
  }
  if (!(lhs.optionalStructListField_ref() == rhs.optionalStructListField_ref())) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_unionField, rhs.__fbthrift_field_unionField)) {
    return false;
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalUnionField_ref(), rhs.optionalUnionField_ref())) {
    return false;
  }
  return true;
}

bool Bar::operator<(const Bar& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_structField, rhs.__fbthrift_field_structField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_structField, rhs.__fbthrift_field_structField);
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalStructField_ref(), rhs.optionalStructField_ref())) {
    return ::apache::thrift::adapt_detail::neq_less_opt<my::Adapter1>(lhs.optionalStructField_ref(), rhs.optionalStructField_ref());
  }
  if (!(lhs.structListField_ref() == rhs.structListField_ref())) {
    return lhs.structListField_ref() < rhs.structListField_ref();
  }
  if (!(lhs.optionalStructListField_ref() == rhs.optionalStructListField_ref())) {
    return lhs.optionalStructListField_ref() < rhs.optionalStructListField_ref();
  }
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_unionField, rhs.__fbthrift_field_unionField)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_unionField, rhs.__fbthrift_field_unionField);
  }
  if (::apache::thrift::adapt_detail::not_equal_opt<my::Adapter1>(lhs.optionalUnionField_ref(), rhs.optionalUnionField_ref())) {
    return ::apache::thrift::adapt_detail::neq_less_opt<my::Adapter1>(lhs.optionalUnionField_ref(), rhs.optionalUnionField_ref());
  }
  return false;
}

const ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>& Bar::get_structListField() const& {
  return __fbthrift_field_structListField;
}

::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>> Bar::get_structListField() && {
  return std::move(__fbthrift_field_structListField);
}

const ::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>* Bar::get_optionalStructListField() const& {
  return optionalStructListField_ref().has_value() ? std::addressof(__fbthrift_field_optionalStructListField) : nullptr;
}

::std::vector<::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Foo>>* Bar::get_optionalStructListField() & {
  return optionalStructListField_ref().has_value() ? std::addressof(__fbthrift_field_optionalStructListField) : nullptr;
}


void swap(Bar& a, Bar& b) {
  using ::std::swap;
  swap(a.structField_ref().value(), b.structField_ref().value());
  swap(a.optionalStructField_ref().value_unchecked(), b.optionalStructField_ref().value_unchecked());
  swap(a.structListField_ref().value(), b.structListField_ref().value());
  swap(a.optionalStructListField_ref().value_unchecked(), b.optionalStructListField_ref().value_unchecked());
  swap(a.unionField_ref().value(), b.unionField_ref().value());
  swap(a.optionalUnionField_ref().value_unchecked(), b.optionalUnionField_ref().value_unchecked());
  swap(a.__isset, b.__isset);
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
    ::apache::thrift::detail::st::gen_check_json<
        Bar,
        ::apache::thrift::type_class::variant,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz>>,
    "inconsistent use of json option");
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        Bar,
        ::apache::thrift::type_class::variant,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz>>,
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
static_assert(
    ::apache::thrift::detail::st::gen_check_nimble<
        Bar,
        ::apache::thrift::type_class::variant,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz>>,
    "inconsistent use of nimble option");
static_assert(
    ::apache::thrift::detail::st::gen_check_nimble<
        Bar,
        ::apache::thrift::type_class::variant,
        ::apache::thrift::adapt_detail::adapted_t<my::Adapter1, ::cpp2::Baz>>,
    "inconsistent use of nimble option");

} // cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::StructWithFieldAdapter>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::StructWithFieldAdapter>;
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

StructWithFieldAdapter::StructWithFieldAdapter(const StructWithFieldAdapter& srcObj) {
  __fbthrift_field_field = srcObj.__fbthrift_field_field;
  __isset.set(0,srcObj.__isset.get(0));
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_field, *this);
}

StructWithFieldAdapter& StructWithFieldAdapter::operator=(const StructWithFieldAdapter& src) {
  StructWithFieldAdapter tmp(src);
  swap(*this, tmp);
  return *this;
}


StructWithFieldAdapter::StructWithFieldAdapter(apache::thrift::FragileConstructor, ::apache::thrift::adapt_detail::adapted_field_t<my::Adapter1, 1, ::std::int32_t, __fbthrift_cpp2_type> field__arg) :
    __fbthrift_field_field(std::move(field__arg)) {
  ::apache::thrift::adapt_detail::construct<my::Adapter1, 1>(__fbthrift_field_field, *this);
  __isset.set(folly::index_constant<0>(), true);
}


void StructWithFieldAdapter::__clear() {
  // clear all fields
  this->__fbthrift_field_field = decltype(this->__fbthrift_field_field)();
  __isset = {};
}

bool StructWithFieldAdapter::operator==(const StructWithFieldAdapter& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_field, rhs.__fbthrift_field_field)) {
    return false;
  }
  return true;
}

bool StructWithFieldAdapter::operator<(const StructWithFieldAdapter& rhs) const {
  (void)rhs;
  auto& lhs = *this;
  (void)lhs;
  if (::apache::thrift::adapt_detail::not_equal<my::Adapter1>(lhs.__fbthrift_field_field, rhs.__fbthrift_field_field)) {
    return ::apache::thrift::adapt_detail::less<my::Adapter1>(lhs.__fbthrift_field_field, rhs.__fbthrift_field_field);
  }
  return false;
}


void swap(StructWithFieldAdapter& a, StructWithFieldAdapter& b) {
  using ::std::swap;
  swap(a.field_ref().value(), b.field_ref().value());
  swap(a.__isset, b.__isset);
}

template void StructWithFieldAdapter::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t StructWithFieldAdapter::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t StructWithFieldAdapter::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t StructWithFieldAdapter::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void StructWithFieldAdapter::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t StructWithFieldAdapter::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t StructWithFieldAdapter::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t StructWithFieldAdapter::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;



} // cpp2

namespace cpp2 { namespace {
FOLLY_MAYBE_UNUSED FOLLY_ERASE void validateAdapters() {
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 1, ::std::int32_t, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 2, ::std::int32_t, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 3, ::std::int32_t, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter2, 4, ::std::set<::std::string>, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter2, 5, ::std::set<::std::string>, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter3, 6, ::std::map<::std::string, ::cpp2::ListWithElemAdapter>, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter3, 7, ::std::map<::std::string, ::cpp2::ListWithElemAdapter>, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 8, ::std::string, ::cpp2::Foo>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 1, ::std::int32_t, ::cpp2::Baz>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter2, 4, ::std::set<::std::string>, ::cpp2::Baz>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter3, 6, ::std::map<::std::string, ::cpp2::ListWithElemAdapter>, ::cpp2::Baz>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 8, ::std::string, ::cpp2::Baz>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 1, ::cpp2::Foo, ::cpp2::Bar>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 2, ::cpp2::Foo, ::cpp2::Bar>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 5, ::cpp2::Baz, ::cpp2::Bar>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 6, ::cpp2::Baz, ::cpp2::Bar>();
  ::apache::thrift::adapt_detail::validateFieldAdapter<my::Adapter1, 1, ::std::int32_t, ::cpp2::StructWithFieldAdapter>();
  ::apache::thrift::adapt_detail::validateAdapter<my::Adapter2, ::std::set<::std::string>>();
  ::apache::thrift::adapt_detail::validateAdapter<my::Adapter2, ::cpp2::Bar>();
  ::apache::thrift::adapt_detail::validateAdapter<my::Adapter2, ::cpp2::Baz>();
}
}} // cpp2
