/**
 * Autogenerated by Thrift for src/enums.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>



namespace apache {
namespace thrift {
namespace tag {
struct fieldA;
} // namespace tag
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_fieldA
#define APACHE_THRIFT_ACCESSOR_fieldA
APACHE_THRIFT_DEFINE_ACCESSOR(fieldA);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums
namespace facebook { namespace ns { namespace qwerty {

enum class AnEnumA {
  FIELDA = 0,
};




enum class AnEnumB {
  FIELDA = 0,
  FIELDB = 2,
};




enum class AnEnumC {
  FIELDC = 0,
};




enum class AnEnumD {
  FIELDD = 0,
};




enum class AnEnumE {
  FIELDA = 0,
};




}}} // facebook::ns::qwerty

namespace std {
template<> struct hash<::facebook::ns::qwerty::AnEnumA> :
  ::apache::thrift::detail::enum_hash<::facebook::ns::qwerty::AnEnumA> {};
template<> struct hash<::facebook::ns::qwerty::AnEnumB> :
  ::apache::thrift::detail::enum_hash<::facebook::ns::qwerty::AnEnumB> {};
template<> struct hash<::facebook::ns::qwerty::AnEnumC> :
  ::apache::thrift::detail::enum_hash<::facebook::ns::qwerty::AnEnumC> {};
template<> struct hash<::facebook::ns::qwerty::AnEnumD> :
  ::apache::thrift::detail::enum_hash<::facebook::ns::qwerty::AnEnumD> {};
template<> struct hash<::facebook::ns::qwerty::AnEnumE> :
  ::apache::thrift::detail::enum_hash<::facebook::ns::qwerty::AnEnumE> {};
} // std

namespace apache { namespace thrift {


template <> struct TEnumDataStorage<::facebook::ns::qwerty::AnEnumA>;

template <> struct TEnumTraits<::facebook::ns::qwerty::AnEnumA> {
  using type = ::facebook::ns::qwerty::AnEnumA;

  static constexpr std::size_t const size = 1;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::FIELDA; }
  static constexpr type max() { return type::FIELDA; }
};


template <> struct TEnumDataStorage<::facebook::ns::qwerty::AnEnumB>;

template <> struct TEnumTraits<::facebook::ns::qwerty::AnEnumB> {
  using type = ::facebook::ns::qwerty::AnEnumB;

  static constexpr std::size_t const size = 2;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::FIELDA; }
  static constexpr type max() { return type::FIELDB; }
};


template <> struct TEnumDataStorage<::facebook::ns::qwerty::AnEnumC>;

template <> struct TEnumTraits<::facebook::ns::qwerty::AnEnumC> {
  using type = ::facebook::ns::qwerty::AnEnumC;

  static constexpr std::size_t const size = 1;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::FIELDC; }
  static constexpr type max() { return type::FIELDC; }
};


template <> struct TEnumDataStorage<::facebook::ns::qwerty::AnEnumD>;

template <> struct TEnumTraits<::facebook::ns::qwerty::AnEnumD> {
  using type = ::facebook::ns::qwerty::AnEnumD;

  static constexpr std::size_t const size = 1;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::FIELDD; }
  static constexpr type max() { return type::FIELDD; }
};


template <> struct TEnumDataStorage<::facebook::ns::qwerty::AnEnumE>;

template <> struct TEnumTraits<::facebook::ns::qwerty::AnEnumE> {
  using type = ::facebook::ns::qwerty::AnEnumE;

  static constexpr std::size_t const size = 1;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::FIELDA; }
  static constexpr type max() { return type::FIELDA; }
};


}} // apache::thrift

namespace facebook { namespace ns { namespace qwerty {

using _AnEnumA_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<AnEnumA>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _AnEnumA_EnumMapFactory::ValuesToNamesMapType _AnEnumA_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _AnEnumA_EnumMapFactory::NamesToValuesMapType _AnEnumA_NAMES_TO_VALUES;

using _AnEnumB_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<AnEnumB>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _AnEnumB_EnumMapFactory::ValuesToNamesMapType _AnEnumB_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _AnEnumB_EnumMapFactory::NamesToValuesMapType _AnEnumB_NAMES_TO_VALUES;

using _AnEnumC_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<AnEnumC>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _AnEnumC_EnumMapFactory::ValuesToNamesMapType _AnEnumC_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _AnEnumC_EnumMapFactory::NamesToValuesMapType _AnEnumC_NAMES_TO_VALUES;

using _AnEnumD_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<AnEnumD>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _AnEnumD_EnumMapFactory::ValuesToNamesMapType _AnEnumD_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _AnEnumD_EnumMapFactory::NamesToValuesMapType _AnEnumD_NAMES_TO_VALUES;

using _AnEnumE_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<AnEnumE>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _AnEnumE_EnumMapFactory::ValuesToNamesMapType _AnEnumE_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _AnEnumE_EnumMapFactory::NamesToValuesMapType _AnEnumE_NAMES_TO_VALUES;

}}} // facebook::ns::qwerty

// END declare_enums
// BEGIN forward_declare
namespace facebook { namespace ns { namespace qwerty {
class SomeStruct;
}}} // facebook::ns::qwerty
// END forward_declare
// BEGIN typedefs

// END typedefs
// BEGIN hash_and_equal_to
// END hash_and_equal_to
namespace facebook { namespace ns { namespace qwerty {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;
}}} // facebook::ns::qwerty
namespace facebook { namespace ns { namespace qwerty {
class SomeStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_gen_nimble = false;
  static constexpr bool __fbthrift_cpp2_gen_has_thrift_uri = false;

 public:
  using __fbthrift_cpp2_type = SomeStruct;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;


 public:

  SomeStruct() :
      __fbthrift_field_fieldA() {
  }
  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  SomeStruct(apache::thrift::FragileConstructor, ::std::int32_t fieldA__arg);

  SomeStruct(SomeStruct&&) = default;

  SomeStruct(const SomeStruct&) = default;


  SomeStruct& operator=(SomeStruct&&) = default;

  SomeStruct& operator=(const SomeStruct&) = default;
  void __clear();
 private:
  ::std::int32_t __fbthrift_field_fieldA;
 private:
  apache::thrift::detail::isset_bitset<1, false> __isset;

 public:

  bool operator==(const SomeStruct&) const;
  bool operator<(const SomeStruct&) const;

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldA_ref() const& {
    return {this->__fbthrift_field_fieldA, __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldA_ref() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_fieldA), __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldA_ref() & {
    return {this->__fbthrift_field_fieldA, __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldA_ref() && {
    return {static_cast<T&&>(this->__fbthrift_field_fieldA), __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldA() const& {
    return {this->__fbthrift_field_fieldA, __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldA() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_fieldA), __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldA() & {
    return {this->__fbthrift_field_fieldA, __isset.at(0), __isset.bit(0)};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldA() && {
    return {static_cast<T&&>(this->__fbthrift_field_fieldA), __isset.at(0), __isset.bit(0)};
  }

  ::std::int32_t get_fieldA() const {
    return __fbthrift_field_fieldA;
  }

  [[deprecated("Use `FOO.fieldA_ref() = BAR;` instead of `FOO.set_fieldA(BAR);`")]]
  ::std::int32_t& set_fieldA(::std::int32_t fieldA_) {
    fieldA_ref() = fieldA_;
    return __fbthrift_field_fieldA;
  }

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<SomeStruct>;
  friend void swap(SomeStruct& a, SomeStruct& b);
};

template <class Protocol_>
uint32_t SomeStruct::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}

}}} // facebook::ns::qwerty
