/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/tablebased/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#include "thrift/compiler/test/fixtures/tablebased/gen-cpp2/module_types.h"
#include "thrift/compiler/test/fixtures/tablebased/gen-cpp2/module_types.tcc"

#include <thrift/lib/cpp2/gen/module_types_cpp.h>

#include "thrift/compiler/test/fixtures/tablebased/gen-cpp2/module_data.h"


namespace apache { namespace thrift {

const std::string_view TEnumTraits<::test::fixtures::tablebased::ExampleEnum>::type_name = TEnumDataStorage<::test::fixtures::tablebased::ExampleEnum>::type_name;
folly::Range<::test::fixtures::tablebased::ExampleEnum const*> const TEnumTraits<::test::fixtures::tablebased::ExampleEnum>::values = folly::range(TEnumDataStorage<::test::fixtures::tablebased::ExampleEnum>::values);
folly::Range<std::string_view const*> const TEnumTraits<::test::fixtures::tablebased::ExampleEnum>::names = folly::range(TEnumDataStorage<::test::fixtures::tablebased::ExampleEnum>::names);

bool TEnumTraits<::test::fixtures::tablebased::ExampleEnum>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::test::fixtures::tablebased::ExampleEnum>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}

}} // apache::thrift

namespace apache {
namespace thrift {
namespace detail {
FOLLY_PUSH_WARNING
FOLLY_GNU_DISABLE_WARNING("-Winvalid-offsetof")
template<>
constexpr ptrdiff_t fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(std::int16_t fieldIndex) {
  constexpr ptrdiff_t offsets[] = {
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __fbthrift_field_fieldA),
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __fbthrift_field_fieldB),
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __fbthrift_field_fieldC),
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __fbthrift_field_fieldD),
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __fbthrift_field_fieldE)};
  return offsets[fieldIndex];
}

template<>
constexpr ptrdiff_t issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(std::int16_t fieldIndex) {
  constexpr ptrdiff_t offsets[] = {
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __isset) + isset_bitset<5>::get_offset() + 0,
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __isset) + isset_bitset<5>::get_offset() + 1,
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __isset) + isset_bitset<5>::get_offset() + 2,
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __isset) + isset_bitset<5>::get_offset() + 3,
    offsetof(::test::fixtures::tablebased::TrivialTypesStruct, __isset) + isset_bitset<5>::get_offset() + 4};
  return offsets[fieldIndex];
}

template<>
constexpr ptrdiff_t fieldOffset<::test::fixtures::tablebased::ContainerStruct>(std::int16_t fieldIndex) {
  constexpr ptrdiff_t offsets[] = {
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldB),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldC),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldD),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldE),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldF),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldG),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldH),
    offsetof(::test::fixtures::tablebased::ContainerStruct, __fbthrift_field_fieldA)};
  return offsets[fieldIndex];
}

template<>
constexpr ptrdiff_t issetOffset<::test::fixtures::tablebased::ContainerStruct>(std::int16_t fieldIndex) {
  constexpr ptrdiff_t offsets[] = {
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 1,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 2,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 3,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 4,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 5,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 6,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 7,
    offsetof(::test::fixtures::tablebased::ContainerStruct, __isset) + isset_bitset<8>::get_offset() + 0};
  return offsets[fieldIndex];
}


template<>
constexpr ptrdiff_t unionTypeOffset<::test::fixtures::tablebased::ExampleUnion>() {
  return offsetof(::test::fixtures::tablebased::ExampleUnion, type_);
}
FOLLY_POP_WARNING
}}} // apache::thrift::detail

namespace test::fixtures::tablebased {

std::string_view TrivialTypesStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<TrivialTypesStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view TrivialTypesStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<TrivialTypesStruct>::name;
}

TrivialTypesStruct::TrivialTypesStruct(const TrivialTypesStruct& srcObj) :
    __fbthrift_field_fieldA(srcObj.__fbthrift_field_fieldA),
    __fbthrift_field_fieldB(srcObj.__fbthrift_field_fieldB),
    __fbthrift_field_fieldC(srcObj.__fbthrift_field_fieldC),
    __fbthrift_field_fieldD(::apache::thrift::detail::st::copy_field<
          ::apache::thrift::type_class::binary>(srcObj.__fbthrift_field_fieldD)),
    __fbthrift_field_fieldE(srcObj.__fbthrift_field_fieldE),
    __isset(srcObj.__isset) {
}

