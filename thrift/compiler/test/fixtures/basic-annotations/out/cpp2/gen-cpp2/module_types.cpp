/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/basic-annotations/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/basic-annotations/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/basic-annotations/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/basic-annotations/gen-cpp2/module_data.h"
[[maybe_unused]] static constexpr std::string_view kModuleName = "module";


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::cpp2::YourEnum>::__fbthrift_module_name_internal_do_not_use = kModuleName;
folly::Range<::cpp2::YourEnum const*> const TEnumTraits<::cpp2::YourEnum>::values = folly::range(TEnumDataStorage<::cpp2::YourEnum>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::YourEnum>::names = folly::range(TEnumDataStorage<::cpp2::YourEnum>::names);

bool TEnumTraits<::cpp2::YourEnum>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::YourEnum>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::MyStructNestedAnnotation>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::MyStructNestedAnnotation>;
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

std::string_view MyStructNestedAnnotation::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<MyStructNestedAnnotation>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view MyStructNestedAnnotation::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<MyStructNestedAnnotation>::name;
}

MyStructNestedAnnotation::MyStructNestedAnnotation(const MyStructNestedAnnotation&) = default;
MyStructNestedAnnotation& MyStructNestedAnnotation::operator=(const MyStructNestedAnnotation&) = default;
MyStructNestedAnnotation::MyStructNestedAnnotation() {
}


MyStructNestedAnnotation::~MyStructNestedAnnotation() {}

MyStructNestedAnnotation::MyStructNestedAnnotation([[maybe_unused]] MyStructNestedAnnotation&& other) noexcept :
    __fbthrift_field_name(std::move(other.__fbthrift_field_name)),
    __isset(other.__isset) {
}

MyStructNestedAnnotation& MyStructNestedAnnotation::operator=([[maybe_unused]] MyStructNestedAnnotation&& other) noexcept {
    this->__fbthrift_field_name = std::move(other.__fbthrift_field_name);
    __isset = other.__isset;
    return *this;
}


MyStructNestedAnnotation::MyStructNestedAnnotation(apache::thrift::FragileConstructor, ::std::string name__arg) :
    __fbthrift_field_name(std::move(name__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void MyStructNestedAnnotation::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_name = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void MyStructNestedAnnotation::__fbthrift_clear_terse_fields() {
}

bool MyStructNestedAnnotation::__fbthrift_is_empty() const {
  return false;
}

bool MyStructNestedAnnotation::operator==([[maybe_unused]] const MyStructNestedAnnotation& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool MyStructNestedAnnotation::operator<([[maybe_unused]] const MyStructNestedAnnotation& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] MyStructNestedAnnotation& a, [[maybe_unused]] MyStructNestedAnnotation& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_name, b.__fbthrift_field_name);
  swap(a.__isset, b.__isset);
}

template void MyStructNestedAnnotation::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t MyStructNestedAnnotation::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t MyStructNestedAnnotation::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t MyStructNestedAnnotation::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void MyStructNestedAnnotation::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t MyStructNestedAnnotation::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t MyStructNestedAnnotation::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t MyStructNestedAnnotation::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::detail::YourUnion>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::detail::YourUnion>;
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

folly::Range<::cpp2::detail::YourUnion::Type const*> const TEnumTraits<::cpp2::detail::YourUnion::Type>::values = folly::range(TEnumDataStorage<::cpp2::detail::YourUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::detail::YourUnion::Type>::names = folly::range(TEnumDataStorage<::cpp2::detail::YourUnion::Type>::names);

bool TEnumTraits<::cpp2::detail::YourUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::detail::YourUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {
namespace detail {

std::string_view YourUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<YourUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view YourUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<YourUnion>::name;
}

void YourUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    default:
      assert(false);
      break;
  }
}

void YourUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}


