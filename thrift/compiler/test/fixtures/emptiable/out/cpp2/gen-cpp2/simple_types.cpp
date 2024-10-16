/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/emptiable/src/simple.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/emptiable/gen-cpp2/simple_types.h"
#include "thrift/compiler/test/fixtures/emptiable/gen-cpp2/simple_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/emptiable/gen-cpp2/simple_data.h"


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::apache::thrift::test::MyEnum>::type_name = TEnumDataStorage<::apache::thrift::test::MyEnum>::type_name;
folly::Range<::apache::thrift::test::MyEnum const*> const TEnumTraits<::apache::thrift::test::MyEnum>::values = folly::range(TEnumDataStorage<::apache::thrift::test::MyEnum>::values);
folly::Range<std::string_view const*> const TEnumTraits<::apache::thrift::test::MyEnum>::names = folly::range(TEnumDataStorage<::apache::thrift::test::MyEnum>::names);

bool TEnumTraits<::apache::thrift::test::MyEnum>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::apache::thrift::test::MyEnum>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::apache::thrift::test::MyStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::apache::thrift::test::MyStruct>;
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

namespace apache::thrift::test {

std::string_view MyStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<MyStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view MyStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<MyStruct>::name;
}


MyStruct::MyStruct(apache::thrift::FragileConstructor) {}


void MyStruct::__fbthrift_clear() {
  // clear all fields
}

void MyStruct::__fbthrift_clear_terse_fields() {
}

bool MyStruct::__fbthrift_is_empty() const {
  return true;
}

bool MyStruct::operator==([[maybe_unused]] const MyStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool MyStruct::operator<([[maybe_unused]] const MyStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] MyStruct& a, [[maybe_unused]] MyStruct& b) {
  using ::std::swap;
}

template void MyStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t MyStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t MyStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t MyStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void MyStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t MyStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t MyStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t MyStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace apache::thrift::test

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::apache::thrift::test::EmptiableStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::apache::thrift::test::EmptiableStruct>;
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

namespace apache::thrift::test {

std::string_view EmptiableStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<EmptiableStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view EmptiableStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<EmptiableStruct>::name;
}

EmptiableStruct::EmptiableStruct(const EmptiableStruct&) = default;
EmptiableStruct& EmptiableStruct::operator=(const EmptiableStruct&) = default;
EmptiableStruct::EmptiableStruct() :
    __fbthrift_field_bool_field(),
    __fbthrift_field_byte_field(),
    __fbthrift_field_short_field(),
    __fbthrift_field_int_field(),
    __fbthrift_field_long_field(),
    __fbthrift_field_float_field(),
    __fbthrift_field_double_field(),
    __fbthrift_field_enum_field() {
}


EmptiableStruct::~EmptiableStruct() {}

EmptiableStruct::EmptiableStruct([[maybe_unused]] EmptiableStruct&& other) noexcept :
    __fbthrift_field_bool_field(std::move(other.__fbthrift_field_bool_field)),
    __fbthrift_field_byte_field(std::move(other.__fbthrift_field_byte_field)),
    __fbthrift_field_short_field(std::move(other.__fbthrift_field_short_field)),
    __fbthrift_field_int_field(std::move(other.__fbthrift_field_int_field)),
    __fbthrift_field_long_field(std::move(other.__fbthrift_field_long_field)),
    __fbthrift_field_float_field(std::move(other.__fbthrift_field_float_field)),
    __fbthrift_field_double_field(std::move(other.__fbthrift_field_double_field)),
    __fbthrift_field_string_field(std::move(other.__fbthrift_field_string_field)),
    __fbthrift_field_binary_field(std::move(other.__fbthrift_field_binary_field)),
    __fbthrift_field_enum_field(std::move(other.__fbthrift_field_enum_field)),
    __fbthrift_field_list_field(std::move(other.__fbthrift_field_list_field)),
    __fbthrift_field_set_field(std::move(other.__fbthrift_field_set_field)),
    __fbthrift_field_map_field(std::move(other.__fbthrift_field_map_field)),
    __fbthrift_field_struct_field(std::move(other.__fbthrift_field_struct_field)),
    __isset(other.__isset) {
}