static void __fbthrift_swap(TrivialTypesStruct& lhs, TrivialTypesStruct& rhs) { swap(lhs, rhs); }
TrivialTypesStruct& TrivialTypesStruct::operator=(const TrivialTypesStruct& other) {
  TrivialTypesStruct tmp(other);
  __fbthrift_swap(*this, tmp);
  return *this;
}

TrivialTypesStruct::TrivialTypesStruct() :
    __fbthrift_field_fieldA(),
    __fbthrift_field_fieldE() {
}


TrivialTypesStruct::~TrivialTypesStruct() {}

TrivialTypesStruct::TrivialTypesStruct([[maybe_unused]] TrivialTypesStruct&& other) noexcept :
    __fbthrift_field_fieldA(std::move(other.__fbthrift_field_fieldA)),
    __fbthrift_field_fieldB(std::move(other.__fbthrift_field_fieldB)),
    __fbthrift_field_fieldC(std::move(other.__fbthrift_field_fieldC)),
    __fbthrift_field_fieldD(std::move(other.__fbthrift_field_fieldD)),
    __fbthrift_field_fieldE(std::move(other.__fbthrift_field_fieldE)),
    __isset(other.__isset) {
}

TrivialTypesStruct& TrivialTypesStruct::operator=([[maybe_unused]] TrivialTypesStruct&& other) noexcept {
    this->__fbthrift_field_fieldA = std::move(other.__fbthrift_field_fieldA);
    this->__fbthrift_field_fieldB = std::move(other.__fbthrift_field_fieldB);
    this->__fbthrift_field_fieldC = std::move(other.__fbthrift_field_fieldC);
    this->__fbthrift_field_fieldD = std::move(other.__fbthrift_field_fieldD);
    this->__fbthrift_field_fieldE = std::move(other.__fbthrift_field_fieldE);
    __isset = other.__isset;
    return *this;
}


TrivialTypesStruct::TrivialTypesStruct(apache::thrift::FragileConstructor, ::std::int32_t fieldA__arg, ::std::string fieldB__arg, ::std::string fieldC__arg, ::test::fixtures::tablebased::IOBufPtr fieldD__arg, ::test::fixtures::tablebased::ExampleEnum fieldE__arg) :
    __fbthrift_field_fieldA(std::move(fieldA__arg)),
    __fbthrift_field_fieldB(std::move(fieldB__arg)),
    __fbthrift_field_fieldC(std::move(fieldC__arg)),
    __fbthrift_field_fieldD(std::move(fieldD__arg)),
    __fbthrift_field_fieldE(std::move(fieldE__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
}


void TrivialTypesStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_fieldA = ::std::int32_t();
  this->__fbthrift_field_fieldB = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_fieldC = apache::thrift::StringTraits<std::string>::fromStringLiteral("");
  this->__fbthrift_field_fieldD = apache::thrift::StringTraits<std::unique_ptr<folly::IOBuf>>::fromStringLiteral("");
  this->__fbthrift_field_fieldE = ::test::fixtures::tablebased::ExampleEnum();
  __isset = {};
}

void TrivialTypesStruct::__fbthrift_clear_terse_fields() {
}

bool TrivialTypesStruct::__fbthrift_is_empty() const {
  return false;
}

bool TrivialTypesStruct::operator==([[maybe_unused]] const TrivialTypesStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

bool TrivialTypesStruct::operator<([[maybe_unused]] const TrivialTypesStruct& rhs) const {
  return ::apache::thrift::op::detail::StructLessThan{}(*this, rhs);
}


void swap([[maybe_unused]] TrivialTypesStruct& a, [[maybe_unused]] TrivialTypesStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_fieldA, b.__fbthrift_field_fieldA);
  swap(a.__fbthrift_field_fieldB, b.__fbthrift_field_fieldB);
  swap(a.__fbthrift_field_fieldC, b.__fbthrift_field_fieldC);
  swap(a.__fbthrift_field_fieldD, b.__fbthrift_field_fieldD);
  swap(a.__fbthrift_field_fieldE, b.__fbthrift_field_fieldE);
  swap(a.__isset, b.__isset);
}

