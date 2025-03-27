/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/any/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>



namespace apache {
namespace thrift {
namespace ident {
struct myString;
struct myString;
struct myString;
} // namespace ident
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_myString
#define APACHE_THRIFT_ACCESSOR_myString
APACHE_THRIFT_DEFINE_ACCESSOR(myString);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_myString
#define APACHE_THRIFT_ACCESSOR_myString
APACHE_THRIFT_DEFINE_ACCESSOR(myString);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_myString
#define APACHE_THRIFT_ACCESSOR_myString
APACHE_THRIFT_DEFINE_ACCESSOR(myString);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums

// END declare_enums
// BEGIN forward_declare
namespace facebook::thrift::compiler::test::fixtures::any {
namespace detail {
class MyStruct;
} // namespace detail
class MyUnion;
class MyException;
} // namespace facebook::thrift::compiler::test::fixtures::any
// END forward_declare
namespace apache::thrift::detail::annotation {
} // namespace apache::thrift::detail::annotation

namespace apache::thrift::detail::qualifier {
} // namespace apache::thrift::detail::qualifier

// BEGIN hash_and_equal_to
// END hash_and_equal_to
namespace facebook::thrift::compiler::test::fixtures::any {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;


namespace detail {
/** Glean {"file": "thrift/compiler/test/fixtures/any/src/module.thrift", "name": "MyStruct", "kind": "struct" } */
class MyStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = false;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static const char* __fbthrift_thrift_uri();
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "module";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::myString
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::string_t
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

  MyStruct();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  MyStruct(apache::thrift::FragileConstructor, ::std::string myString__arg);

  MyStruct(MyStruct&&) noexcept;

  MyStruct(const MyStruct& src);


  MyStruct& operator=(MyStruct&&) noexcept;
  MyStruct& operator=(const MyStruct& src);

  ~MyStruct();

 private:
  ::std::string __fbthrift_field_myString;
 private:
  apache::thrift::detail::isset_bitset<1, apache::thrift::detail::IssetBitsetOption::Unpacked> __isset;

 public:

  bool operator==(const MyStruct&) const;
  bool operator<(const MyStruct&) const;

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> myString_ref() const& {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> myString_ref() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> myString_ref() & {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> myString_ref() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> myString() const& {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> myString() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> myString() & {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> myString() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  [[deprecated("Use `FOO.myString().value()` instead of `FOO.get_myString()`")]]
  const ::std::string& get_myString() const& {
    return __fbthrift_field_myString;
  }

  /** Glean { "field": "myString" } */
  [[deprecated("Use `FOO.myString().value()` instead of `FOO.get_myString()`")]]
  ::std::string get_myString() && {
    return static_cast<::std::string&&>(__fbthrift_field_myString);
  }

  /** Glean { "field": "myString" } */
  template <typename T_MyStruct_myString_struct_setter = ::std::string>
  [[deprecated("Use `FOO.myString() = BAR` instead of `FOO.set_myString(BAR)`")]]
  ::std::string& set_myString(T_MyStruct_myString_struct_setter&& myString_) {
    myString_ref() = std::forward<T_MyStruct_myString_struct_setter>(myString_);
    return __fbthrift_field_myString;
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
} // namespace detail

using MyStruct = ::apache::thrift::adapt_detail::adapted_t<::my::Adapter1, ::facebook::thrift::compiler::test::fixtures::any::detail::MyStruct>;


/** Glean {"file": "thrift/compiler/test/fixtures/any/src/module.thrift", "name": "MyUnion", "kind": "union" } */
class MyUnion final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = false;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static const char* __fbthrift_thrift_uri();
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "module";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::myString
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::string_t
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
  void __fbthrift_destruct();
  bool __fbthrift_is_empty() const;

 public:
  using __fbthrift_cpp2_type = MyUnion;
  static constexpr bool __fbthrift_cpp2_is_union =
    true;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:
  enum Type : int {
    __EMPTY__ = 0,
    myString = 1,
  } ;

  MyUnion()
      : type_(folly::to_underlying(Type::__EMPTY__)) {}

  MyUnion(MyUnion&& rhs) noexcept
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    if (this == &rhs) { return; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
      {
        return;
      }
      case Type::myString:
      {
        set_myString(std::move(rhs.value_.myString));
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }
    apache::thrift::clear(rhs);
  }

  MyUnion(const MyUnion& rhs);

  MyUnion& operator=(MyUnion&& rhs) noexcept {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
      {
        __fbthrift_clear();
        return *this;
      }
      case Type::myString:
      {
        set_myString(std::move(rhs.value_.myString));
        break;
      }
      default:
      {
        assert(false);
        __fbthrift_clear();
      }
    }
    apache::thrift::clear(rhs);
    return *this;
  }

  MyUnion& operator=(const MyUnion& rhs);

  ~MyUnion();

  union storage_type {
    ::std::string myString;

    storage_type() {}
    ~storage_type() {}
  } ;

  bool operator==(const MyUnion&) const;
  bool operator<(const MyUnion&) const;

  /** Glean { "field": "myString" } */
  template <typename... A, std::enable_if_t<!sizeof...(A), int> = 0>
  ::std::string& set_myString(::std::string const &t) {
    using T0 = ::std::string;
    using T = folly::type_t<T0, A...>;
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::myString);
    ::new (std::addressof(value_.myString)) T(t);
    return value_.myString;
  }

  /** Glean { "field": "myString" } */
  template <typename... A, std::enable_if_t<!sizeof...(A), int> = 0>
  ::std::string& set_myString(::std::string&& t) {
    using T0 = ::std::string;
    using T = folly::type_t<T0, A...>;
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::myString);
    ::new (std::addressof(value_.myString)) T(std::move(t));
    return value_.myString;
  }

  /** Glean { "field": "myString" } */
  template<typename... T, typename = ::apache::thrift::safe_overload_t<::std::string, T...>> ::std::string& set_myString(T&&... t) {
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::myString);
    ::new (std::addressof(value_.myString)) ::std::string(std::forward<T>(t)...);
    return value_.myString;
  }


  /** Glean { "field": "myString" } */
  ::std::string const& get_myString() const {
    if (getType() != Type::myString) {
      ::apache::thrift::detail::throw_on_bad_union_field_access();
    }
    return value_.myString;
  }

  ::std::string& mutable_myString() {
    assert(getType() == Type::myString);
    return value_.myString;
  }

