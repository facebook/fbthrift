/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/constants/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/constants/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/constants/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/constants/gen-cpp2/module_data.h"


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::cpp2::EmptyEnum>::type_name = TEnumDataStorage<::cpp2::EmptyEnum>::type_name;
folly::Range<::cpp2::EmptyEnum const*> const TEnumTraits<::cpp2::EmptyEnum>::values = {};
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::EmptyEnum>::names = {};

bool TEnumTraits<::cpp2::EmptyEnum>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::EmptyEnum>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::cpp2::City>::type_name = TEnumDataStorage<::cpp2::City>::type_name;
folly::Range<::cpp2::City const*> const TEnumTraits<::cpp2::City>::values = folly::range(TEnumDataStorage<::cpp2::City>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::City>::names = folly::range(TEnumDataStorage<::cpp2::City>::names);

bool TEnumTraits<::cpp2::City>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::City>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::cpp2::Company>::type_name = TEnumDataStorage<::cpp2::Company>::type_name;
folly::Range<::cpp2::Company const*> const TEnumTraits<::cpp2::Company>::values = folly::range(TEnumDataStorage<::cpp2::Company>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::Company>::names = folly::range(TEnumDataStorage<::cpp2::Company>::names);

bool TEnumTraits<::cpp2::Company>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::Company>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Internship>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Internship>;
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

std::string_view Internship::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Internship>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view Internship::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Internship>::name;
}

Internship::Internship(const Internship&) = default;
Internship& Internship::operator=(const Internship&) = default;
Internship::Internship() :
    __fbthrift_field_weeks(),
    __fbthrift_field_employer(),
    __fbthrift_field_compensation() {
}


Internship::~Internship() {}

Internship::Internship([[maybe_unused]] Internship&& other) noexcept :
    __fbthrift_field_weeks(std::move(other.__fbthrift_field_weeks)),
    __fbthrift_field_title(std::move(other.__fbthrift_field_title)),
    __fbthrift_field_employer(std::move(other.__fbthrift_field_employer)),
    __fbthrift_field_compensation(std::move(other.__fbthrift_field_compensation)),
    __fbthrift_field_school(std::move(other.__fbthrift_field_school)),
    __isset(other.__isset) {
}

Internship& Internship::operator=([[maybe_unused]] Internship&& other) noexcept {
    this->__fbthrift_field_weeks = std::move(other.__fbthrift_field_weeks);
    this->__fbthrift_field_title = std::move(other.__fbthrift_field_title);
    this->__fbthrift_field_employer = std::move(other.__fbthrift_field_employer);
    this->__fbthrift_field_compensation = std::move(other.__fbthrift_field_compensation);
    this->__fbthrift_field_school = std::move(other.__fbthrift_field_school);
    __isset = other.__isset;
    return *this;
}


Internship::Internship(apache::thrift::FragileConstructor, ::std::int32_t weeks__arg, ::std::string title__arg, ::cpp2::Company employer__arg, double compensation__arg, ::std::string school__arg) :
    __fbthrift_field_weeks(std::move(weeks__arg)),
    __fbthrift_field_title(std::move(title__arg)),
    __fbthrift_field_employer(std::move(employer__arg)),
    __fbthrift_field_compensation(std::move(compensation__arg)),
    __fbthrift_field_school(std::move(school__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void Internship::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_weeks = ::std::int32_t();
  this->__fbthrift_field_title = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_employer = ::cpp2::Company();
  this->__fbthrift_field_compensation = double();
  this->__fbthrift_field_school = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void Internship::__fbthrift_clear_terse_fields() {
}

bool Internship::__fbthrift_is_empty() const {
  return false;
}

bool Internship::operator==([[maybe_unused]] const Internship& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool Internship::operator<([[maybe_unused]] const Internship& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] Internship& a, [[maybe_unused]] Internship& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_weeks, b.__fbthrift_field_weeks);
  swap(a.__fbthrift_field_title, b.__fbthrift_field_title);
  swap(a.__fbthrift_field_employer, b.__fbthrift_field_employer);
  swap(a.__fbthrift_field_compensation, b.__fbthrift_field_compensation);
  swap(a.__fbthrift_field_school, b.__fbthrift_field_school);
  swap(a.__isset, b.__isset);
}

template void Internship::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Internship::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Internship::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Internship::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Internship::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Internship::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Internship::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Internship::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Range>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Range>;
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

std::string_view Range::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Range>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view Range::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Range>::name;
}


Range::Range(apache::thrift::FragileConstructor, ::std::int32_t min__arg, ::std::int32_t max__arg) :
    __fbthrift_field_min(std::move(min__arg)),
    __fbthrift_field_max(std::move(max__arg)) { 
}


void Range::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_min = ::std::int32_t();
  this->__fbthrift_field_max = ::std::int32_t();
}