template void TrivialTypesStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t TrivialTypesStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t TrivialTypesStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t TrivialTypesStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void TrivialTypesStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t TrivialTypesStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t TrivialTypesStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t TrivialTypesStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;
template void TrivialTypesStruct::readNoXfer<>(apache::thrift::SimpleJSONProtocolReader*);
template uint32_t TrivialTypesStruct::write<>(apache::thrift::SimpleJSONProtocolWriter*) const;
template uint32_t TrivialTypesStruct::serializedSize<>(apache::thrift::SimpleJSONProtocolWriter const*) const;
template uint32_t TrivialTypesStruct::serializedSizeZC<>(apache::thrift::SimpleJSONProtocolWriter const*) const;

constexpr ::apache::thrift::detail::StructInfoN<5> __fbthrift_struct_info_TrivialTypesStruct = {
  /* .numFields */ 5,
  /* .name */ "TrivialTypesStruct",
  /* .unionExt */ nullptr,
  /* .getIsset */ nullptr,
  /* .setIsset */ nullptr,
  /* .getFieldValuesBasePtr */ nullptr,
  /* .customExt */ nullptr,
  /* .fieldInfos */ {
  {
    /* .id */ 1,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Optional,
    /* .name */ "fieldA",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(0),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(0),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::integral, ::std::int32_t>::typeInfo,
  },
  {
    /* .id */ 2,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Optional,
    /* .name */ "fieldB",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(1),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(1),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::string, ::std::string>::typeInfo,
  },
  {
    /* .id */ 3,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Optional,
    /* .name */ "fieldC",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(2),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(2),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::binary, ::std::string>::typeInfo,
  },
  {
    /* .id */ 4,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Optional,
    /* .name */ "fieldD",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(3),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(3),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::binary, ::test::fixtures::tablebased::IOBufPtr>::typeInfo,
  },
  {
    /* .id */ 5,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldE",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::TrivialTypesStruct>(4),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::TrivialTypesStruct>(4),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::enumeration, ::test::fixtures::tablebased::ExampleEnum>::typeInfo,
  }}
};

} // namespace test::fixtures::tablebased

namespace test::fixtures::tablebased {

std::string_view ContainerStruct::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<ContainerStruct>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view ContainerStruct::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<ContainerStruct>::name;
}

ContainerStruct::ContainerStruct(const ContainerStruct&) = default;
ContainerStruct& ContainerStruct::operator=(const ContainerStruct&) = default;
ContainerStruct::ContainerStruct() {
}


ContainerStruct::~ContainerStruct() {}

ContainerStruct::ContainerStruct([[maybe_unused]] ContainerStruct&& other) noexcept :
    __fbthrift_field_fieldA(std::move(other.__fbthrift_field_fieldA)),
    __fbthrift_field_fieldB(std::move(other.__fbthrift_field_fieldB)),
    __fbthrift_field_fieldC(std::move(other.__fbthrift_field_fieldC)),
    __fbthrift_field_fieldD(std::move(other.__fbthrift_field_fieldD)),
    __fbthrift_field_fieldE(std::move(other.__fbthrift_field_fieldE)),
    __fbthrift_field_fieldF(std::move(other.__fbthrift_field_fieldF)),
    __fbthrift_field_fieldG(std::move(other.__fbthrift_field_fieldG)),
    __fbthrift_field_fieldH(std::move(other.__fbthrift_field_fieldH)),
    __isset(other.__isset) {
}

ContainerStruct& ContainerStruct::operator=([[maybe_unused]] ContainerStruct&& other) noexcept {
    this->__fbthrift_field_fieldA = std::move(other.__fbthrift_field_fieldA);
    this->__fbthrift_field_fieldB = std::move(other.__fbthrift_field_fieldB);
    this->__fbthrift_field_fieldC = std::move(other.__fbthrift_field_fieldC);
    this->__fbthrift_field_fieldD = std::move(other.__fbthrift_field_fieldD);
    this->__fbthrift_field_fieldE = std::move(other.__fbthrift_field_fieldE);
    this->__fbthrift_field_fieldF = std::move(other.__fbthrift_field_fieldF);
    this->__fbthrift_field_fieldG = std::move(other.__fbthrift_field_fieldG);
    this->__fbthrift_field_fieldH = std::move(other.__fbthrift_field_fieldH);
    __isset = other.__isset;
    return *this;
}


