/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/complex-union/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/complex-union/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/complex-union/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/complex-union/gen-cpp2/module_data.h"
[[maybe_unused]] static constexpr std::string_view kModuleName = "module";


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::ComplexUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::ComplexUnion>;
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

folly::Range<::cpp2::ComplexUnion::Type const*> const TEnumTraits<::cpp2::ComplexUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::ComplexUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::ComplexUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::ComplexUnion::Type>::names);

bool TEnumTraits<::cpp2::ComplexUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::ComplexUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view ComplexUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<ComplexUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view ComplexUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<ComplexUnion>::name;
}

void ComplexUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::intValue:
      ::std::destroy_at(::std::addressof(value_.intValue));
      break;
    case Type::stringValue:
      ::std::destroy_at(::std::addressof(value_.stringValue));
      break;
    case Type::intListValue:
      ::std::destroy_at(::std::addressof(value_.intListValue));
      break;
    case Type::stringListValue:
      ::std::destroy_at(::std::addressof(value_.stringListValue));
      break;
    case Type::typedefValue:
      ::std::destroy_at(::std::addressof(value_.typedefValue));
      break;
    case Type::stringRef:
      ::std::destroy_at(::std::addressof(value_.stringRef));
      break;
    default:
      assert(false);
      break;
  }
}

void ComplexUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  ComplexUnion::~ComplexUnion() {
    __fbthrift_destruct();
  }

bool ComplexUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  ComplexUnion::ComplexUnion(const ComplexUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::intValue:
        set_intValue(rhs.value_.intValue);
        break;
      case Type::stringValue:
        set_stringValue(rhs.value_.stringValue);
        break;
      case Type::intListValue:
        set_intListValue(rhs.value_.intListValue);
        break;
      case Type::stringListValue:
        set_stringListValue(rhs.value_.stringListValue);
        break;
      case Type::typedefValue:
        set_typedefValue(rhs.value_.typedefValue);
        break;
      case Type::stringRef:
        set_stringRef(::apache::thrift::detail::st::copy_field<
          ::apache::thrift::type_class::string>(rhs.value_.stringRef));
        break;
      default:
        assert(false);
    }
  }

    ComplexUnion&ComplexUnion::operator=(const ComplexUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::intValue:
        set_intValue(rhs.value_.intValue);
        break;
      case Type::stringValue:
        set_stringValue(rhs.value_.stringValue);
        break;
      case Type::intListValue:
        set_intListValue(rhs.value_.intListValue);
        break;
      case Type::stringListValue:
        set_stringListValue(rhs.value_.stringListValue);
        break;
      case Type::typedefValue:
        set_typedefValue(rhs.value_.typedefValue);
        break;
      case Type::stringRef:
        set_stringRef(::apache::thrift::detail::st::copy_field<
          ::apache::thrift::type_class::string>(rhs.value_.stringRef));
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool ComplexUnion::operator==(const ComplexUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool ComplexUnion::operator<([[maybe_unused]] const ComplexUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(ComplexUnion& a, ComplexUnion& b) {
  ComplexUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void ComplexUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t ComplexUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t ComplexUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t ComplexUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void ComplexUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t ComplexUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t ComplexUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t ComplexUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::ListUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::ListUnion>;
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

folly::Range<::cpp2::ListUnion::Type const*> const TEnumTraits<::cpp2::ListUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::ListUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::ListUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::ListUnion::Type>::names);

bool TEnumTraits<::cpp2::ListUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::ListUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view ListUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<ListUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view ListUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<ListUnion>::name;
}

void ListUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::intListValue:
      ::std::destroy_at(::std::addressof(value_.intListValue));
      break;
    case Type::stringListValue:
      ::std::destroy_at(::std::addressof(value_.stringListValue));
      break;
    default:
      assert(false);
      break;
  }
}

void ListUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  ListUnion::~ListUnion() {
    __fbthrift_destruct();
  }

