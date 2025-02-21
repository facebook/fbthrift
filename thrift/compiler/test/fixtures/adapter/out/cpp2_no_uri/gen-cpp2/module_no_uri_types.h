/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/adapter/src/module_no_uri.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>


#include "adapter_dependency.h"

namespace apache {
namespace thrift {
namespace ident {
struct field1;
} // namespace ident
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_field1
#define APACHE_THRIFT_ACCESSOR_field1
APACHE_THRIFT_DEFINE_ACCESSOR(field1);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums

// END declare_enums
// BEGIN forward_declare
namespace cpp2 {
class RefUnion;
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


/** Glean {"file": "thrift/compiler/test/fixtures/adapter/src/module_no_uri.thrift", "name": "RefUnion", "kind": "union" } */
class RefUnion final  {
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
    return "module_no_uri";
  }
  using __fbthrift_reflection_ident_list = folly::tag_t<
    ::apache::thrift::ident::field1
  >;

  static constexpr std::int16_t __fbthrift_reflection_field_id_list[] = {0,1};
  using __fbthrift_reflection_type_tags = folly::tag_t<
    ::apache::thrift::type::adapted<::my::Adapter1, ::apache::thrift::type::string_t>
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
  using __fbthrift_cpp2_type = RefUnion;
  static constexpr bool __fbthrift_cpp2_is_union =
    true;
  static constexpr bool __fbthrift_cpp2_uses_op_encode =
    true;


 public:
  enum Type : int {
    __EMPTY__ = 0,
    field1 = 1,
  } ;

  RefUnion()
      : type_(folly::to_underlying(Type::__EMPTY__)) {}

  RefUnion(RefUnion&& rhs) noexcept
      : type_(folly::to_underlying(Type::__EMPTY__)) {
    if (this == &rhs) { return; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
      {
        return;
      }
      case Type::field1:
      {
        set_field1(std::move(*rhs.value_.field1));
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

  RefUnion(const RefUnion& rhs);

  RefUnion& operator=(RefUnion&& rhs) noexcept {
    if (this == &rhs) { return *this; }
    switch (rhs.getType()) {
      case Type::__EMPTY__:
      {
        __fbthrift_clear();
        return *this;
      }
      case Type::field1:
      {
        set_field1(std::move(*rhs.value_.field1));
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

  RefUnion& operator=(const RefUnion& rhs);

  ~RefUnion();

  union storage_type {
    ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>> field1;

    storage_type() {}
    ~storage_type() {}
  } ;

  bool operator==(const RefUnion&) const;
  bool operator<(const RefUnion&) const;

  /** Glean { "field": "field1" } */
  template <typename... A, std::enable_if_t<!sizeof...(A), int> = 0>
  ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>& set_field1(::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion> const &t) {
    using T0 = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>;
    using T = folly::type_t<T0, A...>;
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::field1);
    ::new (std::addressof(value_.field1)) T(new typename T::element_type(t));
    return value_.field1;
  }

  /** Glean { "field": "field1" } */
  template <typename... A, std::enable_if_t<!sizeof...(A), int> = 0>
  ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>& set_field1(::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>&& t) {
    using T0 = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>;
    using T = folly::type_t<T0, A...>;
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::field1);
    ::new (std::addressof(value_.field1)) T(new typename T::element_type(std::move(t)));
    return value_.field1;
  }

  /** Glean { "field": "field1" } */
  template<typename... T, typename = ::apache::thrift::safe_overload_t<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>, T...>> ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>& set_field1(T&&... t) {
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::field1);
    ::new (std::addressof(value_.field1)) ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>(new ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>::element_type(std::forward<T>(t)...));
    return value_.field1;
  }

  /** Glean { "field": "field1" } */
  ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>& set_field1(::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>> t) {
    __fbthrift_clear();
    type_ = folly::to_underlying(Type::field1);
    ::new (std::addressof(value_.field1)) ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>(std::move(t));
    return value_.field1;
  }

  /** Glean { "field": "field1" } */
  ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>> const& get_field1() const {
    if (getType() != Type::field1) {
      ::apache::thrift::detail::throw_on_bad_union_field_access();
    }
    return value_.field1;
  }

  ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>& mutable_field1() {
    assert(getType() == Type::field1);
    return value_.field1;
  }

  template <typename..., typename T = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>>
  T move_field1() {
    assert(getType() == Type::field1);
    return std::move(value_.field1);
  }

  /** Glean { "field": "field1" } */
  template <typename..., typename T = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&> field1_ref() const& {
    return {value_.field1, type_, folly::to_underlying(Type::field1), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "field1" } */
  template <typename..., typename T = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&&> field1_ref() const&& {
    return {std::move(value_.field1), type_, folly::to_underlying(Type::field1), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "field1" } */
  template <typename..., typename T = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&> field1_ref() & {
    return {value_.field1, type_, folly::to_underlying(Type::field1), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  /** Glean { "field": "field1" } */
  template <typename..., typename T = ::std::shared_ptr<::apache::thrift::adapt_detail::adapted_field_t<::my::Adapter1, 1, ::std::string, RefUnion>>>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&&> field1_ref() && {
    return {std::move(value_.field1), type_, folly::to_underlying(Type::field1), this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
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

  friend class ::apache::thrift::Cpp2Ops<RefUnion>;
  friend void swap(RefUnion& a, RefUnion& b);
};

template <class Protocol_>
unsigned long RefUnion::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}


} // namespace cpp2

namespace apache { namespace thrift {

template <> struct TEnumDataStorage<::cpp2::RefUnion::Type>;

template <> struct TEnumTraits<::cpp2::RefUnion::Type> {
  using type = ::cpp2::RefUnion::Type;

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