EmptiableStruct& EmptiableStruct::operator=([[maybe_unused]] EmptiableStruct&& other) noexcept {
    this->__fbthrift_field_bool_field = std::move(other.__fbthrift_field_bool_field);
    this->__fbthrift_field_byte_field = std::move(other.__fbthrift_field_byte_field);
    this->__fbthrift_field_short_field = std::move(other.__fbthrift_field_short_field);
    this->__fbthrift_field_int_field = std::move(other.__fbthrift_field_int_field);
    this->__fbthrift_field_long_field = std::move(other.__fbthrift_field_long_field);
    this->__fbthrift_field_float_field = std::move(other.__fbthrift_field_float_field);
    this->__fbthrift_field_double_field = std::move(other.__fbthrift_field_double_field);
    this->__fbthrift_field_string_field = std::move(other.__fbthrift_field_string_field);
    this->__fbthrift_field_binary_field = std::move(other.__fbthrift_field_binary_field);
    this->__fbthrift_field_enum_field = std::move(other.__fbthrift_field_enum_field);
    this->__fbthrift_field_list_field = std::move(other.__fbthrift_field_list_field);
    this->__fbthrift_field_set_field = std::move(other.__fbthrift_field_set_field);
    this->__fbthrift_field_map_field = std::move(other.__fbthrift_field_map_field);
    this->__fbthrift_field_struct_field = std::move(other.__fbthrift_field_struct_field);
    __isset = other.__isset;
    return *this;
}


EmptiableStruct::EmptiableStruct(apache::thrift::FragileConstructor, bool bool_field__arg, ::std::int8_t byte_field__arg, ::std::int16_t short_field__arg, ::std::int32_t int_field__arg, ::std::int64_t long_field__arg, float float_field__arg, double double_field__arg, ::std::string string_field__arg, ::std::string binary_field__arg, ::apache::thrift::test::MyEnum enum_field__arg, ::std::vector<::std::int16_t> list_field__arg, ::std::set<::std::int16_t> set_field__arg, ::std::map<::std::int16_t, ::std::int16_t> map_field__arg, ::apache::thrift::test::MyStruct struct_field__arg) :
    __fbthrift_field_bool_field(std::move(bool_field__arg)),
    __fbthrift_field_byte_field(std::move(byte_field__arg)),
    __fbthrift_field_short_field(std::move(short_field__arg)),
    __fbthrift_field_int_field(std::move(int_field__arg)),
    __fbthrift_field_long_field(std::move(long_field__arg)),
    __fbthrift_field_float_field(std::move(float_field__arg)),
    __fbthrift_field_double_field(std::move(double_field__arg)),
    __fbthrift_field_string_field(std::move(string_field__arg)),
    __fbthrift_field_binary_field(std::move(binary_field__arg)),
    __fbthrift_field_enum_field(std::move(enum_field__arg)),
    __fbthrift_field_list_field(std::move(list_field__arg)),
    __fbthrift_field_set_field(std::move(set_field__arg)),
    __fbthrift_field_map_field(std::move(map_field__arg)),
    __fbthrift_field_struct_field(std::move(struct_field__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
  __isset.set(folly::index_constant<6>(), true);
  __isset.set(folly::index_constant<7>(), true);
  __isset.set(folly::index_constant<8>(), true);
  __isset.set(folly::index_constant<9>(), true);
  __isset.set(folly::index_constant<10>(), true);
  __isset.set(folly::index_constant<11>(), true);
  __isset.set(folly::index_constant<12>(), true);
  __isset.set(folly::index_constant<13>(), true);
}


void EmptiableStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_bool_field = bool();
  this->__fbthrift_field_byte_field = ::std::int8_t();
  this->__fbthrift_field_short_field = ::std::int16_t();
  this->__fbthrift_field_int_field = ::std::int32_t();
  this->__fbthrift_field_long_field = ::std::int64_t();
  this->__fbthrift_field_float_field = float();
  this->__fbthrift_field_double_field = double();
  this->__fbthrift_field_string_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_binary_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_enum_field = ::apache::thrift::test::MyEnum();
  this->__fbthrift_field_list_field.clear();
  this->__fbthrift_field_set_field.clear();
  this->__fbthrift_field_map_field.clear();
  __isset = {};
}