ContainerStruct::ContainerStruct(apache::thrift::FragileConstructor, ::std::vector<::std::int32_t> fieldA__arg, std::list<::std::int32_t> fieldB__arg, std::deque<::std::int32_t> fieldC__arg, folly::fbvector<::std::int32_t> fieldD__arg, folly::small_vector<::std::int32_t> fieldE__arg, folly::sorted_vector_set<::std::int32_t> fieldF__arg, folly::sorted_vector_map<::std::int32_t, ::std::string> fieldG__arg, ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct> fieldH__arg) :
    __fbthrift_field_fieldA(std::move(fieldA__arg)),
    __fbthrift_field_fieldB(std::move(fieldB__arg)),
    __fbthrift_field_fieldC(std::move(fieldC__arg)),
    __fbthrift_field_fieldD(std::move(fieldD__arg)),
    __fbthrift_field_fieldE(std::move(fieldE__arg)),
    __fbthrift_field_fieldF(std::move(fieldF__arg)),
    __fbthrift_field_fieldG(std::move(fieldG__arg)),
    __fbthrift_field_fieldH(std::move(fieldH__arg)) { 
  __isset.set(folly::index_constant<0>(), true);
  __isset.set(folly::index_constant<1>(), true);
  __isset.set(folly::index_constant<2>(), true);
  __isset.set(folly::index_constant<3>(), true);
  __isset.set(folly::index_constant<4>(), true);
  __isset.set(folly::index_constant<5>(), true);
  __isset.set(folly::index_constant<6>(), true);
  __isset.set(folly::index_constant<7>(), true);
}


void ContainerStruct::__fbthrift_clear() {
  // clear all fields
  this->__fbthrift_field_fieldA.clear();
  this->__fbthrift_field_fieldB.clear();
  this->__fbthrift_field_fieldC.clear();
  this->__fbthrift_field_fieldD.clear();
  this->__fbthrift_field_fieldE.clear();
  this->__fbthrift_field_fieldF.clear();
  this->__fbthrift_field_fieldG.clear();
  this->__fbthrift_field_fieldH.clear();
  __isset = {};
}

void ContainerStruct::__fbthrift_clear_terse_fields() {
}

bool ContainerStruct::__fbthrift_is_empty() const {
  return false;
}

bool ContainerStruct::operator==([[maybe_unused]] const ContainerStruct& rhs) const {
  return ::apache::thrift::op::detail::StructEquality{}(*this, rhs);
}

const ::std::vector<::std::int32_t>& ContainerStruct::get_fieldA() const& {
  return __fbthrift_field_fieldA;
}

::std::vector<::std::int32_t> ContainerStruct::get_fieldA() && {
  return std::move(__fbthrift_field_fieldA);
}

const std::list<::std::int32_t>& ContainerStruct::get_fieldB() const& {
  return __fbthrift_field_fieldB;
}

std::list<::std::int32_t> ContainerStruct::get_fieldB() && {
  return std::move(__fbthrift_field_fieldB);
}

const std::deque<::std::int32_t>& ContainerStruct::get_fieldC() const& {
  return __fbthrift_field_fieldC;
}

std::deque<::std::int32_t> ContainerStruct::get_fieldC() && {
  return std::move(__fbthrift_field_fieldC);
}

const folly::fbvector<::std::int32_t>& ContainerStruct::get_fieldD() const& {
  return __fbthrift_field_fieldD;
}

folly::fbvector<::std::int32_t> ContainerStruct::get_fieldD() && {
  return std::move(__fbthrift_field_fieldD);
}

const folly::small_vector<::std::int32_t>& ContainerStruct::get_fieldE() const& {
  return __fbthrift_field_fieldE;
}

folly::small_vector<::std::int32_t> ContainerStruct::get_fieldE() && {
  return std::move(__fbthrift_field_fieldE);
}

const folly::sorted_vector_set<::std::int32_t>& ContainerStruct::get_fieldF() const& {
  return __fbthrift_field_fieldF;
}

folly::sorted_vector_set<::std::int32_t> ContainerStruct::get_fieldF() && {
  return std::move(__fbthrift_field_fieldF);
}

const folly::sorted_vector_map<::std::int32_t, ::std::string>& ContainerStruct::get_fieldG() const& {
  return __fbthrift_field_fieldG;
}

folly::sorted_vector_map<::std::int32_t, ::std::string> ContainerStruct::get_fieldG() && {
  return std::move(__fbthrift_field_fieldG);
}

const ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>& ContainerStruct::get_fieldH() const& {
  return __fbthrift_field_fieldH;
}

