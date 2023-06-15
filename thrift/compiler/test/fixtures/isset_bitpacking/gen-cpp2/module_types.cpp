/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/isset_bitpacking/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/isset_bitpacking/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/isset_bitpacking/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/isset_bitpacking/gen-cpp2/module_data.h"


namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Default>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Default>;
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

const folly::StringPiece Default::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Default>::fields_names[folly::to_underlying(ord) - 1];
}
const folly::StringPiece Default::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Default>::name;
}

Default::Default(const Default&) = default;
Default& Default::operator=(const Default&) = default;
Default::Default(FOLLY_MAYBE_UNUSED Default&& other) noexcept :
    __fbthrift_field_field1(std::move(other.__fbthrift_field_field1)),
    __fbthrift_field_field2(std::move(other.__fbthrift_field_field2)),
    __fbthrift_field_field3(std::move(other.__fbthrift_field_field3)),
    __fbthrift_field_field4(std::move(other.__fbthrift_field_field4)),
    __isset(other.__isset) {
}

Default& Default::operator=(FOLLY_MAYBE_UNUSED Default&& other) noexcept {
    this->__fbthrift_field_field1 = std::move(other.__fbthrift_field_field1);
    this->__fbthrift_field_field2 = std::move(other.__fbthrift_field_field2);
    this->__fbthrift_field_field3 = std::move(other.__fbthrift_field_field3);
    this->__fbthrift_field_field4 = std::move(other.__fbthrift_field_field4);
    __isset = other.__isset;
    return *this;
}


Default::Default(apache::thrift::FragileConstructor, ::std::int32_t field1__arg, ::std::int32_t field2__arg, ::std::string field3__arg, double field4__arg) :
    __fbthrift_field_field1(std::move(field1__arg)),
    __fbthrift_field_field2(std::move(field2__arg)),
    __fbthrift_field_field3(std::move(field3__arg)),
    __fbthrift_field_field4(std::move(field4__arg)) {
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void Default::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_field1 = ::std::int32_t();
  this->__fbthrift_field_field2 = ::std::int32_t();
  this->__fbthrift_field_field3 = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_field4 = double();
  __isset = {};
}

void Default::__fbthrift_clear_terse_fields() {
}

bool Default::__fbthrift_is_empty() const {
  return !(this->__isset.get(0)) &&
 !(this->__isset.get(1)) &&
 !(this->__isset.get(2)) &&
 !(this->__isset.get(3));
}

bool Default::operator==(FOLLY_MAYBE_UNUSED const Default& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return false;
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return false;
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return false;
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return false;
  }
  return true;
}

bool Default::operator<(FOLLY_MAYBE_UNUSED const Default& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return lhs.field1_ref() < rhs.field1_ref();
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return lhs.field2_ref() < rhs.field2_ref();
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return lhs.field3_ref() < rhs.field3_ref();
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return lhs.field4_ref() < rhs.field4_ref();
  }
  return false;
}


void swap(FOLLY_MAYBE_UNUSED Default& a, FOLLY_MAYBE_UNUSED Default& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_field1, b.__fbthrift_field_field1);
  swap(a.__fbthrift_field_field2, b.__fbthrift_field_field2);
  swap(a.__fbthrift_field_field3, b.__fbthrift_field_field3);
  swap(a.__fbthrift_field_field4, b.__fbthrift_field_field4);
  swap(a.__isset, b.__isset);
}

template void Default::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Default::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Default::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Default::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Default::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Default::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Default::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Default::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::NonAtomic>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::NonAtomic>;
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

const folly::StringPiece NonAtomic::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<NonAtomic>::fields_names[folly::to_underlying(ord) - 1];
}
const folly::StringPiece NonAtomic::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<NonAtomic>::name;
}

NonAtomic::NonAtomic(const NonAtomic&) = default;
NonAtomic& NonAtomic::operator=(const NonAtomic&) = default;
NonAtomic::NonAtomic(FOLLY_MAYBE_UNUSED NonAtomic&& other) noexcept :
    __fbthrift_field_field1(std::move(other.__fbthrift_field_field1)),
    __fbthrift_field_field2(std::move(other.__fbthrift_field_field2)),
    __fbthrift_field_field3(std::move(other.__fbthrift_field_field3)),
    __fbthrift_field_field4(std::move(other.__fbthrift_field_field4)),
    __isset(other.__isset) {
}

NonAtomic& NonAtomic::operator=(FOLLY_MAYBE_UNUSED NonAtomic&& other) noexcept {
    this->__fbthrift_field_field1 = std::move(other.__fbthrift_field_field1);
    this->__fbthrift_field_field2 = std::move(other.__fbthrift_field_field2);
    this->__fbthrift_field_field3 = std::move(other.__fbthrift_field_field3);
    this->__fbthrift_field_field4 = std::move(other.__fbthrift_field_field4);
    __isset = other.__isset;
    return *this;
}