void EmptiableStruct::__fbthrift_clear_terse_fields() {
}

bool EmptiableStruct::__fbthrift_is_empty() const {
  return !(this->__isset.get(0)) &&
 !(this->__isset.get(1)) &&
 !(this->__isset.get(2)) &&
 !(this->__isset.get(3)) &&
 !(this->__isset.get(4)) &&
 !(this->__isset.get(5)) &&
 !(this->__isset.get(6)) &&
 !(this->__isset.get(7)) &&
 !(this->__isset.get(8)) &&
 !(this->__isset.get(9)) &&
 !(this->__isset.get(10)) &&
 !(this->__isset.get(11)) &&
 !(this->__isset.get(12)) &&
 !(this->__isset.get(13));
}

bool EmptiableStruct::operator==([[maybe_unused]] const EmptiableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool EmptiableStruct::operator<([[maybe_unused]] const EmptiableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}

const ::std::vector<::std::int16_t>* EmptiableStruct::get_list_field() const& {
  return list_field_ref().has_value() ? std::addressof(__fbthrift_field_list_field) : nullptr;
}

::std::vector<::std::int16_t>* EmptiableStruct::get_list_field() & {
  return list_field_ref().has_value() ? std::addressof(__fbthrift_field_list_field) : nullptr;
}

const ::std::set<::std::int16_t>* EmptiableStruct::get_set_field() const& {
  return set_field_ref().has_value() ? std::addressof(__fbthrift_field_set_field) : nullptr;
}

::std::set<::std::int16_t>* EmptiableStruct::get_set_field() & {
  return set_field_ref().has_value() ? std::addressof(__fbthrift_field_set_field) : nullptr;
}

const ::std::map<::std::int16_t, ::std::int16_t>* EmptiableStruct::get_map_field() const& {
  return map_field_ref().has_value() ? std::addressof(__fbthrift_field_map_field) : nullptr;
}

::std::map<::std::int16_t, ::std::int16_t>* EmptiableStruct::get_map_field() & {
  return map_field_ref().has_value() ? std::addressof(__fbthrift_field_map_field) : nullptr;
}

const ::apache::thrift::test::MyStruct* EmptiableStruct::get_struct_field() const& {
  return struct_field_ref().has_value() ? std::addressof(__fbthrift_field_struct_field) : nullptr;
}

::apache::thrift::test::MyStruct* EmptiableStruct::get_struct_field() & {
  return struct_field_ref().has_value() ? std::addressof(__fbthrift_field_struct_field) : nullptr;
}


void swap([[maybe_unused]] EmptiableStruct& a, [[maybe_unused]] EmptiableStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_bool_field, b.__fbthrift_field_bool_field);
  swap(a.__fbthrift_field_byte_field, b.__fbthrift_field_byte_field);
  swap(a.__fbthrift_field_short_field, b.__fbthrift_field_short_field);
  swap(a.__fbthrift_field_int_field, b.__fbthrift_field_int_field);
  swap(a.__fbthrift_field_long_field, b.__fbthrift_field_long_field);
  swap(a.__fbthrift_field_float_field, b.__fbthrift_field_float_field);
  swap(a.__fbthrift_field_double_field, b.__fbthrift_field_double_field);
  swap(a.__fbthrift_field_string_field, b.__fbthrift_field_string_field);
  swap(a.__fbthrift_field_binary_field, b.__fbthrift_field_binary_field);
  swap(a.__fbthrift_field_enum_field, b.__fbthrift_field_enum_field);
  swap(a.__fbthrift_field_list_field, b.__fbthrift_field_list_field);
  swap(a.__fbthrift_field_set_field, b.__fbthrift_field_set_field);
  swap(a.__fbthrift_field_map_field, b.__fbthrift_field_map_field);
  swap(a.__fbthrift_field_struct_field, b.__fbthrift_field_struct_field);
  swap(a.__isset, b.__isset);
}