::std::vector<::test::fixtures::tablebased::TrivialTypesStruct> ContainerStruct::get_fieldH() && {
  return std::move(__fbthrift_field_fieldH);
}


void swap([[maybe_unused]] ContainerStruct& a, [[maybe_unused]] ContainerStruct& b) {
  using ::std::swap;
  swap(a.__fbthrift_field_fieldA, b.__fbthrift_field_fieldA);
  swap(a.__fbthrift_field_fieldB, b.__fbthrift_field_fieldB);
  swap(a.__fbthrift_field_fieldC, b.__fbthrift_field_fieldC);
  swap(a.__fbthrift_field_fieldD, b.__fbthrift_field_fieldD);
  swap(a.__fbthrift_field_fieldE, b.__fbthrift_field_fieldE);
  swap(a.__fbthrift_field_fieldF, b.__fbthrift_field_fieldF);
  swap(a.__fbthrift_field_fieldG, b.__fbthrift_field_fieldG);
  swap(a.__fbthrift_field_fieldH, b.__fbthrift_field_fieldH);
  swap(a.__isset, b.__isset);
}

template void ContainerStruct::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t ContainerStruct::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t ContainerStruct::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t ContainerStruct::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void ContainerStruct::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t ContainerStruct::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t ContainerStruct::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t ContainerStruct::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;
template void ContainerStruct::readNoXfer<>(apache::thrift::SimpleJSONProtocolReader*);
template uint32_t ContainerStruct::write<>(apache::thrift::SimpleJSONProtocolWriter*) const;
template uint32_t ContainerStruct::serializedSize<>(apache::thrift::SimpleJSONProtocolWriter const*) const;
template uint32_t ContainerStruct::serializedSizeZC<>(apache::thrift::SimpleJSONProtocolWriter const*) const;

constexpr ::apache::thrift::detail::StructInfoN<8> __fbthrift_struct_info_ContainerStruct = {
  /* .numFields */ 8,
  /* .name */ "ContainerStruct",
  /* .unionExt */ nullptr,
  /* .getIsset */ nullptr,
  /* .setIsset */ nullptr,
  /* .getFieldValuesBasePtr */ nullptr,
  /* .customExt */ nullptr,
  /* .fieldInfos */ {
  {
    /* .id */ 2,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldB",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(0),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(0),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, std::list<::std::int32_t>>::typeInfo,
  },
  {
    /* .id */ 3,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldC",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(1),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(1),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, std::deque<::std::int32_t>>::typeInfo,
  },
  {
    /* .id */ 4,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldD",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(2),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(2),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, folly::fbvector<::std::int32_t>>::typeInfo,
  },
  {
    /* .id */ 5,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldE",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(3),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(3),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, folly::small_vector<::std::int32_t>>::typeInfo,
  },
  {
    /* .id */ 6,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldF",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(4),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(4),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::set<::apache::thrift::type_class::integral>, folly::sorted_vector_set<::std::int32_t>>::typeInfo,
  },
  {
    /* .id */ 7,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldG",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(5),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(5),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::map<::apache::thrift::type_class::integral, ::apache::thrift::type_class::string>, folly::sorted_vector_map<::std::int32_t, ::std::string>>::typeInfo,
  },
  {
    /* .id */ 8,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldH",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(6),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(6),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::structure>, ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>::typeInfo,
  },
  {
    /* .id */ 12,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldA",
    /* .memberOffset */ ::apache::thrift::detail::fieldOffset<::test::fixtures::tablebased::ContainerStruct>(7),
    /* .issetOffset */ ::apache::thrift::detail::issetOffset<::test::fixtures::tablebased::ContainerStruct>(7),
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, ::std::vector<::std::int32_t>>::typeInfo,
  }}
};
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        ContainerStruct,
        ::apache::thrift::type_class::list<::apache::thrift::type_class::structure>,
        ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>,
    "inconsistent use of json option");

} // namespace test::fixtures::tablebased