bool YourUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  YourUnion::YourUnion(const YourUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      default:
        assert(false);
    }
  }

    YourUnion&YourUnion::operator=(const YourUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool YourUnion::operator==(const YourUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool YourUnion::operator<([[maybe_unused]] const YourUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(YourUnion& a, YourUnion& b) {
  YourUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void YourUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t YourUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t YourUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t YourUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void YourUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t YourUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t YourUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t YourUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace detail
} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::detail::YourException>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::detail::YourException>;
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
namespace detail {

std::string_view YourException::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<YourException>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view YourException::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<YourException>::name;
}

YourException::YourException(const YourException&) = default;
YourException& YourException::operator=(const YourException&) = default;
YourException::YourException() {
}


YourException::~YourException() {}

YourException::YourException([[maybe_unused]] YourException&& other) noexcept{}
YourException& YourException::operator=([[maybe_unused]] YourException&& other) noexcept {
    return *this;
}


YourException::YourException(apache::thrift::FragileConstructor) {}


void YourException::__fbthrift_clear() {
  // clear all fields
}

void YourException::__fbthrift_clear_terse_fields() {
}

bool YourException::__fbthrift_is_empty() const {
  return true;
}

bool YourException::operator==([[maybe_unused]] const YourException& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool YourException::operator<([[maybe_unused]] const YourException& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] YourException& a, [[maybe_unused]] YourException& b) {
  using ::std::swap;
}

template void YourException::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t YourException::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t YourException::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t YourException::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void YourException::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t YourException::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t YourException::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t YourException::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace detail
} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::detail::YourStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::detail::YourStruct>;
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
namespace detail {

const char* YourStruct::__fbthrift_thrift_uri() {
  return "facebook.com/thrift/compiler/test/fixtures/basic-annotations/src/module/MyStruct";
}

std::string_view YourStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<YourStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view YourStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<YourStruct>::name;
}

YourStruct::YourStruct(const YourStruct& srcObj) :
    __fbthrift_field_majorVer(srcObj.__fbthrift_field_majorVer),
    __fbthrift_field_abstract(srcObj.__fbthrift_field_abstract),
    __fbthrift_field_annotation_with_quote(srcObj.__fbthrift_field_annotation_with_quote),
    __fbthrift_field_class_(srcObj.__fbthrift_field_class_),
    __fbthrift_field_annotation_with_trailing_comma(srcObj.__fbthrift_field_annotation_with_trailing_comma),
    __fbthrift_field_empty_annotations(srcObj.__fbthrift_field_empty_annotations),
    __fbthrift_field_my_enum(srcObj.__fbthrift_field_my_enum),
    __fbthrift_field_cpp_type_annotation(srcObj.__fbthrift_field_cpp_type_annotation),
    __fbthrift_field_my_union(srcObj.__fbthrift_field_my_union),
    __isset(srcObj.__isset) {
  ::apache::thrift::adapt_detail::construct<::StaticCast, 9>(__fbthrift_field_my_union, *this);
}

static void __fbthrift_swap(YourStruct& lhs, YourStruct& rhs) { swap(lhs, rhs); }
YourStruct& YourStruct::operator=(const YourStruct& other) {
  YourStruct tmp(other);
  __fbthrift_swap(*this, tmp);
  return *this;
}

YourStruct::YourStruct() :
    __fbthrift_field_majorVer(),
    __fbthrift_field_my_enum() {
  ::apache::thrift::adapt_detail::construct<::StaticCast, 9>(__fbthrift_field_my_union, *this);
}


YourStruct::~YourStruct() {}

YourStruct::YourStruct([[maybe_unused]] YourStruct&& other) noexcept :
    __fbthrift_field_majorVer(std::move(other.__fbthrift_field_majorVer)),
    __fbthrift_field_abstract(std::move(other.__fbthrift_field_abstract)),
    __fbthrift_field_annotation_with_quote(std::move(other.__fbthrift_field_annotation_with_quote)),
    __fbthrift_field_class_(std::move(other.__fbthrift_field_class_)),
    __fbthrift_field_annotation_with_trailing_comma(std::move(other.__fbthrift_field_annotation_with_trailing_comma)),
    __fbthrift_field_empty_annotations(std::move(other.__fbthrift_field_empty_annotations)),
    __fbthrift_field_my_enum(std::move(other.__fbthrift_field_my_enum)),
    __fbthrift_field_cpp_type_annotation(std::move(other.__fbthrift_field_cpp_type_annotation)),
    __fbthrift_field_my_union(std::move(other.__fbthrift_field_my_union)),
    __isset(other.__isset) {
  ::apache::thrift::adapt_detail::construct<::StaticCast, 9>(__fbthrift_field_my_union, *this);
}

YourStruct& YourStruct::operator=([[maybe_unused]] YourStruct&& other) noexcept {
    this->__fbthrift_field_majorVer = std::move(other.__fbthrift_field_majorVer);
    this->__fbthrift_field_abstract = std::move(other.__fbthrift_field_abstract);
    this->__fbthrift_field_annotation_with_quote = std::move(other.__fbthrift_field_annotation_with_quote);
    this->__fbthrift_field_class_ = std::move(other.__fbthrift_field_class_);
    this->__fbthrift_field_annotation_with_trailing_comma = std::move(other.__fbthrift_field_annotation_with_trailing_comma);
    this->__fbthrift_field_empty_annotations = std::move(other.__fbthrift_field_empty_annotations);
    this->__fbthrift_field_my_enum = std::move(other.__fbthrift_field_my_enum);
    this->__fbthrift_field_cpp_type_annotation = std::move(other.__fbthrift_field_cpp_type_annotation);
    this->__fbthrift_field_my_union = std::move(other.__fbthrift_field_my_union);
    __isset = other.__isset;
    return *this;
}


YourStruct::YourStruct(apache::thrift::FragileConstructor, ::std::int64_t majorVer__arg, ::std::string abstract__arg, ::std::string annotation_with_quote__arg, ::std::string class___arg, ::std::string annotation_with_trailing_comma__arg, ::std::string empty_annotations__arg, ::cpp2::YourEnum my_enum__arg, ::cpp2::list_string_6884 cpp_type_annotation__arg, ::cpp2::YourUnion my_union__arg) :
    __fbthrift_field_majorVer(std::move(majorVer__arg)),
    __fbthrift_field_abstract(std::move(abstract__arg)),
    __fbthrift_field_annotation_with_quote(std::move(annotation_with_quote__arg)),
    __fbthrift_field_class_(std::move(class___arg)),
    __fbthrift_field_annotation_with_trailing_comma(std::move(annotation_with_trailing_comma__arg)),
    __fbthrift_field_empty_annotations(std::move(empty_annotations__arg)),
    __fbthrift_field_my_enum(std::move(my_enum__arg)),
    __fbthrift_field_cpp_type_annotation(std::move(cpp_type_annotation__arg)),
    __fbthrift_field_my_union(std::move(my_union__arg)) { 
  ::apache::thrift::adapt_detail::construct<::StaticCast, 9>(__fbthrift_field_my_union, *this);
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
  __isset.set(folly::index_constant<6>(), true);
  __isset.set(folly::index_constant<7>(), true);
  __isset.set(folly::index_constant<8>(), true);
}


void YourStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_majorVer = ::std::int64_t();
  this->__fbthrift_field_abstract = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_annotation_with_quote = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_class_ = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_annotation_with_trailing_comma = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_empty_annotations = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_my_enum = ::cpp2::YourEnum();
  this->__fbthrift_field_cpp_type_annotation.clear();
  __isset = {};
}

void YourStruct::__fbthrift_clear_terse_fields() {
}

bool YourStruct::__fbthrift_is_empty() const {
  return false;
}

bool YourStruct::operator==([[maybe_unused]] const YourStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool YourStruct::operator<([[maybe_unused]] const YourStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}

const ::cpp2::list_string_6884& YourStruct::get_cpp_type_annotation() const& {
  return __fbthrift_field_cpp_type_annotation;
}

::cpp2::list_string_6884 YourStruct::get_cpp_type_annotation() && {
  return std::move(__fbthrift_field_cpp_type_annotation);
}


void swap([[maybe_unused]] YourStruct& a, [[maybe_unused]] YourStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_majorVer, b.__fbthrift_field_majorVer);
  swap(a.__fbthrift_field_abstract, b.__fbthrift_field_abstract);
  swap(a.__fbthrift_field_annotation_with_quote, b.__fbthrift_field_annotation_with_quote);
  swap(a.__fbthrift_field_class_, b.__fbthrift_field_class_);
  swap(a.__fbthrift_field_annotation_with_trailing_comma, b.__fbthrift_field_annotation_with_trailing_comma);
  swap(a.__fbthrift_field_empty_annotations, b.__fbthrift_field_empty_annotations);
  swap(a.__fbthrift_field_my_enum, b.__fbthrift_field_my_enum);
  swap(a.__fbthrift_field_cpp_type_annotation, b.__fbthrift_field_cpp_type_annotation);
  swap(a.__fbthrift_field_my_union, b.__fbthrift_field_my_union);
  swap(a.__isset, b.__isset);
}

template void YourStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t YourStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t YourStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t YourStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void YourStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t YourStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t YourStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t YourStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        YourStruct,
        ::apache::thrift::type_class::variant,
        ::cpp2::YourUnion>,
    "inconsistent use of json option");

} // namespace detail
} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::SecretStruct>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::SecretStruct>;
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