NonAtomic::NonAtomic(apache::thrift::FragileConstructor, ::std::int32_t field1__arg, ::std::int32_t field2__arg, ::std::string field3__arg, double field4__arg) :
    __fbthrift_field_field1(std::move(field1__arg)),
    __fbthrift_field_field2(std::move(field2__arg)),
    __fbthrift_field_field3(std::move(field3__arg)),
    __fbthrift_field_field4(std::move(field4__arg)) {
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void NonAtomic::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_field1 = ::std::int32_t();
  this->__fbthrift_field_field2 = ::std::int32_t();
  this->__fbthrift_field_field3 = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_field4 = double();
  __isset = {};
}

void NonAtomic::__fbthrift_clear_terse_fields() {
}

bool NonAtomic::__fbthrift_is_empty() const {
  return !(this->__isset.get(0)) &&
 !(this->__isset.get(1)) &&
 !(this->__isset.get(2)) &&
 !(this->__isset.get(3));
}

bool NonAtomic::operator==(FOLLY_MAYBE_UNUSED const NonAtomic& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return false;
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return false;
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return false;
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return false;
  }
  return true;
}

bool NonAtomic::operator<(FOLLY_MAYBE_UNUSED const NonAtomic& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return lhs.field1_ref() < rhs.field1_ref();
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return lhs.field2_ref() < rhs.field2_ref();
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return lhs.field3_ref() < rhs.field3_ref();
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return lhs.field4_ref() < rhs.field4_ref();
  }
  return false;
}


void swap(FOLLY_MAYBE_UNUSED NonAtomic& a, FOLLY_MAYBE_UNUSED NonAtomic& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_field1, b.__fbthrift_field_field1);
  swap(a.__fbthrift_field_field2, b.__fbthrift_field_field2);
  swap(a.__fbthrift_field_field3, b.__fbthrift_field_field3);
  swap(a.__fbthrift_field_field4, b.__fbthrift_field_field4);
  swap(a.__isset, b.__isset);
}

template void NonAtomic::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t NonAtomic::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t NonAtomic::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t NonAtomic::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void NonAtomic::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t NonAtomic::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t NonAtomic::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t NonAtomic::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::Atomic>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::Atomic>;
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

const folly::StringPiece Atomic::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<Atomic>::fields_names[folly::to_underlying(ord) - 1];
}
const folly::StringPiece Atomic::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<Atomic>::name;
}

Atomic::Atomic(const Atomic&) = default;
Atomic& Atomic::operator=(const Atomic&) = default;
Atomic::Atomic(FOLLY_MAYBE_UNUSED Atomic&& other) noexcept :
    __fbthrift_field_field1(std::move(other.__fbthrift_field_field1)),
    __fbthrift_field_field2(std::move(other.__fbthrift_field_field2)),
    __fbthrift_field_field3(std::move(other.__fbthrift_field_field3)),
    __fbthrift_field_field4(std::move(other.__fbthrift_field_field4)),
    __isset(other.__isset) {
}

Atomic& Atomic::operator=(FOLLY_MAYBE_UNUSED Atomic&& other) noexcept {
    this->__fbthrift_field_field1 = std::move(other.__fbthrift_field_field1);
    this->__fbthrift_field_field2 = std::move(other.__fbthrift_field_field2);
    this->__fbthrift_field_field3 = std::move(other.__fbthrift_field_field3);
    this->__fbthrift_field_field4 = std::move(other.__fbthrift_field_field4);
    __isset = other.__isset;
    return *this;
}


Atomic::Atomic(apache::thrift::FragileConstructor, ::std::int32_t field1__arg, ::std::int32_t field2__arg, ::std::string field3__arg, double field4__arg) :
    __fbthrift_field_field1(std::move(field1__arg)),
    __fbthrift_field_field2(std::move(field2__arg)),
    __fbthrift_field_field3(std::move(field3__arg)),
    __fbthrift_field_field4(std::move(field4__arg)) {
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void Atomic::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_field1 = ::std::int32_t();
  this->__fbthrift_field_field2 = ::std::int32_t();
  this->__fbthrift_field_field3 = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_field4 = double();
  __isset = {};
}

void Atomic::__fbthrift_clear_terse_fields() {
}

bool Atomic::__fbthrift_is_empty() const {
  return !(this->__isset.get(0)) &&
 !(this->__isset.get(1)) &&
 !(this->__isset.get(2)) &&
 !(this->__isset.get(3));
}

bool Atomic::operator==(FOLLY_MAYBE_UNUSED const Atomic& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return false;
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return false;
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return false;
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return false;
  }
  return true;
}

bool Atomic::operator<(FOLLY_MAYBE_UNUSED const Atomic& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return lhs.field1_ref() < rhs.field1_ref();
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return lhs.field2_ref() < rhs.field2_ref();
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return lhs.field3_ref() < rhs.field3_ref();
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return lhs.field4_ref() < rhs.field4_ref();
  }
  return false;
}