template void EmptiableStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EmptiableStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EmptiableStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EmptiableStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void EmptiableStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t EmptiableStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t EmptiableStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t EmptiableStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        EmptiableStruct,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::test::MyStruct>,
    "inconsistent use of json option");

} // namespace apache::thrift::test

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::apache::thrift::test::EmptiableTerseStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::apache::thrift::test::EmptiableTerseStruct>;
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

namespace apache::thrift::test {

std::string_view EmptiableTerseStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<EmptiableTerseStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view EmptiableTerseStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<EmptiableTerseStruct>::name;
}

EmptiableTerseStruct::EmptiableTerseStruct(const EmptiableTerseStruct&) = default;
EmptiableTerseStruct& EmptiableTerseStruct::operator=(const EmptiableTerseStruct&) = default;
EmptiableTerseStruct::EmptiableTerseStruct() :
    __fbthrift_field_bool_field(),
    __fbthrift_field_byte_field(),
    __fbthrift_field_short_field(),
    __fbthrift_field_int_field(),
    __fbthrift_field_long_field(),
    __fbthrift_field_float_field(),
    __fbthrift_field_double_field(),
    __fbthrift_field_enum_field() {
}


EmptiableTerseStruct::~EmptiableTerseStruct() {}

EmptiableTerseStruct::EmptiableTerseStruct([[maybe_unused]] EmptiableTerseStruct&& other) noexcept :
    __fbthrift_field_bool_field(std::move(other.__fbthrift_field_bool_field)),
    __fbthrift_field_byte_field(std::move(other.__fbthrift_field_byte_field)),
    __fbthrift_field_short_field(std::move(other.__fbthrift_field_short_field)),
    __fbthrift_field_int_field(std::move(other.__fbthrift_field_int_field)),
    __fbthrift_field_long_field(std::move(other.__fbthrift_field_long_field)),
    __fbthrift_field_float_field(std::move(other.__fbthrift_field_float_field)),
    __fbthrift_field_double_field(std::move(other.__fbthrift_field_double_field)),
    __fbthrift_field_string_field(std::move(other.__fbthrift_field_string_field)),
    __fbthrift_field_binary_field(std::move(other.__fbthrift_field_binary_field)),
    __fbthrift_field_enum_field(std::move(other.__fbthrift_field_enum_field)),
    __fbthrift_field_list_field(std::move(other.__fbthrift_field_list_field)),
    __fbthrift_field_set_field(std::move(other.__fbthrift_field_set_field)),
    __fbthrift_field_map_field(std::move(other.__fbthrift_field_map_field)),
    __fbthrift_field_struct_field(std::move(other.__fbthrift_field_struct_field)) {
}

EmptiableTerseStruct& EmptiableTerseStruct::operator=([[maybe_unused]] EmptiableTerseStruct&& other) noexcept {
    this->__fbthrift_field_bool_field = std::move(other.__fbthrift_field_bool_field);
    this->__fbthrift_field_byte_field = std::move(other.__fbthrift_field_byte_field);
    this->__fbthrift_field_short_field = std::move(other.__fbthrift_field_short_field);
    this->__fbthrift_field_int_field = std::move(other.__fbthrift_field_int_field);
    this->__fbthrift_field_long_field = std::move(other.__fbthrift_field_long_field);
    this->__fbthrift_field_float_field = std::move(other.__fbthrift_field_float_field);
    this->__fbthrift_field_double_field = std::move(other.__fbthrift_field_double_field);
    this->__fbthrift_field_string_field = std::move(other.__fbthrift_field_string_field);
    this->__fbthrift_field_binary_field = std::move(other.__fbthrift_field_binary_field);
    this->__fbthrift_field_enum_field = std::move(other.__fbthrift_field_enum_field);
    this->__fbthrift_field_list_field = std::move(other.__fbthrift_field_list_field);
    this->__fbthrift_field_set_field = std::move(other.__fbthrift_field_set_field);
    this->__fbthrift_field_map_field = std::move(other.__fbthrift_field_map_field);
    this->__fbthrift_field_struct_field = std::move(other.__fbthrift_field_struct_field);
    return *this;
}


