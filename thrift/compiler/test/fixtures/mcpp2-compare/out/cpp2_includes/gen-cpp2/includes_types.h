/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/mcpp2-compare/src/includes.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>


#include "folly/sorted_vector_types.h"

namespace apache {
namespace thrift {
namespace ident {
struct FieldA;
struct FieldA;
} // namespace ident
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_FieldA
#define APACHE_THRIFT_ACCESSOR_FieldA
APACHE_THRIFT_DEFINE_ACCESSOR(FieldA);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_FieldA
#define APACHE_THRIFT_ACCESSOR_FieldA
APACHE_THRIFT_DEFINE_ACCESSOR(FieldA);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums
namespace a::different::ns {

/** Glean {"file": "thrift/compiler/test/fixtures/mcpp2-compare/src/includes.thrift", "name": "AnEnum", "kind": "enum" } */
enum class AnEnum {
  FIELDA = 2,
  FIELDB = 4,
};



} // namespace a::different::ns

namespace std {
template<> struct hash<::a::different::ns::AnEnum> :
  ::apache::thrift::detail::enum_hash<::a::different::ns::AnEnum> {};
} // std

namespace apache { namespace thrift {


template <> struct TEnumDataStorage<::a::different::ns::AnEnum>;

template <> struct TEnumTraits<::a::different::ns::AnEnum> {
  using type = ::a::different::ns::AnEnum;

  static constexpr std::size_t const size = 2;
  static folly::Range<type const*> const values;
  static folly::Range<std::string_view const*> const names;
  static const std::string_view __fbthrift_module_name_internal_do_not_use;

  static bool findName(type value, std::string_view* out) noexcept;
  static bool findValue(std::string_view name, type* out) noexcept;

  FOLLY_ERASE static std::string_view typeName() noexcept {
    return "AnEnum";
  }

  FOLLY_ERASE static constexpr std::string_view moduleName() noexcept {
    return "includes";
  }

  static char const* findName(type value) noexcept {
    std::string_view ret;
    (void)findName(value, &ret);
    return ret.data();
  }
  static constexpr type min() { return type::FIELDA; }
  static constexpr type max() { return type::FIELDB; }
};


}} // apache::thrift


// END declare_enums
// BEGIN forward_declare
namespace a::different::ns {
class AStruct;
class AStructB;
} // namespace a::different::ns
// END forward_declare
namespace apache::thrift::detail::annotation {
} // namespace apache::thrift::detail::annotation

namespace apache::thrift::detail::qualifier {
} // namespace apache::thrift::detail::qualifier

// BEGIN hash_and_equal_to
// END hash_and_equal_to
namespace a::different::ns {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;

/** Glean {"file": "thrift/compiler/test/fixtures/mcpp2-compare/src/includes.thrift", "name": "IncludedInt64", "kind": "typedef" } */
typedef ::std::int64_t IncludedInt64;

/** Glean {"file": "thrift/compiler/test/fixtures/mcpp2-compare/src/includes.thrift", "name": "AStruct", "kind": "struct" } */
class AStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "includes";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::FieldA
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::i32_t
  >;

  static constexpr std::size_t __fbthrift_field_size_v = 1;

  template<class T>
  using __fbthrift_id = ::apache::thrift::type::field_id<__fbthrift_reflection_field_id_list[folly::to_underlying(T::value)]>;

  template<class T>
  using __fbthrift_type_tag = ::apache::thrift::detail::at<__fbthrift_reflection_type_tags, T::value>;

  template<class T>
  using __fbthrift_ident = ::apache::thrift::detail::at<__fbthrift_reflection_ident_list, T::value>;

  template<class T> using __fbthrift_ordinal = ::apache::thrift::type::ordinal_tag<
    ::apache::thrift::detail::getFieldOrdinal<T,
                                              __fbthrift_reflection_ident_list,
                                              __fbthrift_reflection_type_tags>(
      __fbthrift_reflection_field_id_list
    )
  >;
  void __fbthrift_clear();
  void __fbthrift_clear_terse_fields();
  bool __fbthrift_is_empty() const;

 public:
  using __fbthrift_cpp2_type = AStruct;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:

  AStruct() :
      __fbthrift_field_FieldA() {
  }
  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  AStruct(apache::thrift::FragileConstructor, ::std::int32_t FieldA__arg);

  AStruct(AStruct&&) = default;

  AStruct(const AStruct&) = default;


  AStruct& operator=(AStruct&&) = default;

