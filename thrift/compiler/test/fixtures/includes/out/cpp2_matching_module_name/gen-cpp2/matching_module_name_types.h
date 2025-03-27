/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/includes/src/matching_module_name.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>


#include "other/gen-cpp2/matching_module_name_types.h"

namespace apache {
namespace thrift {
namespace ident {
struct OtherStructField;
} // namespace ident
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_OtherStructField
#define APACHE_THRIFT_ACCESSOR_OtherStructField
APACHE_THRIFT_DEFINE_ACCESSOR(OtherStructField);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums

// END declare_enums
// BEGIN forward_declare
namespace matching_module_name {
class MyStruct;
} // namespace matching_module_name
// END forward_declare
namespace apache::thrift::detail::annotation {
} // namespace apache::thrift::detail::annotation

namespace apache::thrift::detail::qualifier {
} // namespace apache::thrift::detail::qualifier

// BEGIN hash_and_equal_to
// END hash_and_equal_to
namespace matching_module_name {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;


/** Glean {"file": "thrift/compiler/test/fixtures/includes/src/matching_module_name.thrift", "name": "MyStruct", "kind": "struct" } */
class MyStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = false;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "matching_module_name";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::OtherStructField
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::struct_t<::matching_module_name::OtherStruct>
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
  using __fbthrift_cpp2_type = MyStruct;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:

  MyStruct() {
  }
  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  MyStruct(apache::thrift::FragileConstructor, ::matching_module_name::OtherStruct OtherStructField__arg);

  MyStruct(MyStruct&&) = default;

  MyStruct(const MyStruct&) = default;


  MyStruct& operator=(MyStruct&&) = default;

  MyStruct& operator=(const MyStruct&) = default;
 private:
  ::matching_module_name::OtherStruct __fbthrift_field_OtherStructField;
 private:
  apache::thrift::detail::isset_bitset<1, apache::thrift::detail::IssetBitsetOption::Unpacked> __isset;

 public:

  bool operator==(const MyStruct&) const;
  bool operator<(const MyStruct&) const;

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> OtherStructField_ref() const& {
    return {this->__fbthrift_field_OtherStructField, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> OtherStructField_ref() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_OtherStructField), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> OtherStructField_ref() & {
    return {this->__fbthrift_field_OtherStructField, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> OtherStructField_ref() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_OtherStructField), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> OtherStructField() const& {
    return {this->__fbthrift_field_OtherStructField, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> OtherStructField() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_OtherStructField), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> OtherStructField() & {
    return {this->__fbthrift_field_OtherStructField, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  template <typename..., typename fbthrift_T = ::matching_module_name::OtherStruct>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> OtherStructField() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_OtherStructField), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "OtherStructField" } */
  [[deprecated("Use `FOO.OtherStructField().value()` instead of `FOO.get_OtherStructField()`")]]
  const ::matching_module_name::OtherStruct& get_OtherStructField() const&;

  /** Glean { "field": "OtherStructField" } */
  [[deprecated("Use `FOO.OtherStructField().value()` instead of `FOO.get_OtherStructField()`")]]
  ::matching_module_name::OtherStruct get_OtherStructField() &&;

  /** Glean { "field": "OtherStructField" } */
  template <typename T_MyStruct_OtherStructField_struct_setter = ::matching_module_name::OtherStruct>
  [[deprecated("Use `FOO.OtherStructField() = BAR` instead of `FOO.set_OtherStructField(BAR)`")]]
  ::matching_module_name::OtherStruct& set_OtherStructField(T_MyStruct_OtherStructField_struct_setter&& OtherStructField_) {
    OtherStructField_ref() = std::forward<T_MyStruct_OtherStructField_struct_setter>(OtherStructField_);
    return __fbthrift_field_OtherStructField;
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

  friend class ::apache::thrift::Cpp2Ops<MyStruct>;
  friend void swap(MyStruct& a, MyStruct& b);
};

template <class Protocol_>
unsigned long MyStruct::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


} // namespace matching_module_name