void Range::__fbthrift_clear_terse_fields() {
}

bool Range::__fbthrift_is_empty() const {
  return false;
}

bool Range::operator==([[maybe_unused]] const Range& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool Range::operator<([[maybe_unused]] const Range& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] Range& a, [[maybe_unused]] Range& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_min, b.__fbthrift_field_min);
  swap(a.__fbthrift_field_max, b.__fbthrift_field_max);
}

template void Range::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Range::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Range::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Range::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Range::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Range::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Range::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Range::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::struct1>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::struct1>;
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

std::string_view struct1::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<struct1>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view struct1::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<struct1>::name;
}

struct1::struct1(const struct1&) = default;
struct1& struct1::operator=(const struct1&) = default;
struct1::struct1() :
    __fbthrift_field_a(static_cast<::std::int32_t>(1234567)),
    __fbthrift_field_b(apache::thrift::StringTraits<std::string>::fromStringLiteral("<uninitialized>")) {
}


struct1::~struct1() {}

struct1::struct1([[maybe_unused]] struct1&& other) noexcept :
    __fbthrift_field_a(std::move(other.__fbthrift_field_a)),
    __fbthrift_field_b(std::move(other.__fbthrift_field_b)),
    __isset(other.__isset) {
}

struct1& struct1::operator=([[maybe_unused]] struct1&& other) noexcept {
    this->__fbthrift_field_a = std::move(other.__fbthrift_field_a);
    this->__fbthrift_field_b = std::move(other.__fbthrift_field_b);
    __isset = other.__isset;
    return *this;
}


struct1::struct1(apache::thrift::FragileConstructor, ::std::int32_t a__arg, ::std::string b__arg) :
    __fbthrift_field_a(std::move(a__arg)),
    __fbthrift_field_b(std::move(b__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
}


void struct1::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_a = ::std::int32_t();
  this->__fbthrift_field_b = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  __isset = {};
}

void struct1::__fbthrift_clear_terse_fields() {
}

bool struct1::__fbthrift_is_empty() const {
  return false;
}

bool struct1::operator==([[maybe_unused]] const struct1& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool struct1::operator<([[maybe_unused]] const struct1& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] struct1& a, [[maybe_unused]] struct1& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_a, b.__fbthrift_field_a);
  swap(a.__fbthrift_field_b, b.__fbthrift_field_b);
  swap(a.__isset, b.__isset);
}

template void struct1::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t struct1::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t struct1::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t struct1::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void struct1::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t struct1::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t struct1::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t struct1::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::struct2>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::struct2>;
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

std::string_view struct2::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<struct2>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view struct2::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<struct2>::name;
}

struct2::struct2(const struct2&) = default;
struct2& struct2::operator=(const struct2&) = default;
struct2::struct2() :
    __fbthrift_field_a() {
}


struct2::~struct2() {}

struct2::struct2([[maybe_unused]] struct2&& other) noexcept :
    __fbthrift_field_a(std::move(other.__fbthrift_field_a)),
    __fbthrift_field_b(std::move(other.__fbthrift_field_b)),
    __fbthrift_field_c(std::move(other.__fbthrift_field_c)),
    __fbthrift_field_d(std::move(other.__fbthrift_field_d)),
    __isset(other.__isset) {
}

struct2& struct2::operator=([[maybe_unused]] struct2&& other) noexcept {
    this->__fbthrift_field_a = std::move(other.__fbthrift_field_a);
    this->__fbthrift_field_b = std::move(other.__fbthrift_field_b);
    this->__fbthrift_field_c = std::move(other.__fbthrift_field_c);
    this->__fbthrift_field_d = std::move(other.__fbthrift_field_d);
    __isset = other.__isset;
    return *this;
}