std::string_view SecretStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<SecretStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view SecretStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<SecretStruct>::name;
}

SecretStruct::SecretStruct(const SecretStruct&) = default;
SecretStruct& SecretStruct::operator=(const SecretStruct&) = default;
SecretStruct::SecretStruct() :
    __fbthrift_field_id() {
}


SecretStruct::~SecretStruct() {}

SecretStruct::SecretStruct([[maybe_unused]] SecretStruct&& other) noexcept :
    __fbthrift_field_id(std::move(other.__fbthrift_field_id)),
    __fbthrift_field_password(std::move(other.__fbthrift_field_password)),
    __isset(other.__isset) {
}

SecretStruct& SecretStruct::operator=([[maybe_unused]] SecretStruct&& other) noexcept {
    this->__fbthrift_field_id = std::move(other.__fbthrift_field_id);
    this->__fbthrift_field_password = std::move(other.__fbthrift_field_password);
    __isset = other.__isset;
    return *this;
}


SecretStruct::SecretStruct(apache::thrift::FragileConstructor, ::std::int64_t id__arg, ::std::string password__arg) :
    __fbthrift_field_id(std::move(id__arg)),
    __fbthrift_field_password(std::move(password__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
}


void SecretStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_id = ::std::int64_t();
  this->__fbthrift_field_password = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void SecretStruct::__fbthrift_clear_terse_fields() {
}

bool SecretStruct::__fbthrift_is_empty() const {
  return false;
}

bool SecretStruct::operator==([[maybe_unused]] const SecretStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool SecretStruct::operator<([[maybe_unused]] const SecretStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] SecretStruct& a, [[maybe_unused]] SecretStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_id, b.__fbthrift_field_id);
  swap(a.__fbthrift_field_password, b.__fbthrift_field_password);
  swap(a.__isset, b.__isset);
}

template void SecretStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t SecretStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t SecretStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t SecretStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void SecretStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t SecretStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t SecretStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t SecretStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
  ::apache::thrift::adapt_detail::validateFieldAdapter<::StaticCast, 9, ::cpp2::detail::YourUnion, ::cpp2::detail::YourStruct>();
  ::apache::thrift::adapt_detail::validateAdapter<::StaticCast, ::cpp2::detail::YourStruct>();
  ::apache::thrift::adapt_detail::validateAdapter<::StaticCast, ::cpp2::detail::YourStruct>();
}
}} // namespace cpp2
namespace apache::thrift::detail::annotation {
}