namespace apache { namespace thrift {

folly::Range<::test::fixtures::tablebased::ExampleUnion::Type const*> const TEnumTraits<::test::fixtures::tablebased::ExampleUnion::Type>::values = folly::range(TEnumDataStorage<::test::fixtures::tablebased::ExampleUnion::Type>::values);
folly::Range<std::string_view const*> const TEnumTraits<::test::fixtures::tablebased::ExampleUnion::Type>::names = folly::range(TEnumDataStorage<::test::fixtures::tablebased::ExampleUnion::Type>::names);

bool TEnumTraits<::test::fixtures::tablebased::ExampleUnion::Type>::findName(type value, std::string_view* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_name(value, out);
}

bool TEnumTraits<::test::fixtures::tablebased::ExampleUnion::Type>::findValue(std::string_view name, type* out) noexcept {
  return ::apache::thrift::detail::st::enum_find_value(name, out);
}
}} // apache::thrift
namespace test::fixtures::tablebased {

std::string_view ExampleUnion::__fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord) {
  if (ord == ::apache::thrift::FieldOrdinal{0}) { return {}; }
  return apache::thrift::TStructDataStorage<ExampleUnion>::fields_names[folly::to_underlying(ord) - 1];
}
std::string_view ExampleUnion::__fbthrift_get_class_name() {
  return apache::thrift::TStructDataStorage<ExampleUnion>::name;
}

void ExampleUnion::__fbthrift_destruct() {
  switch(getType()) {
    case Type::__EMPTY__:
      break;
    case Type::fieldA:
      ::std::destroy_at(::std::addressof(value_.fieldA));
      break;
    case Type::fieldB:
      ::std::destroy_at(::std::addressof(value_.fieldB));
      break;
    default:
      assert(false);
      break;
  }
}

void ExampleUnion::__fbthrift_clear() {
  __fbthrift_destruct();
  type_ = folly::to_underlying(Type::__EMPTY__);
}

  ExampleUnion::~ExampleUnion() {
    __fbthrift_destruct();
  }

bool ExampleUnion::__fbthrift_is_empty() const {
  return getType() == Type::__EMPTY__;
}
  ExampleUnion::ExampleUnion(const ExampleUnion& rhs)
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        return;
      case Type::fieldA:
        set_fieldA(rhs.value_.fieldA);
        break;
      case Type::fieldB:
        set_fieldB(rhs.value_.fieldB);
        break;
      default:
        assert(false);
    }
  }

    ExampleUnion&ExampleUnion::operator=(const ExampleUnion& rhs) {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
        __fbthrift_clear();
        return *this;
      case Type::fieldA:
        set_fieldA(rhs.value_.fieldA);
        break;
      case Type::fieldB:
        set_fieldB(rhs.value_.fieldB);
        break;
      default:
        __fbthrift_clear();
        assert(false);
    }
    return *this;
  }


bool ExampleUnion::operator==(const ExampleUnion& rhs) const {
  return ::apache::thrift::op::detail::UnionEquality{}(*this, rhs);
}

void swap(ExampleUnion& a, ExampleUnion& b) {
  ExampleUnion temp(std::move(a));
  a = std::move(b);
  b = std::move(temp);
}

template void ExampleUnion::readNoXfer<>(apache::thrift::BinaryProtocolReader*);
template uint32_t ExampleUnion::write<>(apache::thrift::BinaryProtocolWriter*) const;
template uint32_t ExampleUnion::serializedSize<>(apache::thrift::BinaryProtocolWriter const*) const;
template uint32_t ExampleUnion::serializedSizeZC<>(apache::thrift::BinaryProtocolWriter const*) const;
template void ExampleUnion::readNoXfer<>(apache::thrift::CompactProtocolReader*);
template uint32_t ExampleUnion::write<>(apache::thrift::CompactProtocolWriter*) const;
template uint32_t ExampleUnion::serializedSize<>(apache::thrift::CompactProtocolWriter const*) const;
template uint32_t ExampleUnion::serializedSizeZC<>(apache::thrift::CompactProtocolWriter const*) const;
template void ExampleUnion::readNoXfer<>(apache::thrift::SimpleJSONProtocolReader*);
template uint32_t ExampleUnion::write<>(apache::thrift::SimpleJSONProtocolWriter*) const;
template uint32_t ExampleUnion::serializedSize<>(apache::thrift::SimpleJSONProtocolWriter const*) const;
template uint32_t ExampleUnion::serializedSizeZC<>(apache::thrift::SimpleJSONProtocolWriter const*) const;