struct2::struct2(apache::thrift::FragileConstructor, ::std::int32_t a__arg, ::std::string b__arg, ::cpp2::struct1 c__arg, ::std::vector<::std::int32_t> d__arg) :
    __fbthrift_field_a(std::move(a__arg)),
    __fbthrift_field_b(std::move(b__arg)),
    __fbthrift_field_c(std::move(c__arg)),
    __fbthrift_field_d(std::move(d__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void struct2::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_a = ::std::int32_t();
  this->__fbthrift_field_b = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  ::apache::thrift::clear(this->__fbthrift_field_c);
  this->__fbthrift_field_d.clear();
  __isset = {};
}

void struct2::__fbthrift_clear_terse_fields() {
}

bool struct2::__fbthrift_is_empty() const {
  return false;
}

bool struct2::operator==([[maybe_unused]] const struct2& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool struct2::operator<([[maybe_unused]] const struct2& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}

const ::cpp2::struct1& struct2::get_c() const& {
  return __fbthrift_field_c;
}

::cpp2::struct1 struct2::get_c() && {
  return std::move(__fbthrift_field_c);
}

const ::std::vector<::std::int32_t>& struct2::get_d() const& {
  return __fbthrift_field_d;
}

::std::vector<::std::int32_t> struct2::get_d() && {
  return std::move(__fbthrift_field_d);
}


void swap([[maybe_unused]] struct2& a, [[maybe_unused]] struct2& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_a, b.__fbthrift_field_a);
  swap(a.__fbthrift_field_b, b.__fbthrift_field_b);
  swap(a.__fbthrift_field_c, b.__fbthrift_field_c);
  swap(a.__fbthrift_field_d, b.__fbthrift_field_d);
  swap(a.__isset, b.__isset);
}

template void struct2::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t struct2::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t struct2::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t struct2::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void struct2::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t struct2::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t struct2::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t struct2::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        struct2,
        ::apache::thrift::type_class::structure,
        ::cpp2::struct1>,
    "inconsistent use of json option");

} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::struct3>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::struct3>;
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

std::string_view struct3::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<struct3>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view struct3::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<struct3>::name;
}

struct3::struct3(const struct3&) = default;
struct3& struct3::operator=(const struct3&) = default;
struct3::struct3() :
    __fbthrift_field_b() {
}


struct3::~struct3() {}

struct3::struct3([[maybe_unused]] struct3&& other) noexcept :
    __fbthrift_field_a(std::move(other.__fbthrift_field_a)),
    __fbthrift_field_b(std::move(other.__fbthrift_field_b)),
    __fbthrift_field_c(std::move(other.__fbthrift_field_c)),
    __isset(other.__isset) {
}

struct3& struct3::operator=([[maybe_unused]] struct3&& other) noexcept {
    this->__fbthrift_field_a = std::move(other.__fbthrift_field_a);
    this->__fbthrift_field_b = std::move(other.__fbthrift_field_b);
    this->__fbthrift_field_c = std::move(other.__fbthrift_field_c);
    __isset = other.__isset;
    return *this;
}


struct3::struct3(apache::thrift::FragileConstructor, ::std::string a__arg, ::std::int32_t b__arg, ::cpp2::struct2 c__arg) :
    __fbthrift_field_a(std::move(a__arg)),
    __fbthrift_field_b(std::move(b__arg)),
    __fbthrift_field_c(std::move(c__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
}


void struct3::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_a = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_b = ::std::int32_t();
  ::apache::thrift::clear(this->__fbthrift_field_c);
  __isset = {};
}

void struct3::__fbthrift_clear_terse_fields() {
}

bool struct3::__fbthrift_is_empty() const {
  return false;
}

bool struct3::operator==([[maybe_unused]] const struct3& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool struct3::operator<([[maybe_unused]] const struct3& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}

const ::cpp2::struct2& struct3::get_c() const& {
  return __fbthrift_field_c;
}

::cpp2::struct2 struct3::get_c() && {
  return std::move(__fbthrift_field_c);
}


void swap([[maybe_unused]] struct3& a, [[maybe_unused]] struct3& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_a, b.__fbthrift_field_a);
  swap(a.__fbthrift_field_b, b.__fbthrift_field_b);
  swap(a.__fbthrift_field_c, b.__fbthrift_field_c);
  swap(a.__isset, b.__isset);
}

template void struct3::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t struct3::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t struct3::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t struct3::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void struct3::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t struct3::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t struct3::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t struct3::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        struct3,
        ::apache::thrift::type_class::structure,
        ::cpp2::struct2>,
    "inconsistent use of json option");

} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::struct4>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::struct4>;
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

std::string_view struct4::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<struct4>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view struct4::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<struct4>::name;
}

struct4::struct4(const struct4&) = default;
struct4& struct4::operator=(const struct4&) = default;
struct4::struct4([[maybe_unused]] struct4&& other) noexcept :
    __fbthrift_field_a(std::move(other.__fbthrift_field_a)),
    __fbthrift_field_b(std::move(other.__fbthrift_field_b)),
    __fbthrift_field_c(std::move(other.__fbthrift_field_c)),
    __isset(other.__isset) {
}

struct4& struct4::operator=([[maybe_unused]] struct4&& other) noexcept {
    this->__fbthrift_field_a = std::move(other.__fbthrift_field_a);
    this->__fbthrift_field_b = std::move(other.__fbthrift_field_b);
    this->__fbthrift_field_c = std::move(other.__fbthrift_field_c);
    __isset = other.__isset;
    return *this;
}


