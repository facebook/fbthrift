/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/client-methods/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/client-methods/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/client-methods/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/client-methods/gen-cpp2/module_data.h"
[[maybe_unused]] static constexpr std::string_view kModuleName = "module";


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::EchoRequest>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::EchoRequest>;
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

std::string_view EchoRequest::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<EchoRequest>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view EchoRequest::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<EchoRequest>::name;
}

EchoRequest::EchoRequest(const EchoRequest&) = default;
EchoRequest& EchoRequest::operator=(const EchoRequest&) = default;
EchoRequest::EchoRequest() {
}


EchoRequest::~EchoRequest() {}

EchoRequest::EchoRequest([[maybe_unused]] EchoRequest&& other) noexcept :
    __fbthrift_field_text(std::move(other.__fbthrift_field_text)),
    __isset(other.__isset) {
}

EchoRequest& EchoRequest::operator=([[maybe_unused]] EchoRequest&& other) noexcept {
    this->__fbthrift_field_text = std::move(other.__fbthrift_field_text);
    __isset = other.__isset;
    return *this;
}


EchoRequest::EchoRequest(apache::thrift::FragileConstructor, ::std::string text__arg) :
    __fbthrift_field_text(std::move(text__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void EchoRequest::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_text = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void EchoRequest::__fbthrift_clear_terse_fields() {
}

bool EchoRequest::__fbthrift_is_empty() const {
  return false;
}

bool EchoRequest::operator==([[maybe_unused]] const EchoRequest& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool EchoRequest::operator<([[maybe_unused]] const EchoRequest& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


const ::std::string& EchoRequest::get_text() const& {
  return __fbthrift_field_text;
}

::std::string EchoRequest::get_text() && {
  return static_cast<::std::string&&>(__fbthrift_field_text);
}

void swap([[maybe_unused]] EchoRequest& a, [[maybe_unused]] EchoRequest& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_text, b.__fbthrift_field_text);
  swap(a.__isset, b.__isset);
}

template void EchoRequest::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EchoRequest::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EchoRequest::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EchoRequest::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void EchoRequest::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t EchoRequest::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t EchoRequest::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t EchoRequest::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::EchoResponse>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::EchoResponse>;
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

std::string_view EchoResponse::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<EchoResponse>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view EchoResponse::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<EchoResponse>::name;
}

EchoResponse::EchoResponse(const EchoResponse&) = default;
EchoResponse& EchoResponse::operator=(const EchoResponse&) = default;
EchoResponse::EchoResponse() {
}


EchoResponse::~EchoResponse() {}

EchoResponse::EchoResponse([[maybe_unused]] EchoResponse&& other) noexcept :
    __fbthrift_field_text(std::move(other.__fbthrift_field_text)),
    __isset(other.__isset) {
}

EchoResponse& EchoResponse::operator=([[maybe_unused]] EchoResponse&& other) noexcept {
    this->__fbthrift_field_text = std::move(other.__fbthrift_field_text);
    __isset = other.__isset;
    return *this;
}


EchoResponse::EchoResponse(apache::thrift::FragileConstructor, ::std::string text__arg) :
    __fbthrift_field_text(std::move(text__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
}


void EchoResponse::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_text = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void EchoResponse::__fbthrift_clear_terse_fields() {
}

bool EchoResponse::__fbthrift_is_empty() const {
  return false;
}

bool EchoResponse::operator==([[maybe_unused]] const EchoResponse& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool EchoResponse::operator<([[maybe_unused]] const EchoResponse& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


const ::std::string& EchoResponse::get_text() const& {
  return __fbthrift_field_text;
}

::std::string EchoResponse::get_text() && {
  return static_cast<::std::string&&>(__fbthrift_field_text);
}

void swap([[maybe_unused]] EchoResponse& a, [[maybe_unused]] EchoResponse& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_text, b.__fbthrift_field_text);
  swap(a.__isset, b.__isset);
}

template void EchoResponse::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t EchoResponse::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t EchoResponse::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t EchoResponse::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void EchoResponse::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t EchoResponse::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t EchoResponse::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t EchoResponse::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace cpp2
namespace apache::thrift::detail::annotation {
}