EmptiableTerseStruct::EmptiableTerseStruct(apache::thrift::FragileConstructor, bool bool_field__arg, ::std::int8_t byte_field__arg, ::std::int16_t short_field__arg, ::std::int32_t int_field__arg, ::std::int64_t long_field__arg, float float_field__arg, double double_field__arg, ::std::string string_field__arg, ::std::string binary_field__arg, ::apache::thrift::test::MyEnum enum_field__arg, ::std::vector<::std::int16_t> list_field__arg, ::std::set<::std::int16_t> set_field__arg, ::std::map<::std::int16_t, ::std::int16_t> map_field__arg, ::apache::thrift::test::MyStruct struct_field__arg) :
    __fbthrift_field_bool_field(std::move(bool_field__arg)),
    __fbthrift_field_byte_field(std::move(byte_field__arg)),
    __fbthrift_field_short_field(std::move(short_field__arg)),
    __fbthrift_field_int_field(std::move(int_field__arg)),
    __fbthrift_field_long_field(std::move(long_field__arg)),
    __fbthrift_field_float_field(std::move(float_field__arg)),
    __fbthrift_field_double_field(std::move(double_field__arg)),
    __fbthrift_field_string_field(std::move(string_field__arg)),
    __fbthrift_field_binary_field(std::move(binary_field__arg)),
    __fbthrift_field_enum_field(std::move(enum_field__arg)),
    __fbthrift_field_list_field(std::move(list_field__arg)),
    __fbthrift_field_set_field(std::move(set_field__arg)),
    __fbthrift_field_map_field(std::move(map_field__arg)),
    __fbthrift_field_struct_field(std::move(struct_field__arg)) { 
}


void EmptiableTerseStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_bool_field = bool();
  this->__fbthrift_field_byte_field = ::std::int8_t();
  this->__fbthrift_field_short_field = ::std::int16_t();
  this->__fbthrift_field_int_field = ::std::int32_t();
  this->__fbthrift_field_long_field = ::std::int64_t();
  this->__fbthrift_field_float_field = float();
  this->__fbthrift_field_double_field = double();
  this->__fbthrift_field_string_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_binary_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_enum_field = ::apache::thrift::test::MyEnum();
  this->__fbthrift_field_list_field.clear();
  this->__fbthrift_field_set_field.clear();
  this->__fbthrift_field_map_field.clear();
}

void EmptiableTerseStruct::__fbthrift_clear_terse_fields() {
  this->__fbthrift_field_bool_field = bool();
  this->__fbthrift_field_byte_field = ::std::int8_t();
  this->__fbthrift_field_short_field = ::std::int16_t();
  this->__fbthrift_field_int_field = ::std::int32_t();
  this->__fbthrift_field_long_field = ::std::int64_t();
  this->__fbthrift_field_float_field = float();
  this->__fbthrift_field_double_field = double();
  this->__fbthrift_field_string_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_binary_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_enum_field = ::apache::thrift::test::MyEnum();
  this->__fbthrift_field_list_field.clear();
  this->__fbthrift_field_set_field.clear();
  this->__fbthrift_field_map_field.clear();
}