bool ListUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  ListUnion::ListUnion(const ListUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::intListValue:
        set_intListValue(rhs.value_.intListValue);
        break;
      case Type::stringListValue:
        set_stringListValue(rhs.value_.stringListValue);
        break;
      default:
        assert(false);
    }
  }

    ListUnion&ListUnion::operator=(const ListUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::intListValue:
        set_intListValue(rhs.value_.intListValue);
        break;
      case Type::stringListValue:
        set_stringListValue(rhs.value_.stringListValue);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool ListUnion::operator==(const ListUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool ListUnion::operator<([[maybe_unused]] const ListUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(ListUnion& a, ListUnion& b) {
  ListUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void ListUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t ListUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t ListUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t ListUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void ListUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t ListUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t ListUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t ListUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::DataUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::DataUnion>;
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

folly::Range<::cpp2::DataUnion::Type const*> const TEnumTraits<::cpp2::DataUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::DataUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::DataUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::DataUnion::Type>::names);

bool TEnumTraits<::cpp2::DataUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::DataUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view DataUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<DataUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view DataUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<DataUnion>::name;
}

void DataUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::binaryData:
      ::std::destroy_at(::std::addressof(value_.binaryData));
      break;
    case Type::stringData:
      ::std::destroy_at(::std::addressof(value_.stringData));
      break;
    default:
      assert(false);
      break;
  }
}

void DataUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  DataUnion::~DataUnion() {
    __fbthrift_destruct();
  }

bool DataUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  DataUnion::DataUnion(const DataUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::binaryData:
        set_binaryData(rhs.value_.binaryData);
        break;
      case Type::stringData:
        set_stringData(rhs.value_.stringData);
        break;
      default:
        assert(false);
    }
  }

    DataUnion&DataUnion::operator=(const DataUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::binaryData:
        set_binaryData(rhs.value_.binaryData);
        break;
      case Type::stringData:
        set_stringData(rhs.value_.stringData);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool DataUnion::operator==(const DataUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool DataUnion::operator<([[maybe_unused]] const DataUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(DataUnion& a, DataUnion& b) {
  DataUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void DataUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t DataUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t DataUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t DataUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void DataUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t DataUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t DataUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t DataUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Val>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Val>;
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

std::string_view Val::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Val>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view Val::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Val>::name;
}

Val::Val(const Val&) = default;
Val& Val::operator=(const Val&) = default;
Val::Val() :
    __fbthrift_field_intVal() {
}


Val::~Val() {}

Val::Val([[maybe_unused]] Val&& other) noexcept :
    __fbthrift_field_strVal(std::move(other.__fbthrift_field_strVal)),
    __fbthrift_field_intVal(std::move(other.__fbthrift_field_intVal)),
    __fbthrift_field_typedefValue(std::move(other.__fbthrift_field_typedefValue)),
    __isset(other.__isset) {
}

Val& Val::operator=([[maybe_unused]] Val&& other) noexcept {
    this->__fbthrift_field_strVal = std::move(other.__fbthrift_field_strVal);
    this->__fbthrift_field_intVal = std::move(other.__fbthrift_field_intVal);
    this->__fbthrift_field_typedefValue = std::move(other.__fbthrift_field_typedefValue);
    __isset = other.__isset;
    return *this;
}


Val::Val(apache::thrift::FragileConstructor, ::std::string strVal__arg, ::std::int32_t intVal__arg, ::cpp2::containerTypedef typedefValue__arg) :
    __fbthrift_field_strVal(std::move(strVal__arg)),
    __fbthrift_field_intVal(std::move(intVal__arg)),
    __fbthrift_field_typedefValue(std::move(typedefValue__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
}


void Val::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_strVal = apache::thrift::StringTraits<::std::string>::fromStringLiteral("");
  this->__fbthrift_field_intVal = ::std::int32_t();
  this->__fbthrift_field_typedefValue.clear();
  __isset = {};
}

void Val::__fbthrift_clear_terse_fields() {
}

bool Val::__fbthrift_is_empty() const {
  return false;
}

bool Val::operator==([[maybe_unused]] const Val& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool Val::operator<([[maybe_unused]] const Val& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


::std::int32_t Val::get_intVal() const {
  return __fbthrift_field_intVal;
}

::std::int32_t& Val::set_intVal(::std::int32_t intVal_) {
  intVal_ref() = intVal_;
  return __fbthrift_field_intVal;
}

const ::cpp2::containerTypedef& Val::get_typedefValue() const& {
  return __fbthrift_field_typedefValue;
}

::cpp2::containerTypedef Val::get_typedefValue() && {
  return static_cast<::cpp2::containerTypedef&&>(__fbthrift_field_typedefValue);
}

void swap([[maybe_unused]] Val& a, [[maybe_unused]] Val& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_strVal, b.__fbthrift_field_strVal);
  swap(a.__fbthrift_field_intVal, b.__fbthrift_field_intVal);
  swap(a.__fbthrift_field_typedefValue, b.__fbthrift_field_typedefValue);
  swap(a.__isset, b.__isset);
}

template void Val::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Val::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Val::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Val::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Val::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Val::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Val::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Val::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::ValUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::ValUnion>;
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

folly::Range<::cpp2::ValUnion::Type const*> const TEnumTraits<::cpp2::ValUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::ValUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::ValUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::ValUnion::Type>::names);

bool TEnumTraits<::cpp2::ValUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::ValUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view ValUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<ValUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view ValUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<ValUnion>::name;
}

void ValUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::v1:
      ::std::destroy_at(::std::addressof(value_.v1));
      break;
    case Type::v2:
      ::std::destroy_at(::std::addressof(value_.v2));
      break;
    default:
      assert(false);
      break;
  }
}

void ValUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  ValUnion::~ValUnion() {
    __fbthrift_destruct();
  }

bool ValUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  ValUnion::ValUnion(const ValUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::v1:
        set_v1(rhs.value_.v1);
        break;
      case Type::v2:
        set_v2(rhs.value_.v2);
        break;
      default:
        assert(false);
    }
  }

    ValUnion&ValUnion::operator=(const ValUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::v1:
        set_v1(rhs.value_.v1);
        break;
      case Type::v2:
        set_v2(rhs.value_.v2);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool ValUnion::operator==(const ValUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool ValUnion::operator<([[maybe_unused]] const ValUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(ValUnion& a, ValUnion& b) {
  ValUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void ValUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t ValUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t ValUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t ValUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void ValUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t ValUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t ValUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t ValUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        ValUnion,
        ::apache::thrift::type_class::structure,
        ::cpp2::Val>,
    "inconsistent use of json option");
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        ValUnion,
        ::apache::thrift::type_class::structure,
        ::cpp2::Val>,
    "inconsistent use of json option");

} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::VirtualComplexUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::VirtualComplexUnion>;
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

folly::Range<::cpp2::VirtualComplexUnion::Type const*> const TEnumTraits<::cpp2::VirtualComplexUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::VirtualComplexUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::VirtualComplexUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::VirtualComplexUnion::Type>::names);

bool TEnumTraits<::cpp2::VirtualComplexUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::VirtualComplexUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view VirtualComplexUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<VirtualComplexUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view VirtualComplexUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<VirtualComplexUnion>::name;
}

void VirtualComplexUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::thingOne:
      ::std::destroy_at(::std::addressof(value_.thingOne));
      break;
    case Type::thingTwo:
      ::std::destroy_at(::std::addressof(value_.thingTwo));
      break;
    default:
      assert(false);
      break;
  }
}

void VirtualComplexUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  VirtualComplexUnion::~VirtualComplexUnion() {
    __fbthrift_destruct();
  }