constexpr ::apache::thrift::detail::UnionExtN<2> ExampleUnion_unionExt = {
  /* .clear */ ::apache::thrift::detail::clearUnion<::test::fixtures::tablebased::ExampleUnion>,
  /* .unionTypeOffset */ ::apache::thrift::detail::unionTypeOffset<::test::fixtures::tablebased::ExampleUnion>(),
  /* .getActiveId */ nullptr,
  /* .setActiveId */ nullptr,
  /* .initMember */ {
  ::apache::thrift::detail::placementNewUnionValue<::test::fixtures::tablebased::ContainerStruct>,
::apache::thrift::detail::placementNewUnionValue<::test::fixtures::tablebased::TrivialTypesStruct>},
};
constexpr ::apache::thrift::detail::StructInfoN<2> __fbthrift_struct_info_ExampleUnion = {
  /* .numFields */ 2,
  /* .name */ "ExampleUnion",
  /* .unionExt */ &ExampleUnion_unionExt,
  /* .getIsset */ nullptr,
  /* .setIsset */ nullptr,
  /* .getFieldValuesBasePtr */ nullptr,
  /* .customExt */ nullptr,
  /* .fieldInfos */ {
  {
    /* .id */ 1,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldA",
    /* .memberOffset */ 0,
    /* .issetOffset */ 0,
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::structure, ::test::fixtures::tablebased::ContainerStruct>::typeInfo,
  },
  {
    /* .id */ 2,
    /* .qualifier */ ::apache::thrift::detail::FieldQualifier::Unqualified,
    /* .name */ "fieldB",
    /* .memberOffset */ 0,
    /* .issetOffset */ 0,
    /* .typeInfo */ &::apache::thrift::detail::TypeToInfo<::apache::thrift::type_class::structure, ::test::fixtures::tablebased::TrivialTypesStruct>::typeInfo,
  }}
};
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        ExampleUnion,
        ::apache::thrift::type_class::structure,
        ::test::fixtures::tablebased::ContainerStruct>,
    "inconsistent use of json option");
static_assert(
    ::apache::thrift::detail::st::gen_check_json<
        ExampleUnion,
        ::apache::thrift::type_class::structure,
        ::test::fixtures::tablebased::TrivialTypesStruct>,
    "inconsistent use of json option");

} // namespace test::fixtures::tablebased

namespace test::fixtures::tablebased { namespace {
[[maybe_unused]] FOLLY_ERASE void validateAdapters() {
}
}} // namespace test::fixtures::tablebased

namespace apache {
namespace thrift {
namespace detail {
const ::apache::thrift::detail::TypeInfo TypeToInfo<
    apache::thrift::type_class::enumeration,
    ::test::fixtures::tablebased::ExampleEnum>::typeInfo = {
  /* .type */ apache::thrift::protocol::TType::T_I32,
  /* .get */ get<std::int32_t, ::test::fixtures::tablebased::ExampleEnum>,
  /* .set */ reinterpret_cast<VoidFuncPtr>(set<::test::fixtures::tablebased::ExampleEnum, std::int32_t>),
  /* .typeExt */ nullptr,
};
const ::apache::thrift::detail::TypeInfo TypeToInfo<
  ::apache::thrift::type_class::structure,
  ::test::fixtures::tablebased::TrivialTypesStruct>::typeInfo = {
  /* .type */ ::apache::thrift::protocol::TType::T_STRUCT,
  /* .get */ nullptr,
  /* .set */ nullptr,
  /* .typeExt */ &::test::fixtures::tablebased::__fbthrift_struct_info_TrivialTypesStruct,
};
const ::apache::thrift::detail::TypeInfo TypeToInfo<
  ::apache::thrift::type_class::structure,
  ::test::fixtures::tablebased::ContainerStruct>::typeInfo = {
  /* .type */ ::apache::thrift::protocol::TType::T_STRUCT,
  /* .get */ nullptr,
  /* .set */ nullptr,
  /* .typeExt */ &::test::fixtures::tablebased::__fbthrift_struct_info_ContainerStruct,
};
const ::apache::thrift::detail::TypeInfo TypeToInfo<
  ::apache::thrift::type_class::variant,
  ::test::fixtures::tablebased::ExampleUnion>::typeInfo = {
  /* .type */ ::apache::thrift::protocol::TType::T_STRUCT,
  /* .get */ nullptr,
  /* .set */ nullptr,
  /* .typeExt */ &::test::fixtures::tablebased::__fbthrift_struct_info_ExampleUnion,
};
}}} // namespace apache::thrift::detail
namespace apache::thrift::detail::annotation {
}