  AStruct& operator=(const AStruct&) = default;
 private:
  ::std::int32_t __fbthrift_field_FieldA;
 private:
  apache::thrift::detail::isset_bitset<1, apache::thrift::detail::IssetBitsetOption::Unpacked> __isset;

 public:

  bool operator==(const AStruct&) const;
  bool operator<(const AStruct&) const;

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> FieldA_ref() const& {
    return {this->__fbthrift_field_FieldA, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> FieldA_ref() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_FieldA), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> FieldA_ref() & {
    return {this->__fbthrift_field_FieldA, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> FieldA_ref() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_FieldA), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> FieldA() const& {
    return {this->__fbthrift_field_FieldA, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> FieldA() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_FieldA), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> FieldA() & {
    return {this->__fbthrift_field_FieldA, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> FieldA() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_FieldA), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "FieldA" } */
  [[deprecated("Use `FOO.FieldA().value()` instead of `FOO.get_FieldA()`")]]
  ::std::int32_t get_FieldA() const;

  /** Glean { "field": "FieldA" } */
  [[deprecated("Use `FOO.FieldA() = BAR` instead of `FOO.set_FieldA(BAR)`")]]
  ::std::int32_t& set_FieldA(::std::int32_t FieldA_);

  template <class Protocol_>
  unsigned long read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<AStruct>;
  friend void swap(AStruct& a, AStruct& b);
};

template <class Protocol_>
unsigned long AStruct::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


/** Glean {"file": "thrift/compiler/test/fixtures/mcpp2-compare/src/includes.thrift", "name": "AStructB", "kind": "struct" } */
class AStructB final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "includes";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::FieldA
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::struct_t<::a::different::ns::AStruct>
  >;

  static constexpr std::size_t __fbthrift_field_size_v = 1;

  template<class T>
  using __fbthrift_id = ::apache::thrift::type::field_id<__fbthrift_reflection_field_id_list[folly::to_underlying(T::value)]>;

  template<class T>
  using __fbthrift_type_tag = ::apache::thrift::detail::at<__fbthrift_reflection_type_tags, T::value>;

  template<class T>
  using __fbthrift_ident = ::apache::thrift::detail::at<__fbthrift_reflection_ident_list, T::value>;

  template<class T> using __fbthrift_ordinal = ::apache::thrift::type::ordinal_tag<
    ::apache::thrift::detail::getFieldOrdinal<T,
                                              __fbthrift_reflection_ident_list,
                                              __fbthrift_reflection_type_tags>(
      __fbthrift_reflection_field_id_list
    )
  >;
  void __fbthrift_clear();
  void __fbthrift_clear_terse_fields();
  bool __fbthrift_is_empty() const;

 public:
  using __fbthrift_cpp2_type = AStructB;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:

  AStructB();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  AStructB(apache::thrift::FragileConstructor, ::std::shared_ptr<const ::a::different::ns::AStruct> FieldA__arg);

  AStructB(AStructB&&) noexcept;

  AStructB(const AStructB& src);


  AStructB& operator=(AStructB&&) noexcept;
  AStructB& operator=(const AStructB& src);

  ~AStructB();

 private:
  ::std::shared_ptr<const ::a::different::ns::AStruct> __fbthrift_field_FieldA;

 public:

  bool operator==(const AStructB&) const;
  bool operator<(const AStructB&) const;
  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE fbthrift_T& FieldA_ref() & {
    return __fbthrift_field_FieldA;
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE const fbthrift_T& FieldA_ref() const& {
    return __fbthrift_field_FieldA;
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE fbthrift_T&& FieldA_ref() && {
    return static_cast<fbthrift_T&&>(__fbthrift_field_FieldA);
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE const fbthrift_T&& FieldA_ref() const&& {
    return static_cast<const fbthrift_T&&>(__fbthrift_field_FieldA);
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE fbthrift_T& FieldA() & {
    return __fbthrift_field_FieldA;
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE const fbthrift_T& FieldA() const& {
    return __fbthrift_field_FieldA;
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE fbthrift_T&& FieldA() && {
    return static_cast<fbthrift_T&&>(__fbthrift_field_FieldA);
  }

  /** Glean { "field": "FieldA" } */
  template <typename..., typename fbthrift_T = ::std::shared_ptr<const ::a::different::ns::AStruct>>
  FOLLY_ERASE const fbthrift_T&& FieldA() const&& {
    return static_cast<const fbthrift_T&&>(__fbthrift_field_FieldA);
  }

  template <class Protocol_>
  unsigned long read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<AStructB>;
  friend void swap(AStructB& a, AStructB& b);
};

template <class Protocol_>
unsigned long AStructB::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


} // namespace a::different::ns