bool VirtualComplexUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  VirtualComplexUnion::VirtualComplexUnion(const VirtualComplexUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::thingOne:
        set_thingOne(rhs.value_.thingOne);
        break;
      case Type::thingTwo:
        set_thingTwo(rhs.value_.thingTwo);
        break;
      default:
        assert(false);
    }
  }

    VirtualComplexUnion&VirtualComplexUnion::operator=(const VirtualComplexUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::thingOne:
        set_thingOne(rhs.value_.thingOne);
        break;
      case Type::thingTwo:
        set_thingTwo(rhs.value_.thingTwo);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool VirtualComplexUnion::operator==(const VirtualComplexUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool VirtualComplexUnion::operator<([[maybe_unused]] const VirtualComplexUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(VirtualComplexUnion& a, VirtualComplexUnion& b) {
  VirtualComplexUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void VirtualComplexUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t VirtualComplexUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t VirtualComplexUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t VirtualComplexUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void VirtualComplexUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t VirtualComplexUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t VirtualComplexUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t VirtualComplexUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::NonCopyableStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::NonCopyableStruct>;
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

std::string_view NonCopyableStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<NonCopyableStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view NonCopyableStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<NonCopyableStruct>::name;
}


NonCopyableStruct::NonCopyableStruct(apache::thrift::FragileConstructor, ::std::int64_t num__arg) :
    __fbthrift_field_num(std::move(num__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void NonCopyableStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_num = ::std::int64_t();
  __isset = {};
}

void NonCopyableStruct::__fbthrift_clear_terse_fields() {
}

bool NonCopyableStruct::__fbthrift_is_empty() const {
  return false;
}

bool NonCopyableStruct::operator==([[maybe_unused]] const NonCopyableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool NonCopyableStruct::operator<([[maybe_unused]] const NonCopyableStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


::std::int64_t NonCopyableStruct::get_num() const {
  return __fbthrift_field_num;
}

::std::int64_t& NonCopyableStruct::set_num(::std::int64_t num_) {
  num_ref() = num_;
  return __fbthrift_field_num;
}

void swap([[maybe_unused]] NonCopyableStruct& a, [[maybe_unused]] NonCopyableStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_num, b.__fbthrift_field_num);
  swap(a.__isset, b.__isset);
}

template void NonCopyableStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t NonCopyableStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t NonCopyableStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t NonCopyableStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void NonCopyableStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t NonCopyableStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t NonCopyableStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t NonCopyableStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::NonCopyableUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::NonCopyableUnion>;
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

folly::Range<::cpp2::NonCopyableUnion::Type const*> const TEnumTraits<::cpp2::NonCopyableUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::NonCopyableUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::NonCopyableUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::NonCopyableUnion::Type>::names);

bool TEnumTraits<::cpp2::NonCopyableUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::NonCopyableUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view NonCopyableUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<NonCopyableUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view NonCopyableUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<NonCopyableUnion>::name;
}

void NonCopyableUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::s:
      ::std::destroy_at(::std::addressof(value_.s));
      break;
    default:
      assert(false);
      break;
  }
}

void NonCopyableUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  NonCopyableUnion::~NonCopyableUnion() {
    __fbthrift_destruct();
  }

bool NonCopyableUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}


bool NonCopyableUnion::operator==(const NonCopyableUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool NonCopyableUnion::operator<([[maybe_unused]] const NonCopyableUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(NonCopyableUnion& a, NonCopyableUnion& b) {
  NonCopyableUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void NonCopyableUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t NonCopyableUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t NonCopyableUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t NonCopyableUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void NonCopyableUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t NonCopyableUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t NonCopyableUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t NonCopyableUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        NonCopyableUnion,
        ::apache::thrift::type_class::structure,
        ::cpp2::NonCopyableStruct>,
    "inconsistent use of json option");

} // namespace cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace cpp2
namespace apache::thrift::detail::annotation {
}