  template <typename..., typename T = ::std::string>
  T move_myString() {
    assert(getType() == Type::myString);
    return std::move(value_.myString);
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&> myString_ref() const& {
    return {value_.myString, type_, folly::to_underlying(Type::myString), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&&> myString_ref() const&& {
    return {std::move(value_.myString), type_, folly::to_underlying(Type::myString), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&> myString_ref() & {
    return {value_.myString, type_, folly::to_underlying(Type::myString), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&&> myString_ref() && {
    return {std::move(value_.myString), type_, folly::to_underlying(Type::myString), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }
  Type getType() const { return static_cast<Type>(type_); }

  template <class Protocol_>
  unsigned long read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;
 protected:
  storage_type value_;
  std::underlying_type_t<Type> type_;

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<MyUnion>;
  friend void swap(MyUnion& a, MyUnion& b);
};

template <class Protocol_>
unsigned long MyUnion::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


/** Glean {"file": "thrift/compiler/test/fixtures/any/src/module.thrift", "name": "MyException", "kind": "exception" } */
class FOLLY_EXPORT MyException : public virtual apache::thrift::TException {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;
  template<class> friend struct ::apache::thrift::detail::invoke_reffer;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = false;
  static constexpr bool __fbthrift_cpp2_is_runtime_annotation = false;
  static const char* __fbthrift_thrift_uri();
  static std::string_view __fbthrift_get_field_name(::apache::thrift::FieldOrdinal ord);
  static std::string_view __fbthrift_get_class_name();
  template <class ...>
  FOLLY_ERASE static constexpr std::string_view __fbthrift_get_module_name() noexcept {
    return "module";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::myString
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::string_t
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
  static constexpr ::apache::thrift::ExceptionKind __fbthrift_cpp2_gen_exception_kind =
         ::apache::thrift::ExceptionKind::UNSPECIFIED;
  static constexpr ::apache::thrift::ExceptionSafety __fbthrift_cpp2_gen_exception_safety =
         ::apache::thrift::ExceptionSafety::UNSPECIFIED;
  static constexpr ::apache::thrift::ExceptionBlame __fbthrift_cpp2_gen_exception_blame =
         ::apache::thrift::ExceptionBlame::UNSPECIFIED;

 public:
  using __fbthrift_cpp2_type = MyException;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    false;


 public:

  MyException();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  MyException(apache::thrift::FragileConstructor, ::std::string myString__arg);

  MyException(MyException&&) noexcept;

  MyException(const MyException& src);


  MyException& operator=(MyException&&) noexcept;
  MyException& operator=(const MyException& src);

  ~MyException() override;

 private:
  ::std::string __fbthrift_field_myString;
 private:
  apache::thrift::detail::isset_bitset<1, apache::thrift::detail::IssetBitsetOption::Unpacked> __isset;

 public:

  bool operator==(const MyException&) const;
  bool operator<(const MyException&) const;

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> myString_ref() const& {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> myString_ref() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> myString_ref() & {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> myString_ref() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&> myString() const& {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<const fbthrift_T&&> myString() const&& {
    return {static_cast<const fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&> myString() & {
    return {this->__fbthrift_field_myString, __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  template <typename..., typename fbthrift_T = ::std::string>
  FOLLY_ERASE ::apache::thrift::field_ref<fbthrift_T&&> myString() && {
    return {static_cast<fbthrift_T&&>(this->__fbthrift_field_myString), __isset.at(0), __isset.bit(0)};
  }

  /** Glean { "field": "myString" } */
  [[deprecated("Use `FOO.myString().value()` instead of `FOO.get_myString()`")]]
  const ::std::string& get_myString() const& {
    return __fbthrift_field_myString;
  }

  /** Glean { "field": "myString" } */
  [[deprecated("Use `FOO.myString().value()` instead of `FOO.get_myString()`")]]
  ::std::string get_myString() && {
    return static_cast<::std::string&&>(__fbthrift_field_myString);
  }

  /** Glean { "field": "myString" } */
  template <typename T_MyException_myString_struct_setter = ::std::string>
  [[deprecated("Use `FOO.myString() = BAR` instead of `FOO.set_myString(BAR)`")]]
  ::std::string& set_myString(T_MyException_myString_struct_setter&& myString_) {
    myString_ref() = std::forward<T_MyException_myString_struct_setter>(myString_);
    return __fbthrift_field_myString;
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
    return "::facebook::thrift::compiler::test::fixtures::any::MyException";
  }

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<MyException>;
  friend void swap(MyException& a, MyException& b);
};

template <class Protocol_>
unsigned long MyException::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


} // namespace facebook::thrift::compiler::test::fixtures::any

namespace apache { namespace thrift {

template <> struct TEnumDataStorage<::facebook::thrift::compiler::test::fixtures::any::MyUnion::Type>;

template <> struct TEnumTraits<::facebook::thrift::compiler::test::fixtures::any::MyUnion::Type> {
  using type = ::facebook::thrift::compiler::test::fixtures::any::MyUnion::Type;

  static constexpr std::size_t const size = 1;
  static folly::Range<type const*> const values;
  static folly::Range<std::string_view const*> const names;

  static bool findName(type value, std::string_view* out) noexcept;
  static bool findValue(std::string_view name, type* out) noexcept;

  static char const* findName(type value) noexcept {
    std::string_view ret;
    (void)findName(value, &ret);
    return ret.data();
  }
};
}} // apache::thrift