bool EmptiableTerseStruct::__fbthrift_is_empty() const {
  return ::apache::thrift::op::isEmpty<::apache::thrift::type::bool_t>(this->__fbthrift_field_bool_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::byte_t>(this->__fbthrift_field_byte_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::i16_t>(this->__fbthrift_field_short_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::i32_t>(this->__fbthrift_field_int_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::i64_t>(this->__fbthrift_field_long_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::float_t>(this->__fbthrift_field_float_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::double_t>(this->__fbthrift_field_double_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::string_t>(this->__fbthrift_field_string_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::binary_t>(this->__fbthrift_field_binary_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::enum_t<::apache::thrift::test::MyEnum>>(this->__fbthrift_field_enum_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::list<::apache::thrift::type::i16_t>>(this->__fbthrift_field_list_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::set<::apache::thrift::type::i16_t>>(this->__fbthrift_field_set_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::map<::apache::thrift::type::i16_t, ::apache::thrift::type::i16_t>>(this->__fbthrift_field_map_field) &&
 ::apache::thrift::op::isEmpty<::apache::thrift::type::struct_t<::apache::thrift::test::MyStruct>>(this->__fbthrift_field_struct_field);
}

bool EmptiableTerseStruct::operator==([[maybe_unused]] const EmptiableTerseStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool EmptiableTerseStruct::operator<([[maybe_unused]] const EmptiableTerseStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] EmptiableTerseStruct& a, [[maybe_unused]] EmptiableTerseStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_bool_field, b.__fbthrift_field_bool_field);
  swap(a.__fbthrift_field_byte_field, b.__fbthrift_field_byte_field);
  swap(a.__fbthrift_field_short_field, b.__fbthrift_field_short_field);
  swap(a.__fbthrift_field_int_field, b.__fbthrift_field_int_field);
  swap(a.__fbthrift_field_long_field, b.__fbthrift_field_long_field);
  swap(a.__fbthrift_field_float_field, b.__fbthrift_field_float_field);
  swap(a.__fbthrift_field_double_field, b.__fbthrift_field_double_field);
  swap(a.__fbthrift_field_string_field, b.__fbthrift_field_string_field);
  swap(a.__fbthrift_field_binary_field, b.__fbthrift_field_binary_field);
  swap(a.__fbthrift_field_enum_field, b.__fbthrift_field_enum_field);
  swap(a.__fbthrift_field_list_field, b.__fbthrift_field_list_field);
  swap(a.__fbthrift_field_set_field, b.__fbthrift_field_set_field);
  swap(a.__fbthrift_field_map_field, b.__fbthrift_field_map_field);
  swap(a.__fbthrift_field_struct_field, b.__fbthrift_field_struct_field);
}

template void EmptiableTerseStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EmptiableTerseStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EmptiableTerseStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EmptiableTerseStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void EmptiableTerseStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t EmptiableTerseStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t EmptiableTerseStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t EmptiableTerseStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        EmptiableTerseStruct,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::test::MyStruct>,
    "inconsistent use of json option");

} // namespace apache::thrift::test

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::apache::thrift::test::NotEmptiableStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::apache::thrift::test::NotEmptiableStruct>;
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

namespace apache::thrift::test {

std::string_view NotEmptiableStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<NotEmptiableStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view NotEmptiableStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<NotEmptiableStruct>::name;
}

NotEmptiableStruct::NotEmptiableStruct(const NotEmptiableStruct&) = default;
NotEmptiableStruct& NotEmptiableStruct::operator=(const NotEmptiableStruct&) = default;
NotEmptiableStruct::NotEmptiableStruct() :
    __fbthrift_field_bool_field(),
    __fbthrift_field_byte_field(),
    __fbthrift_field_short_field(),
    __fbthrift_field_int_field(),
    __fbthrift_field_long_field(),
    __fbthrift_field_float_field(),
    __fbthrift_field_double_field(),
    __fbthrift_field_enum_field() {
}


NotEmptiableStruct::~NotEmptiableStruct() {}

NotEmptiableStruct::NotEmptiableStruct([[maybe_unused]] NotEmptiableStruct&& other) noexcept :
    __fbthrift_field_bool_field(std::move(other.__fbthrift_field_bool_field)),
    __fbthrift_field_byte_field(std::move(other.__fbthrift_field_byte_field)),
    __fbthrift_field_short_field(std::move(other.__fbthrift_field_short_field)),
    __fbthrift_field_int_field(std::move(other.__fbthrift_field_int_field)),
    __fbthrift_field_long_field(std::move(other.__fbthrift_field_long_field)),
    __fbthrift_field_float_field(std::move(other.__fbthrift_field_float_field)),
    __fbthrift_field_double_field(std::move(other.__fbthrift_field_double_field)),
    __fbthrift_field_string_field(std::move(other.__fbthrift_field_string_field)),
    __fbthrift_field_binary_field(std::move(other.__fbthrift_field_binary_field)),
    __fbthrift_field_enum_field(std::move(other.__fbthrift_field_enum_field)),
    __fbthrift_field_list_field(std::move(other.__fbthrift_field_list_field)),
    __fbthrift_field_set_field(std::move(other.__fbthrift_field_set_field)),
    __fbthrift_field_map_field(std::move(other.__fbthrift_field_map_field)),
    __fbthrift_field_struct_field(std::move(other.__fbthrift_field_struct_field)),
    __isset(other.__isset) {
}

NotEmptiableStruct& NotEmptiableStruct::operator=([[maybe_unused]] NotEmptiableStruct&& other) noexcept {
    this->__fbthrift_field_bool_field = std::move(other.__fbthrift_field_bool_field);
    this->__fbthrift_field_byte_field = std::move(other.__fbthrift_field_byte_field);
    this->__fbthrift_field_short_field = std::move(other.__fbthrift_field_short_field);
    this->__fbthrift_field_int_field = std::move(other.__fbthrift_field_int_field);
    this->__fbthrift_field_long_field = std::move(other.__fbthrift_field_long_field);
    this->__fbthrift_field_float_field = std::move(other.__fbthrift_field_float_field);
    this->__fbthrift_field_double_field = std::move(other.__fbthrift_field_double_field);
    this->__fbthrift_field_string_field = std::move(other.__fbthrift_field_string_field);
    this->__fbthrift_field_binary_field = std::move(other.__fbthrift_field_binary_field);
    this->__fbthrift_field_enum_field = std::move(other.__fbthrift_field_enum_field);
    this->__fbthrift_field_list_field = std::move(other.__fbthrift_field_list_field);
    this->__fbthrift_field_set_field = std::move(other.__fbthrift_field_set_field);
    this->__fbthrift_field_map_field = std::move(other.__fbthrift_field_map_field);
    this->__fbthrift_field_struct_field = std::move(other.__fbthrift_field_struct_field);
    __isset = other.__isset;
    return *this;
}


NotEmptiableStruct::NotEmptiableStruct(apache::thrift::FragileConstructor, bool bool_field__arg, ::std::int8_t byte_field__arg, ::std::int16_t short_field__arg, ::std::int32_t int_field__arg, ::std::int64_t long_field__arg, float float_field__arg, double double_field__arg, ::std::string string_field__arg, ::std::string binary_field__arg, ::apache::thrift::test::MyEnum enum_field__arg, ::std::vector<::std::int16_t> list_field__arg, ::std::set<::std::int16_t> set_field__arg, ::std::map<::std::int16_t, ::std::int16_t> map_field__arg, ::apache::thrift::test::MyStruct struct_field__arg) :
    __fbthrift_field_bool_field(std::move(bool_field__arg)),
    __fbthrift_field_byte_field(std::move(byte_field__arg)),
    __fbthrift_field_short_field(std::move(short_field__arg)),
    __fbthrift_field_int_field(std::move(int_field__arg)),
    __fbthrift_field_long_field(std::move(long_field__arg)),
    __fbthrift_field_float_field(std::move(float_field__arg)),
    __fbthrift_field_double_field(std::move(double_field__arg)),
    __fbthrift_field_string_field(std::move(string_field__arg)),
    __fbthrift_field_binary_field(std::move(binary_field__arg)),
    __fbthrift_field_enum_field(std::move(enum_field__arg)),
    __fbthrift_field_list_field(std::move(list_field__arg)),
    __fbthrift_field_set_field(std::move(set_field__arg)),
    __fbthrift_field_map_field(std::move(map_field__arg)),
    __fbthrift_field_struct_field(std::move(struct_field__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
  __isset.set(folly::index_constant<6>(), true);
  __isset.set(folly::index_constant<7>(), true);
  __isset.set(folly::index_constant<8>(), true);
  __isset.set(folly::index_constant<9>(), true);
  __isset.set(folly::index_constant<10>(), true);
  __isset.set(folly::index_constant<11>(), true);
  __isset.set(folly::index_constant<12>(), true);
  __isset.set(folly::index_constant<13>(), true);
}


void NotEmptiableStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_bool_field = bool();
  this->__fbthrift_field_byte_field = ::std::int8_t();
  this->__fbthrift_field_short_field = ::std::int16_t();
  this->__fbthrift_field_int_field = ::std::int32_t();
  this->__fbthrift_field_long_field = ::std::int64_t();
  this->__fbthrift_field_float_field = float();
  this->__fbthrift_field_double_field = double();
  this->__fbthrift_field_string_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_binary_field = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_enum_field = ::apache::thrift::test::MyEnum();
  this->__fbthrift_field_list_field.clear();
  this->__fbthrift_field_set_field.clear();
  this->__fbthrift_field_map_field.clear();
  __isset = {};
}

void NotEmptiableStruct::__fbthrift_clear_terse_fields() {
}

bool NotEmptiableStruct::__fbthrift_is_empty() const {
  return false;
}

bool NotEmptiableStruct::operator==([[maybe_unused]] const NotEmptiableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool NotEmptiableStruct::operator<([[maybe_unused]] const NotEmptiableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}

const ::std::vector<::std::int16_t>* NotEmptiableStruct::get_list_field() const& {
  return list_field_ref().has_value() ? std::addressof(__fbthrift_field_list_field) : nullptr;
}

::std::vector<::std::int16_t>* NotEmptiableStruct::get_list_field() & {
  return list_field_ref().has_value() ? std::addressof(__fbthrift_field_list_field) : nullptr;
}

const ::std::set<::std::int16_t>* NotEmptiableStruct::get_set_field() const& {
  return set_field_ref().has_value() ? std::addressof(__fbthrift_field_set_field) : nullptr;
}

::std::set<::std::int16_t>* NotEmptiableStruct::get_set_field() & {
  return set_field_ref().has_value() ? std::addressof(__fbthrift_field_set_field) : nullptr;
}

const ::std::map<::std::int16_t, ::std::int16_t>* NotEmptiableStruct::get_map_field() const& {
  return map_field_ref().has_value() ? std::addressof(__fbthrift_field_map_field) : nullptr;
}

::std::map<::std::int16_t, ::std::int16_t>* NotEmptiableStruct::get_map_field() & {
  return map_field_ref().has_value() ? std::addressof(__fbthrift_field_map_field) : nullptr;
}

const ::apache::thrift::test::MyStruct* NotEmptiableStruct::get_struct_field() const& {
  return struct_field_ref().has_value() ? std::addressof(__fbthrift_field_struct_field) : nullptr;
}

::apache::thrift::test::MyStruct* NotEmptiableStruct::get_struct_field() & {
  return struct_field_ref().has_value() ? std::addressof(__fbthrift_field_struct_field) : nullptr;
}


void swap([[maybe_unused]] NotEmptiableStruct& a, [[maybe_unused]] NotEmptiableStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_bool_field, b.__fbthrift_field_bool_field);
  swap(a.__fbthrift_field_byte_field, b.__fbthrift_field_byte_field);
  swap(a.__fbthrift_field_short_field, b.__fbthrift_field_short_field);
  swap(a.__fbthrift_field_int_field, b.__fbthrift_field_int_field);
  swap(a.__fbthrift_field_long_field, b.__fbthrift_field_long_field);
  swap(a.__fbthrift_field_float_field, b.__fbthrift_field_float_field);
  swap(a.__fbthrift_field_double_field, b.__fbthrift_field_double_field);
  swap(a.__fbthrift_field_string_field, b.__fbthrift_field_string_field);
  swap(a.__fbthrift_field_binary_field, b.__fbthrift_field_binary_field);
  swap(a.__fbthrift_field_enum_field, b.__fbthrift_field_enum_field);
  swap(a.__fbthrift_field_list_field, b.__fbthrift_field_list_field);
  swap(a.__fbthrift_field_set_field, b.__fbthrift_field_set_field);
  swap(a.__fbthrift_field_map_field, b.__fbthrift_field_map_field);
  swap(a.__fbthrift_field_struct_field, b.__fbthrift_field_struct_field);
  swap(a.__isset, b.__isset);
}

template void NotEmptiableStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t NotEmptiableStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t NotEmptiableStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t NotEmptiableStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void NotEmptiableStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t NotEmptiableStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t NotEmptiableStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t NotEmptiableStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        NotEmptiableStruct,
        ::apache::thrift::type_class::structure,
        ::apache::thrift::test::MyStruct>,
    "inconsistent use of json option");

} // namespace apache::thrift::test

namespace apache::thrift::test { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace apache::thrift::test
namespace apache::thrift::detail::annotation {
}