void swap(FOLLY_MAYBE_UNUSED Atomic& a, FOLLY_MAYBE_UNUSED Atomic& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_field1, b.__fbthrift_field_field1);
  swap(a.__fbthrift_field_field2, b.__fbthrift_field_field2);
  swap(a.__fbthrift_field_field3, b.__fbthrift_field_field3);
  swap(a.__fbthrift_field_field4, b.__fbthrift_field_field4);
  swap(a.__isset, b.__isset);
}

template void Atomic::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t Atomic::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t Atomic::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t Atomic::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void Atomic::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t Atomic::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t Atomic::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t Atomic::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // cpp2

namespace apache {
namespace thrift {
namespace detail {

void TccStructTraits<::cpp2::AtomicFoo>::translateFieldName(
    folly::StringPiece _fname,
    int16_t& fid,
    apache::thrift::protocol::TType& _ftype) noexcept {
  using data = apache::thrift::TStructDataStorage<::cpp2::AtomicFoo>;
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

const folly::StringPiece AtomicFoo::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<AtomicFoo>::fields_names[folly::to_underlying(ord) - 1];
}
const folly::StringPiece AtomicFoo::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<AtomicFoo>::name;
}

AtomicFoo::AtomicFoo(const AtomicFoo&) = default;
AtomicFoo& AtomicFoo::operator=(const AtomicFoo&) = default;
AtomicFoo::AtomicFoo(FOLLY_MAYBE_UNUSED AtomicFoo&& other) noexcept :
    __fbthrift_field_field1(std::move(other.__fbthrift_field_field1)),
    __fbthrift_field_field2(std::move(other.__fbthrift_field_field2)),
    __fbthrift_field_field3(std::move(other.__fbthrift_field_field3)),
    __fbthrift_field_field4(std::move(other.__fbthrift_field_field4)),
    __isset(other.__isset) {
}

AtomicFoo& AtomicFoo::operator=(FOLLY_MAYBE_UNUSED AtomicFoo&& other) noexcept {
    this->__fbthrift_field_field1 = std::move(other.__fbthrift_field_field1);
    this->__fbthrift_field_field2 = std::move(other.__fbthrift_field_field2);
    this->__fbthrift_field_field3 = std::move(other.__fbthrift_field_field3);
    this->__fbthrift_field_field4 = std::move(other.__fbthrift_field_field4);
    __isset = other.__isset;
    return *this;
}


AtomicFoo::AtomicFoo(apache::thrift::FragileConstructor, ::std::int32_t field1__arg, ::std::int32_t field2__arg, ::std::string field3__arg, double field4__arg) :
    __fbthrift_field_field1(std::move(field1__arg)),
    __fbthrift_field_field2(std::move(field2__arg)),
    __fbthrift_field_field3(std::move(field3__arg)),
    __fbthrift_field_field4(std::move(field4__arg)) {
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
}


void AtomicFoo::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_field1 = ::std::int32_t();
  this->__fbthrift_field_field2 = ::std::int32_t();
  this->__fbthrift_field_field3 = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_field4 = double();
  __isset = {};
}

void AtomicFoo::__fbthrift_clear_terse_fields() {
}

bool AtomicFoo::__fbthrift_is_empty() const {
  return !(this->__isset.get(0)) &&
 !(this->__isset.get(1)) &&
 !(this->__isset.get(2)) &&
 !(this->__isset.get(3));
}

bool AtomicFoo::operator==(FOLLY_MAYBE_UNUSED const AtomicFoo& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return false;
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return false;
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return false;
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return false;
  }
  return true;
}

bool AtomicFoo::operator<(FOLLY_MAYBE_UNUSED const AtomicFoo& rhs) const {
  FOLLY_MAYBE_UNUSED auto& lhs = *this;
  if (!(lhs.field1_ref() == rhs.field1_ref())) {
    return lhs.field1_ref() < rhs.field1_ref();
  }
  if (!(lhs.field2_ref() == rhs.field2_ref())) {
    return lhs.field2_ref() < rhs.field2_ref();
  }
  if (!(lhs.field3_ref() == rhs.field3_ref())) {
    return lhs.field3_ref() < rhs.field3_ref();
  }
  if (!(lhs.field4_ref() == rhs.field4_ref())) {
    return lhs.field4_ref() < rhs.field4_ref();
  }
  return false;
}


void swap(FOLLY_MAYBE_UNUSED AtomicFoo& a, FOLLY_MAYBE_UNUSED AtomicFoo& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_field1, b.__fbthrift_field_field1);
  swap(a.__fbthrift_field_field2, b.__fbthrift_field_field2);
  swap(a.__fbthrift_field_field3, b.__fbthrift_field_field3);
  swap(a.__fbthrift_field_field4, b.__fbthrift_field_field4);
  swap(a.__isset, b.__isset);
}

template void AtomicFoo::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t AtomicFoo::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t AtomicFoo::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t AtomicFoo::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void AtomicFoo::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t AtomicFoo::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t AtomicFoo::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t AtomicFoo::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;


} // cpp2

namespace cpp2 { namespace {
FOLLY_MAYBE_UNUSED FOLLY_ERASE void validateAdapters() {
}
}} // cpp2
