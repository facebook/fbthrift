/**
 * Autogenerated by Thrift for src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#pragma once

#include <thrift/lib/cpp2/gen/module_types_h.h>



namespace apache {
namespace thrift {
namespace tag {
struct fieldA;
struct fieldB;
struct fieldC;
struct fieldD;
struct fieldE;
struct fieldA;
struct fieldB;
struct fieldC;
struct fieldD;
struct fieldE;
struct fieldF;
struct fieldG;
struct fieldH;
struct fieldA;
struct fieldB;
} // namespace tag
namespace detail {
#ifndef APACHE_THRIFT_ACCESSOR_fieldA
#define APACHE_THRIFT_ACCESSOR_fieldA
APACHE_THRIFT_DEFINE_ACCESSOR(fieldA);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldB
#define APACHE_THRIFT_ACCESSOR_fieldB
APACHE_THRIFT_DEFINE_ACCESSOR(fieldB);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldC
#define APACHE_THRIFT_ACCESSOR_fieldC
APACHE_THRIFT_DEFINE_ACCESSOR(fieldC);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldD
#define APACHE_THRIFT_ACCESSOR_fieldD
APACHE_THRIFT_DEFINE_ACCESSOR(fieldD);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldE
#define APACHE_THRIFT_ACCESSOR_fieldE
APACHE_THRIFT_DEFINE_ACCESSOR(fieldE);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldA
#define APACHE_THRIFT_ACCESSOR_fieldA
APACHE_THRIFT_DEFINE_ACCESSOR(fieldA);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldB
#define APACHE_THRIFT_ACCESSOR_fieldB
APACHE_THRIFT_DEFINE_ACCESSOR(fieldB);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldC
#define APACHE_THRIFT_ACCESSOR_fieldC
APACHE_THRIFT_DEFINE_ACCESSOR(fieldC);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldD
#define APACHE_THRIFT_ACCESSOR_fieldD
APACHE_THRIFT_DEFINE_ACCESSOR(fieldD);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldE
#define APACHE_THRIFT_ACCESSOR_fieldE
APACHE_THRIFT_DEFINE_ACCESSOR(fieldE);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldF
#define APACHE_THRIFT_ACCESSOR_fieldF
APACHE_THRIFT_DEFINE_ACCESSOR(fieldF);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldG
#define APACHE_THRIFT_ACCESSOR_fieldG
APACHE_THRIFT_DEFINE_ACCESSOR(fieldG);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldH
#define APACHE_THRIFT_ACCESSOR_fieldH
APACHE_THRIFT_DEFINE_ACCESSOR(fieldH);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldA
#define APACHE_THRIFT_ACCESSOR_fieldA
APACHE_THRIFT_DEFINE_ACCESSOR(fieldA);
#endif
#ifndef APACHE_THRIFT_ACCESSOR_fieldB
#define APACHE_THRIFT_ACCESSOR_fieldB
APACHE_THRIFT_DEFINE_ACCESSOR(fieldB);
#endif
} // namespace detail
} // namespace thrift
} // namespace apache

// BEGIN declare_enums
namespace test { namespace fixtures { namespace tablebased {

enum class ExampleEnum {
  ZERO = 0,
  NONZERO = 123,
};




}}} // test::fixtures::tablebased

namespace std {
template<> struct hash<::test::fixtures::tablebased::ExampleEnum> :
  ::apache::thrift::detail::enum_hash<::test::fixtures::tablebased::ExampleEnum> {};
} // std

namespace apache { namespace thrift {


template <> struct TEnumDataStorage<::test::fixtures::tablebased::ExampleEnum>;

template <> struct TEnumTraits<::test::fixtures::tablebased::ExampleEnum> {
  using type = ::test::fixtures::tablebased::ExampleEnum;

  static constexpr std::size_t const size = 2;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

  static constexpr type min() { return type::ZERO; }
  static constexpr type max() { return type::NONZERO; }
};


}} // apache::thrift

namespace test { namespace fixtures { namespace tablebased {

using _ExampleEnum_EnumMapFactory = apache::thrift::detail::TEnumMapFactory<ExampleEnum>;
[[deprecated("use apache::thrift::util::enumNameSafe, apache::thrift::util::enumName, or apache::thrift::TEnumTraits")]]
extern const _ExampleEnum_EnumMapFactory::ValuesToNamesMapType _ExampleEnum_VALUES_TO_NAMES;
[[deprecated("use apache::thrift::TEnumTraits")]]
extern const _ExampleEnum_EnumMapFactory::NamesToValuesMapType _ExampleEnum_NAMES_TO_VALUES;

}}} // test::fixtures::tablebased

// END declare_enums
// BEGIN forward_declare
namespace test { namespace fixtures { namespace tablebased {
class TrivialTypesStruct;
class ContainerStruct;
class ExampleUnion;
}}} // test::fixtures::tablebased
// END forward_declare
// BEGIN typedefs
namespace test { namespace fixtures { namespace tablebased {
typedef std::unique_ptr<folly::IOBuf> IOBufPtr;

}}} // test::fixtures::tablebased
// END typedefs
// BEGIN hash_and_equal_to
// END hash_and_equal_to
THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
namespace test { namespace fixtures { namespace tablebased {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;

class TrivialTypesStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_gen_nimble = false;
  static constexpr bool __fbthrift_cpp2_gen_has_thrift_uri = false;

 public:
  using __fbthrift_cpp2_type = TrivialTypesStruct;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;


 public:

  TrivialTypesStruct();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  TrivialTypesStruct(apache::thrift::FragileConstructor, ::std::int32_t fieldA__arg, ::std::string fieldB__arg, ::std::string fieldC__arg, ::test::fixtures::tablebased::IOBufPtr fieldD__arg, ::test::fixtures::tablebased::ExampleEnum fieldE__arg);

  TrivialTypesStruct(TrivialTypesStruct&&) noexcept;
  TrivialTypesStruct(const TrivialTypesStruct& src);


  TrivialTypesStruct& operator=(TrivialTypesStruct&&) noexcept;
  TrivialTypesStruct& operator=(const TrivialTypesStruct& src);
  void __clear();

  ~TrivialTypesStruct();

 private:
  ::std::int32_t fieldA;
 private:
  ::std::string fieldB;
 private:
  ::std::string fieldC;
 public:
  ::test::fixtures::tablebased::IOBufPtr fieldD;
 private:
  ::test::fixtures::tablebased::ExampleEnum fieldE;

 private:
  [[deprecated("__isset field is deprecated in Thrift struct. Use _ref() accessors instead.")]]
  struct __isset {
    bool fieldA;
    bool fieldB;
    bool fieldC;
    bool fieldD;
    bool fieldE;
  } __isset = {};

 public:

  bool operator==(const TrivialTypesStruct&) const;
  bool operator<(const TrivialTypesStruct&) const;

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&> fieldA_ref() const& {
    return {this->fieldA, __isset.fieldA};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&&> fieldA_ref() const&& {
    return {std::move(this->fieldA), __isset.fieldA};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&> fieldA_ref() & {
    return {this->fieldA, __isset.fieldA};
  }

  template <typename..., typename T = ::std::int32_t>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&&> fieldA_ref() && {
    return {std::move(this->fieldA), __isset.fieldA};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&> fieldB_ref() const& {
    return {this->fieldB, __isset.fieldB};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&&> fieldB_ref() const&& {
    return {std::move(this->fieldB), __isset.fieldB};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&> fieldB_ref() & {
    return {this->fieldB, __isset.fieldB};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&&> fieldB_ref() && {
    return {std::move(this->fieldB), __isset.fieldB};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&> fieldC_ref() const& {
    return {this->fieldC, __isset.fieldC};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&&> fieldC_ref() const&& {
    return {std::move(this->fieldC), __isset.fieldC};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&> fieldC_ref() & {
    return {this->fieldC, __isset.fieldC};
  }

  template <typename..., typename T = ::std::string>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&&> fieldC_ref() && {
    return {std::move(this->fieldC), __isset.fieldC};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::IOBufPtr>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&> fieldD_ref() const& {
    return {this->fieldD, __isset.fieldD};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::IOBufPtr>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<const T&&> fieldD_ref() const&& {
    return {std::move(this->fieldD), __isset.fieldD};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::IOBufPtr>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&> fieldD_ref() & {
    return {this->fieldD, __isset.fieldD};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::IOBufPtr>
  FOLLY_ERASE ::apache::thrift::optional_field_ref<T&&> fieldD_ref() && {
    return {std::move(this->fieldD), __isset.fieldD};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ExampleEnum>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldE_ref() const& {
    return {this->fieldE, __isset.fieldE};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ExampleEnum>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldE_ref() const&& {
    return {std::move(this->fieldE), __isset.fieldE};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ExampleEnum>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldE_ref() & {
    return {this->fieldE, __isset.fieldE};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ExampleEnum>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldE_ref() && {
    return {std::move(this->fieldE), __isset.fieldE};
  }

  const ::std::int32_t* get_fieldA() const& {
    return fieldA_ref() ? std::addressof(fieldA) : nullptr;
  }

  ::std::int32_t* get_fieldA() & {
    return fieldA_ref() ? std::addressof(fieldA) : nullptr;
  }
  ::std::int32_t* get_fieldA() && = delete;

  [[deprecated("Use `FOO.fieldA_ref() = BAR;` instead of `FOO.set_fieldA(BAR);`")]]
  ::std::int32_t& set_fieldA(::std::int32_t fieldA_) {
    fieldA = fieldA_;
    __isset.fieldA = true;
    return fieldA;
  }

  const ::std::string* get_fieldB() const& {
    return fieldB_ref() ? std::addressof(fieldB) : nullptr;
  }

  ::std::string* get_fieldB() & {
    return fieldB_ref() ? std::addressof(fieldB) : nullptr;
  }
  ::std::string* get_fieldB() && = delete;

  template <typename T_TrivialTypesStruct_fieldB_struct_setter = ::std::string>
  [[deprecated("Use `FOO.fieldB_ref() = BAR;` instead of `FOO.set_fieldB(BAR);`")]]
  ::std::string& set_fieldB(T_TrivialTypesStruct_fieldB_struct_setter&& fieldB_) {
    fieldB = std::forward<T_TrivialTypesStruct_fieldB_struct_setter>(fieldB_);
    __isset.fieldB = true;
    return fieldB;
  }

  const ::std::string* get_fieldC() const& {
    return fieldC_ref() ? std::addressof(fieldC) : nullptr;
  }

  ::std::string* get_fieldC() & {
    return fieldC_ref() ? std::addressof(fieldC) : nullptr;
  }
  ::std::string* get_fieldC() && = delete;

  template <typename T_TrivialTypesStruct_fieldC_struct_setter = ::std::string>
  [[deprecated("Use `FOO.fieldC_ref() = BAR;` instead of `FOO.set_fieldC(BAR);`")]]
  ::std::string& set_fieldC(T_TrivialTypesStruct_fieldC_struct_setter&& fieldC_) {
    fieldC = std::forward<T_TrivialTypesStruct_fieldC_struct_setter>(fieldC_);
    __isset.fieldC = true;
    return fieldC;
  }

  const ::test::fixtures::tablebased::IOBufPtr* get_fieldD() const& {
    return fieldD_ref() ? std::addressof(fieldD) : nullptr;
  }

  ::test::fixtures::tablebased::IOBufPtr* get_fieldD() & {
    return fieldD_ref() ? std::addressof(fieldD) : nullptr;
  }
  ::test::fixtures::tablebased::IOBufPtr* get_fieldD() && = delete;

  template <typename T_TrivialTypesStruct_fieldD_struct_setter = ::test::fixtures::tablebased::IOBufPtr>
  [[deprecated("Use `FOO.fieldD_ref() = BAR;` instead of `FOO.set_fieldD(BAR);`")]]
  ::test::fixtures::tablebased::IOBufPtr& set_fieldD(T_TrivialTypesStruct_fieldD_struct_setter&& fieldD_) {
    fieldD = std::forward<T_TrivialTypesStruct_fieldD_struct_setter>(fieldD_);
    __isset.fieldD = true;
    return fieldD;
  }

  ::test::fixtures::tablebased::ExampleEnum get_fieldE() const {
    return fieldE;
  }

  [[deprecated("Use `FOO.fieldE_ref() = BAR;` instead of `FOO.set_fieldE(BAR);`")]]
  ::test::fixtures::tablebased::ExampleEnum& set_fieldE(::test::fixtures::tablebased::ExampleEnum fieldE_) {
    fieldE = fieldE_;
    __isset.fieldE = true;
    return fieldE;
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

  friend class ::apache::thrift::Cpp2Ops<TrivialTypesStruct>;
  friend void swap(TrivialTypesStruct& a, TrivialTypesStruct& b);
  friend constexpr ptrdiff_t (::apache::thrift::detail::fieldOffset<TrivialTypesStruct>)(std::int16_t fieldIndex);
  friend constexpr ptrdiff_t (::apache::thrift::detail::issetOffset<TrivialTypesStruct>)(std::int16_t fieldIndex);
};

template <class Protocol_>
uint32_t TrivialTypesStruct::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}

}}} // test::fixtures::tablebased
namespace test { namespace fixtures { namespace tablebased {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;

class ContainerStruct final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_gen_nimble = false;
  static constexpr bool __fbthrift_cpp2_gen_has_thrift_uri = false;

 public:
  using __fbthrift_cpp2_type = ContainerStruct;
  static constexpr bool __fbthrift_cpp2_is_union =
    false;


 public:

  ContainerStruct();

  // FragileConstructor for use in initialization lists only.
  [[deprecated("This constructor is deprecated")]]
  ContainerStruct(apache::thrift::FragileConstructor, ::std::vector<::std::int32_t> fieldA__arg, std::list<::std::int32_t> fieldB__arg, std::deque<::std::int32_t> fieldC__arg, folly::fbvector<::std::int32_t> fieldD__arg, folly::small_vector<::std::int32_t> fieldE__arg, folly::sorted_vector_set<::std::int32_t> fieldF__arg, folly::sorted_vector_map<::std::int32_t, ::std::string> fieldG__arg, ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct> fieldH__arg);

  ContainerStruct(ContainerStruct&&) noexcept;

  ContainerStruct(const ContainerStruct& src);


  ContainerStruct& operator=(ContainerStruct&&) noexcept;
  ContainerStruct& operator=(const ContainerStruct& src);
  void __clear();

  ~ContainerStruct();

 private:
  ::std::vector<::std::int32_t> fieldA;
 private:
  std::list<::std::int32_t> fieldB;
 private:
  std::deque<::std::int32_t> fieldC;
 private:
  folly::fbvector<::std::int32_t> fieldD;
 private:
  folly::small_vector<::std::int32_t> fieldE;
 private:
  folly::sorted_vector_set<::std::int32_t> fieldF;
 private:
  folly::sorted_vector_map<::std::int32_t, ::std::string> fieldG;
 private:
  ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct> fieldH;

 private:
  [[deprecated("__isset field is deprecated in Thrift struct. Use _ref() accessors instead.")]]
  struct __isset {
    bool fieldA;
    bool fieldB;
    bool fieldC;
    bool fieldD;
    bool fieldE;
    bool fieldF;
    bool fieldG;
    bool fieldH;
  } __isset = {};

 public:

  bool operator==(const ContainerStruct&) const;
  bool operator<(const ContainerStruct&) const;

  template <typename..., typename T = ::std::vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldA_ref() const& {
    return {this->fieldA, __isset.fieldA};
  }

  template <typename..., typename T = ::std::vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldA_ref() const&& {
    return {std::move(this->fieldA), __isset.fieldA};
  }

  template <typename..., typename T = ::std::vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldA_ref() & {
    return {this->fieldA, __isset.fieldA};
  }

  template <typename..., typename T = ::std::vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldA_ref() && {
    return {std::move(this->fieldA), __isset.fieldA};
  }

  template <typename..., typename T = std::list<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldB_ref() const& {
    return {this->fieldB, __isset.fieldB};
  }

  template <typename..., typename T = std::list<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldB_ref() const&& {
    return {std::move(this->fieldB), __isset.fieldB};
  }

  template <typename..., typename T = std::list<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldB_ref() & {
    return {this->fieldB, __isset.fieldB};
  }

  template <typename..., typename T = std::list<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldB_ref() && {
    return {std::move(this->fieldB), __isset.fieldB};
  }

  template <typename..., typename T = std::deque<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldC_ref() const& {
    return {this->fieldC, __isset.fieldC};
  }

  template <typename..., typename T = std::deque<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldC_ref() const&& {
    return {std::move(this->fieldC), __isset.fieldC};
  }

  template <typename..., typename T = std::deque<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldC_ref() & {
    return {this->fieldC, __isset.fieldC};
  }

  template <typename..., typename T = std::deque<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldC_ref() && {
    return {std::move(this->fieldC), __isset.fieldC};
  }

  template <typename..., typename T = folly::fbvector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldD_ref() const& {
    return {this->fieldD, __isset.fieldD};
  }

  template <typename..., typename T = folly::fbvector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldD_ref() const&& {
    return {std::move(this->fieldD), __isset.fieldD};
  }

  template <typename..., typename T = folly::fbvector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldD_ref() & {
    return {this->fieldD, __isset.fieldD};
  }

  template <typename..., typename T = folly::fbvector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldD_ref() && {
    return {std::move(this->fieldD), __isset.fieldD};
  }

  template <typename..., typename T = folly::small_vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldE_ref() const& {
    return {this->fieldE, __isset.fieldE};
  }

  template <typename..., typename T = folly::small_vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldE_ref() const&& {
    return {std::move(this->fieldE), __isset.fieldE};
  }

  template <typename..., typename T = folly::small_vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldE_ref() & {
    return {this->fieldE, __isset.fieldE};
  }

  template <typename..., typename T = folly::small_vector<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldE_ref() && {
    return {std::move(this->fieldE), __isset.fieldE};
  }

  template <typename..., typename T = folly::sorted_vector_set<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldF_ref() const& {
    return {this->fieldF, __isset.fieldF};
  }

  template <typename..., typename T = folly::sorted_vector_set<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldF_ref() const&& {
    return {std::move(this->fieldF), __isset.fieldF};
  }

  template <typename..., typename T = folly::sorted_vector_set<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldF_ref() & {
    return {this->fieldF, __isset.fieldF};
  }

  template <typename..., typename T = folly::sorted_vector_set<::std::int32_t>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldF_ref() && {
    return {std::move(this->fieldF), __isset.fieldF};
  }

  template <typename..., typename T = folly::sorted_vector_map<::std::int32_t, ::std::string>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldG_ref() const& {
    return {this->fieldG, __isset.fieldG};
  }

  template <typename..., typename T = folly::sorted_vector_map<::std::int32_t, ::std::string>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldG_ref() const&& {
    return {std::move(this->fieldG), __isset.fieldG};
  }

  template <typename..., typename T = folly::sorted_vector_map<::std::int32_t, ::std::string>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldG_ref() & {
    return {this->fieldG, __isset.fieldG};
  }

  template <typename..., typename T = folly::sorted_vector_map<::std::int32_t, ::std::string>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldG_ref() && {
    return {std::move(this->fieldG), __isset.fieldG};
  }

  template <typename..., typename T = ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&> fieldH_ref() const& {
    return {this->fieldH, __isset.fieldH};
  }

  template <typename..., typename T = ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>
  FOLLY_ERASE ::apache::thrift::field_ref<const T&&> fieldH_ref() const&& {
    return {std::move(this->fieldH), __isset.fieldH};
  }

  template <typename..., typename T = ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&> fieldH_ref() & {
    return {this->fieldH, __isset.fieldH};
  }

  template <typename..., typename T = ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>
  FOLLY_ERASE ::apache::thrift::field_ref<T&&> fieldH_ref() && {
    return {std::move(this->fieldH), __isset.fieldH};
  }
  const ::std::vector<::std::int32_t>& get_fieldA() const&;
  ::std::vector<::std::int32_t> get_fieldA() &&;

  template <typename T_ContainerStruct_fieldA_struct_setter = ::std::vector<::std::int32_t>>
  [[deprecated("Use `FOO.fieldA_ref() = BAR;` instead of `FOO.set_fieldA(BAR);`")]]
  ::std::vector<::std::int32_t>& set_fieldA(T_ContainerStruct_fieldA_struct_setter&& fieldA_) {
    fieldA = std::forward<T_ContainerStruct_fieldA_struct_setter>(fieldA_);
    __isset.fieldA = true;
    return fieldA;
  }
  const std::list<::std::int32_t>& get_fieldB() const&;
  std::list<::std::int32_t> get_fieldB() &&;

  template <typename T_ContainerStruct_fieldB_struct_setter = std::list<::std::int32_t>>
  [[deprecated("Use `FOO.fieldB_ref() = BAR;` instead of `FOO.set_fieldB(BAR);`")]]
  std::list<::std::int32_t>& set_fieldB(T_ContainerStruct_fieldB_struct_setter&& fieldB_) {
    fieldB = std::forward<T_ContainerStruct_fieldB_struct_setter>(fieldB_);
    __isset.fieldB = true;
    return fieldB;
  }
  const std::deque<::std::int32_t>& get_fieldC() const&;
  std::deque<::std::int32_t> get_fieldC() &&;

  template <typename T_ContainerStruct_fieldC_struct_setter = std::deque<::std::int32_t>>
  [[deprecated("Use `FOO.fieldC_ref() = BAR;` instead of `FOO.set_fieldC(BAR);`")]]
  std::deque<::std::int32_t>& set_fieldC(T_ContainerStruct_fieldC_struct_setter&& fieldC_) {
    fieldC = std::forward<T_ContainerStruct_fieldC_struct_setter>(fieldC_);
    __isset.fieldC = true;
    return fieldC;
  }
  const folly::fbvector<::std::int32_t>& get_fieldD() const&;
  folly::fbvector<::std::int32_t> get_fieldD() &&;

  template <typename T_ContainerStruct_fieldD_struct_setter = folly::fbvector<::std::int32_t>>
  [[deprecated("Use `FOO.fieldD_ref() = BAR;` instead of `FOO.set_fieldD(BAR);`")]]
  folly::fbvector<::std::int32_t>& set_fieldD(T_ContainerStruct_fieldD_struct_setter&& fieldD_) {
    fieldD = std::forward<T_ContainerStruct_fieldD_struct_setter>(fieldD_);
    __isset.fieldD = true;
    return fieldD;
  }
  const folly::small_vector<::std::int32_t>& get_fieldE() const&;
  folly::small_vector<::std::int32_t> get_fieldE() &&;

  template <typename T_ContainerStruct_fieldE_struct_setter = folly::small_vector<::std::int32_t>>
  [[deprecated("Use `FOO.fieldE_ref() = BAR;` instead of `FOO.set_fieldE(BAR);`")]]
  folly::small_vector<::std::int32_t>& set_fieldE(T_ContainerStruct_fieldE_struct_setter&& fieldE_) {
    fieldE = std::forward<T_ContainerStruct_fieldE_struct_setter>(fieldE_);
    __isset.fieldE = true;
    return fieldE;
  }
  const folly::sorted_vector_set<::std::int32_t>& get_fieldF() const&;
  folly::sorted_vector_set<::std::int32_t> get_fieldF() &&;

  template <typename T_ContainerStruct_fieldF_struct_setter = folly::sorted_vector_set<::std::int32_t>>
  [[deprecated("Use `FOO.fieldF_ref() = BAR;` instead of `FOO.set_fieldF(BAR);`")]]
  folly::sorted_vector_set<::std::int32_t>& set_fieldF(T_ContainerStruct_fieldF_struct_setter&& fieldF_) {
    fieldF = std::forward<T_ContainerStruct_fieldF_struct_setter>(fieldF_);
    __isset.fieldF = true;
    return fieldF;
  }
  const folly::sorted_vector_map<::std::int32_t, ::std::string>& get_fieldG() const&;
  folly::sorted_vector_map<::std::int32_t, ::std::string> get_fieldG() &&;

  template <typename T_ContainerStruct_fieldG_struct_setter = folly::sorted_vector_map<::std::int32_t, ::std::string>>
  [[deprecated("Use `FOO.fieldG_ref() = BAR;` instead of `FOO.set_fieldG(BAR);`")]]
  folly::sorted_vector_map<::std::int32_t, ::std::string>& set_fieldG(T_ContainerStruct_fieldG_struct_setter&& fieldG_) {
    fieldG = std::forward<T_ContainerStruct_fieldG_struct_setter>(fieldG_);
    __isset.fieldG = true;
    return fieldG;
  }
  const ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>& get_fieldH() const&;
  ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct> get_fieldH() &&;

  template <typename T_ContainerStruct_fieldH_struct_setter = ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>>
  [[deprecated("Use `FOO.fieldH_ref() = BAR;` instead of `FOO.set_fieldH(BAR);`")]]
  ::std::vector<::test::fixtures::tablebased::TrivialTypesStruct>& set_fieldH(T_ContainerStruct_fieldH_struct_setter&& fieldH_) {
    fieldH = std::forward<T_ContainerStruct_fieldH_struct_setter>(fieldH_);
    __isset.fieldH = true;
    return fieldH;
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

  friend class ::apache::thrift::Cpp2Ops<ContainerStruct>;
  friend void swap(ContainerStruct& a, ContainerStruct& b);
  friend constexpr ptrdiff_t (::apache::thrift::detail::fieldOffset<ContainerStruct>)(std::int16_t fieldIndex);
  friend constexpr ptrdiff_t (::apache::thrift::detail::issetOffset<ContainerStruct>)(std::int16_t fieldIndex);
};

template <class Protocol_>
uint32_t ContainerStruct::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}

}}} // test::fixtures::tablebased
namespace test { namespace fixtures { namespace tablebased {
using ::apache::thrift::detail::operator!=;
using ::apache::thrift::detail::operator>;
using ::apache::thrift::detail::operator<=;
using ::apache::thrift::detail::operator>=;

class ExampleUnion final  {
 private:
  friend struct ::apache::thrift::detail::st::struct_private_access;

  //  used by a static_assert in the corresponding source
  static constexpr bool __fbthrift_cpp2_gen_json = true;
  static constexpr bool __fbthrift_cpp2_gen_nimble = false;
  static constexpr bool __fbthrift_cpp2_gen_has_thrift_uri = false;

 public:
  using __fbthrift_cpp2_type = ExampleUnion;
  static constexpr bool __fbthrift_cpp2_is_union =
    true;


 public:
  enum Type : int {
    __EMPTY__ = 0,
    fieldA = 1,
    fieldB = 2,
  } ;

  ExampleUnion()
      : type_(Type::__EMPTY__) {}

  ExampleUnion(ExampleUnion&& rhs) noexcept
      : type_(Type::__EMPTY__) {
    if (this == &rhs) { return; }
    if (rhs.type_ == Type::__EMPTY__) { return; }
    switch (rhs.type_) {
      case Type::fieldA:
      {
        set_fieldA(std::move(rhs.value_.fieldA));
        break;
      }
      case Type::fieldB:
      {
        set_fieldB(std::move(rhs.value_.fieldB));
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }
    rhs.__clear();
  }

  ExampleUnion(const ExampleUnion& rhs)
      : type_(Type::__EMPTY__) {
    if (this == &rhs) { return; }
    if (rhs.type_ == Type::__EMPTY__) { return; }
    switch (rhs.type_) {
      case Type::fieldA:
      {
        set_fieldA(rhs.value_.fieldA);
        break;
      }
      case Type::fieldB:
      {
        set_fieldB(rhs.value_.fieldB);
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }
  }

  ExampleUnion& operator=(ExampleUnion&& rhs) noexcept {
    if (this == &rhs) { return *this; }
    __clear();
    if (rhs.type_ == Type::__EMPTY__) { return *this; }
    switch (rhs.type_) {
      case Type::fieldA:
      {
        set_fieldA(std::move(rhs.value_.fieldA));
        break;
      }
      case Type::fieldB:
      {
        set_fieldB(std::move(rhs.value_.fieldB));
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }
    rhs.__clear();
    return *this;
  }

  ExampleUnion& operator=(const ExampleUnion& rhs) {
    if (this == &rhs) { return *this; }
    __clear();
    if (rhs.type_ == Type::__EMPTY__) { return *this; }
    switch (rhs.type_) {
      case Type::fieldA:
      {
        set_fieldA(rhs.value_.fieldA);
        break;
      }
      case Type::fieldB:
      {
        set_fieldB(rhs.value_.fieldB);
        break;
      }
      default:
      {
        assert(false);
        break;
      }
    }
    return *this;
  }
  void __clear();

  ~ExampleUnion() {
    __clear();
  }
  union storage_type {
    ::test::fixtures::tablebased::ContainerStruct fieldA;
    ::test::fixtures::tablebased::TrivialTypesStruct fieldB;

    storage_type() {}
    ~storage_type() {}
  } ;

  bool operator==(const ExampleUnion&) const;
  bool operator<(const ExampleUnion&) const;

  ::test::fixtures::tablebased::ContainerStruct& set_fieldA(::test::fixtures::tablebased::ContainerStruct const &t) {
    __clear();
    type_ = Type::fieldA;
    ::new (std::addressof(value_.fieldA)) ::test::fixtures::tablebased::ContainerStruct(t);
    return value_.fieldA;
  }

  ::test::fixtures::tablebased::ContainerStruct& set_fieldA(::test::fixtures::tablebased::ContainerStruct&& t) {
    __clear();
    type_ = Type::fieldA;
    ::new (std::addressof(value_.fieldA)) ::test::fixtures::tablebased::ContainerStruct(std::move(t));
    return value_.fieldA;
  }

  template<typename... T, typename = ::apache::thrift::safe_overload_t<::test::fixtures::tablebased::ContainerStruct, T...>> ::test::fixtures::tablebased::ContainerStruct& set_fieldA(T&&... t) {
    __clear();
    type_ = Type::fieldA;
    ::new (std::addressof(value_.fieldA)) ::test::fixtures::tablebased::ContainerStruct(std::forward<T>(t)...);
    return value_.fieldA;
  }

  ::test::fixtures::tablebased::TrivialTypesStruct& set_fieldB(::test::fixtures::tablebased::TrivialTypesStruct const &t) {
    __clear();
    type_ = Type::fieldB;
    ::new (std::addressof(value_.fieldB)) ::test::fixtures::tablebased::TrivialTypesStruct(t);
    return value_.fieldB;
  }

  ::test::fixtures::tablebased::TrivialTypesStruct& set_fieldB(::test::fixtures::tablebased::TrivialTypesStruct&& t) {
    __clear();
    type_ = Type::fieldB;
    ::new (std::addressof(value_.fieldB)) ::test::fixtures::tablebased::TrivialTypesStruct(std::move(t));
    return value_.fieldB;
  }

  template<typename... T, typename = ::apache::thrift::safe_overload_t<::test::fixtures::tablebased::TrivialTypesStruct, T...>> ::test::fixtures::tablebased::TrivialTypesStruct& set_fieldB(T&&... t) {
    __clear();
    type_ = Type::fieldB;
    ::new (std::addressof(value_.fieldB)) ::test::fixtures::tablebased::TrivialTypesStruct(std::forward<T>(t)...);
    return value_.fieldB;
  }

  ::test::fixtures::tablebased::ContainerStruct const& get_fieldA() const {
    if (type_ != Type::fieldA) {
      ::apache::thrift::detail::throw_on_bad_field_access();
    }
    return value_.fieldA;
  }

  ::test::fixtures::tablebased::TrivialTypesStruct const& get_fieldB() const {
    if (type_ != Type::fieldB) {
      ::apache::thrift::detail::throw_on_bad_field_access();
    }
    return value_.fieldB;
  }

  ::test::fixtures::tablebased::ContainerStruct& mutable_fieldA() {
    assert(type_ == Type::fieldA);
    return value_.fieldA;
  }

  ::test::fixtures::tablebased::TrivialTypesStruct& mutable_fieldB() {
    assert(type_ == Type::fieldB);
    return value_.fieldB;
  }

  ::test::fixtures::tablebased::ContainerStruct move_fieldA() {
    assert(type_ == Type::fieldA);
    return std::move(value_.fieldA);
  }

  ::test::fixtures::tablebased::TrivialTypesStruct move_fieldB() {
    assert(type_ == Type::fieldB);
    return std::move(value_.fieldB);
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ContainerStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&> fieldA_ref() const& {
    return {value_.fieldA, type_, fieldA, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ContainerStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&&> fieldA_ref() const&& {
    return {std::move(value_.fieldA), type_, fieldA, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ContainerStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&> fieldA_ref() & {
    return {value_.fieldA, type_, fieldA, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::ContainerStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&&> fieldA_ref() && {
    return {std::move(value_.fieldA), type_, fieldA, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }
  template <typename..., typename T = ::test::fixtures::tablebased::TrivialTypesStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&> fieldB_ref() const& {
    return {value_.fieldB, type_, fieldB, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::TrivialTypesStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<const T&&> fieldB_ref() const&& {
    return {std::move(value_.fieldB), type_, fieldB, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::TrivialTypesStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&> fieldB_ref() & {
    return {value_.fieldB, type_, fieldB, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }

  template <typename..., typename T = ::test::fixtures::tablebased::TrivialTypesStruct>
  FOLLY_ERASE ::apache::thrift::union_field_ref<T&&> fieldB_ref() && {
    return {std::move(value_.fieldB), type_, fieldB, this, ::apache::thrift::detail::union_field_ref_owner_vtable_for<decltype(*this)>};
  }
  Type getType() const { return static_cast<Type>(type_); }

  template <class Protocol_>
  uint32_t read(Protocol_* iprot);
  template <class Protocol_>
  uint32_t serializedSize(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t serializedSizeZC(Protocol_ const* prot_) const;
  template <class Protocol_>
  uint32_t write(Protocol_* prot_) const;
 protected:
  template <class T>
  void destruct(T &val) {
    (&val)->~T();
  }

  storage_type value_;
  std::underlying_type_t<Type> type_;

 private:
  template <class Protocol_>
  void readNoXfer(Protocol_* iprot);

  friend class ::apache::thrift::Cpp2Ops<ExampleUnion>;
  friend void swap(ExampleUnion& a, ExampleUnion& b);
  friend constexpr ptrdiff_t (::apache::thrift::detail::unionTypeOffset<ExampleUnion>)();
};

template <class Protocol_>
uint32_t ExampleUnion::read(Protocol_* iprot) {
  auto _xferStart = iprot->getCursorPosition();
  readNoXfer(iprot);
  return iprot->getCursorPosition() - _xferStart;
}

}}} // test::fixtures::tablebased
THRIFT_IGNORE_ISSET_USE_WARNING_END

namespace apache { namespace thrift {

template <> struct TEnumDataStorage<::test::fixtures::tablebased::ExampleUnion::Type>;

template <> struct TEnumTraits<::test::fixtures::tablebased::ExampleUnion::Type> {
  using type = ::test::fixtures::tablebased::ExampleUnion::Type;

  static constexpr std::size_t const size = 2;
  static folly::Range<type const*> const values;
  static folly::Range<folly::StringPiece const*> const names;

  static char const* findName(type value);
  static bool findValue(char const* name, type* out);

};
}} // apache::thrift
namespace apache {
namespace thrift {
namespace detail {
template <>
struct TypeToInfo<
    ::apache::thrift::type_class::structure,
    ::test::fixtures::tablebased::TrivialTypesStruct> {
  static const ::apache::thrift::detail::TypeInfo typeInfo;
};
template <>
struct TypeToInfo<
    ::apache::thrift::type_class::structure,
    ::test::fixtures::tablebased::ContainerStruct> {
  static const ::apache::thrift::detail::TypeInfo typeInfo;
};
template <>
struct TypeToInfo<
    ::apache::thrift::type_class::variant,
    ::test::fixtures::tablebased::ExampleUnion> {
  static const ::apache::thrift::detail::TypeInfo typeInfo;
};
  template <>
    struct TypeToInfo<
        ::apache::thrift::type_class::enumeration,
        ::test::fixtures::tablebased::ExampleEnum> {
    static const ::apache::thrift::detail::TypeInfo typeInfo;
  };
}}} // namespace apache::thrift::detail