struct4::struct4(apache::thrift::FragileConstructor, ::std::int32_t a__arg, double b__arg, ::std::int8_t c__arg) :
    __fbthrift_field_a(std::move(a__arg)),
    __fbthrift_field_b(std::move(b__arg)),
    __fbthrift_field_c(std::move(c__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
}


void struct4::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_a = ::std::int32_t();
  this->__fbthrift_field_b = double();
  this->__fbthrift_field_c = ::std::int8_t();
  __isset = {};
}

void struct4::__fbthrift_clear_terse_fields() {
}

bool struct4::__fbthrift_is_empty() const {
  return false;
}

bool struct4::operator==([[maybe_unused]] const struct4& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool struct4::operator<([[maybe_unused]] const struct4& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] struct4& a, [[maybe_unused]] struct4& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_a, b.__fbthrift_field_a);
  swap(a.__fbthrift_field_b, b.__fbthrift_field_b);
  swap(a.__fbthrift_field_c, b.__fbthrift_field_c);
  swap(a.__isset, b.__isset);
}

template void struct4::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t struct4::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t struct4::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t struct4::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void struct4::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t struct4::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t struct4::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t struct4::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::union1>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::union1>;
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

folly::Range<::cpp2::union1::Type const*> const TEnumTraits<::cpp2::union1::Type>::values = folly::range(TEnumDataStorage<::cpp2::union1::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::union1::Type>::names = folly::range(TEnumDataStorage<::cpp2::union1::Type>::names);

bool TEnumTraits<::cpp2::union1::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::union1::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view union1::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<union1>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view union1::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<union1>::name;
}

void union1::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::i:
      ::std::destroy_at(::std::addressof(value_.i));
      break;
    case Type::d:
      ::std::destroy_at(::std::addressof(value_.d));
      break;
    default:
      assert(false);
      break;
  }
}

void union1::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}


bool union1::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  union1::union1(const union1& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::d:
        set_d(rhs.value_.d);
        break;
      default:
        assert(false);
    }
  }

    union1&union1::operator=(const union1& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::d:
        set_d(rhs.value_.d);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool union1::operator==(const union1& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool union1::operator<([[maybe_unused]] const union1& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(union1& a, union1& b) {
  union1 temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void union1::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t union1::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t union1::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t union1::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void union1::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t union1::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t union1::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t union1::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // namespace cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::union2>::translateFieldName(
    std::string_view _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::union2>;
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

folly::Range<::cpp2::union2::Type const*> const TEnumTraits<::cpp2::union2::Type>::values = folly::range(TEnumDataStorage<::cpp2::union2::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::cpp2::union2::Type>::names = folly::range(TEnumDataStorage<::cpp2::union2::Type>::names);

bool TEnumTraits<::cpp2::union2::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::cpp2::union2::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace cpp2 {

std::string_view union2::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<union2>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view union2::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<union2>::name;
}

void union2::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::i:
      ::std::destroy_at(::std::addressof(value_.i));
      break;
    case Type::d:
      ::std::destroy_at(::std::addressof(value_.d));
      break;
    case Type::s:
      ::std::destroy_at(::std::addressof(value_.s));
      break;
    case Type::u:
      ::std::destroy_at(::std::addressof(value_.u));
      break;
    default:
      assert(false);
      break;
  }
}

void union2::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  union2::~union2() {
    __fbthrift_destruct();
  }

bool union2::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  union2::union2(const union2& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::d:
        set_d(rhs.value_.d);
        break;
      case Type::s:
        set_s(rhs.value_.s);
        break;
      case Type::u:
        set_u(rhs.value_.u);
        break;
      default:
        assert(false);
    }
  }

    union2&union2::operator=(const union2& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::i:
        set_i(rhs.value_.i);
        break;
      case Type::d:
        set_d(rhs.value_.d);
        break;
      case Type::s:
        set_s(rhs.value_.s);
        break;
      case Type::u:
        set_u(rhs.value_.u);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool union2::operator==(const union2& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

bool union2::operator<([[maybe_unused]] const union2& rhs) const {
  return ::apache::thrift::op::detail::UnionLessThan{}(*this, rhs);
}

void swap(union2& a, union2& b) {
  union2 temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void union2::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t union2::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t union2::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t union2::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void union2::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t union2::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t union2::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t union2::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;

static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        union2,
        ::apache::thrift::type_class::structure,
        ::cpp2::struct1>,
    "inconsistent use of json option");
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        union2,
        ::apache::thrift::type_class::variant,
        ::cpp2::union1>,
    "inconsistent use of json option");

} // namespace cpp2

namespace cpp2 { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace cpp2
namespace apache::thrift::detail::annotation {
}
