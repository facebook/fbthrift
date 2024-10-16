/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/service-schema/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>


#include "thrift/compiler/test/fixtures/service-schema/gen-cpp2/include_types.h"

namespace apache {
namespace thrift {
namespace ident {
struct name;
struct result;
} // namespace ident
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_name
#define APACHE_THRIFT_ACCESSOR_name
APACHE_THRIFT_DEFINE_ACCESSOR(name);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_result
#define APACHE_THRIFT_ACCESSOR_result
APACHE_THRIFT_DEFINE_ACCESSOR(result);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums
namespace cpp2 {

/** Glean {"file": "thrift/compiler/test/fixtures/service-schema/src/module.thrift", "name": "Result", "kind": "enum" } */
enum class Result {
  OK = 0,
  SO_SO = 1,
  GOOD = 2,
};



} // namespace cpp2

namespace std {
template<> struct hash<::cpp2::Result> :
  ::apache::thrift::detail::enum_hash<::cpp2::Result> {};
} // std

namespace apache { namespace thrift {


template <> struct TEnumDataStorage<::cpp2::Result>;

template <> struct TEnumTraits<::cpp2::Result> {
  using type = ::cpp2::Result;

  static constexpr std::size_t const size = 3;
  static const std::string_view type_name;
  static folly::Range<type const*> const values;
  static folly::Range<std::string_view const*> const names;

  static bool findName(type value, std::string_view* out) noexcept;
  static bool findValue(std::string_view name, type* out) noexcept;

  static char const* findName(type value) noexcept {
    std::string_view ret;
    (void)findName(value, &ret);
    return ret.data();
  }
  static constexpr type min() { return type::OK; }
  static constexpr type max() { return type::GOOD; }
};


}} // apache::thrift


// END declare_enums
// BEGIN forward_declare
namespace cpp2 {
class CustomException;
} // namespace cpp2
// END forward_declare
namespace apache::thrift::detail::annotation {
} // namespace apache::thrift::detail::annotation

namespace apache::thrift::detail::qualifier {
} // namespace apache::thrift::detail::qualifier

// BEGIN hash_and_equal_to
// END hash_and_equal_to
namespace cpp2 {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;


/** Glean {"file": "thrift/compiler/test/fixtures/service-schema/src/module.thrift", "name": "CustomException", "kind": "exception" } */
class FOLLY_EXPORT CustomException : public virtual apache::thrift::TException {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = false;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::name,
    ::apache::thrift::ident::result
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1,2};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::string_t,
    ::apache::thrift::type::enum_t<::cpp2::Result>
  >;

  static constexpr std::size_t __fbthrift_field_size_v = 2;

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
  static constexpr ::apache::thrift::ExceptionKind __fbthrift_cpp2_gen_exception_kind =
         ::apache::thrift::ExceptionKind::UNSPECIFIED;
  static constexpr ::apache::thrift::ExceptionSafety __fbthrift_cpp2_gen_exception_safety =
         ::apache::thrift::ExceptionSafety::UNSPECIFIED;
  static constexpr ::apache::thrift::ExceptionBlame __fbthrift_cpp2_gen_exception_blame =
         ::apache::thrift::ExceptionBlame::UNSPECIFIED;

 public:
  using __fbthrift_cpp2_type = CustomException;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:

  CustomException();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  CustomException(apache::thrift::FragileConstructor, ::std::string name__arg, ::cpp2::Result result__arg);

  CustomException(CustomException&&) noexcept;

  CustomException(const CustomException& src);


  CustomException& operator=(CustomException&&) noexcept;
  CustomException& operator=(const CustomException& src);

  ~CustomException() override;

 private:
  ::std::string __fbthrift_field_name;
 private:
  ::cpp2::Result __fbthrift_field_result;
 private:
  apache::thrift::detail::isset_bitset<2, apache::thrift::detail::IssetBitsetOption::Unpacked> __isset;

 public:

  bool operator==(const CustomException&) const;
  bool operator<(const CustomException&) const;

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> name_ref() const& {
    return {this->__fbthrift_field_name, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> name_ref() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_name), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> name_ref() & {
    return {this->__fbthrift_field_name, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> name_ref() && {
    return {static_cast<T&&>(this->__fbthrift_field_name), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> name() const& {
    return {this->__fbthrift_field_name, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> name() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_name), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> name() & {
    return {this->__fbthrift_field_name, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "name" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> name() && {
    return {static_cast<T&&>(this->__fbthrift_field_name), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> result_ref() const& {
    return {this->__fbthrift_field_result, __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> result_ref() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_result), __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> result_ref() & {
    return {this->__fbthrift_field_result, __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> result_ref() && {
    return {static_cast<T&&>(this->__fbthrift_field_result), __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> result() const& {
    return {this->__fbthrift_field_result, __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> result() const&& {
    return {static_cast<const T&&>(this->__fbthrift_field_result), __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> result() & {
    return {this->__fbthrift_field_result, __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "result" } */
  template <typename..., typename T = ::cpp2::Result>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> result() && {
    return {static_cast<T&&>(this->__fbthrift_field_result), __isset.at(1), __isset.bit(1)};
  }

  /** Glean { "field": "name" } */
  const ::std::string& get_name() const& {
    return __fbthrift_field_name;
  }

  /** Glean { "field": "name" } */
  ::std::string get_name() && {
    return std::move(__fbthrift_field_name);
  }

  /** Glean { "field": "name" } */
  template <typename T_CustomException_name_struct_setter = ::std::string>
  [[deprecated("Use `FOO.name() = BAR;` instead of `FOO.set_name(BAR);`")]]
  ::std::string& set_name(T_CustomException_name_struct_setter&& name_) {
    name_ref() = std::forward<T_CustomException_name_struct_setter>(name_);
    return __fbthrift_field_name;
  }

  /** Glean { "field": "result" } */
  ::cpp2::Result get_result() const {
    return __fbthrift_field_result;
  }

  /** Glean { "field": "result" } */
  [[deprecated("Use `FOO.result() = BAR;` instead of `FOO.set_result(BAR);`")]]
  ::cpp2::Result& set_result(::cpp2::Result result_) {
    result_ref() = result_;
    return __fbthrift_field_result;
  }

  template <class Protocol_>
  unsigned long read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;

  const char* what() const noexcept override {
    return "::cpp2::CustomException";
  }

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<CustomException>;
  friend void swap(CustomException& a, CustomException& b);
};

template <class Protocol_>
unsigned long CustomException::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


} // namespace cpp2
