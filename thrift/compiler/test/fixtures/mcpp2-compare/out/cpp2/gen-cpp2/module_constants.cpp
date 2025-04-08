/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/mcpp2-compare/src/module.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */

#include "thrift/compiler/test/fixtures/mcpp2-compare/gen-cpp2/module_constants.h"

#include <thrift/lib/cpp2/gen/module_constants_cpp.h>

#include "thrift/compiler/test/fixtures/mcpp2-compare/gen-cpp2/includes_constants.h"

namespace some::valid::ns {
namespace module_constants {









::std::vector<bool> const& aList() {
  static folly::Indestructible<::std::vector<bool>> const instance{ std::initializer_list<bool>{ true,
  false } };
  return *instance;
}

::std::map<::std::string, ::std::int32_t> const& anEmptyMap() {
  static folly::Indestructible<::std::map<::std::string, ::std::int32_t>> const instance{ std::initializer_list<::std::map<::std::string, ::std::int32_t>::value_type>{  } };
  return *instance;
}

::std::map<::std::int32_t, ::std::string> const& aMap() {
  static folly::Indestructible<::std::map<::std::int32_t, ::std::string>> const instance{ std::initializer_list<::std::map<::std::int32_t, ::std::string>::value_type>{ { static_cast<::std::int32_t>(1), apache::thrift::StringTraits<std::string>::fromStringLiteral("foo") },
  { static_cast<::std::int32_t>(2), apache::thrift::StringTraits<std::string>::fromStringLiteral("bar") } } };
  return *instance;
}

::std::set<::std::string> const& aSet() {
  static folly::Indestructible<::std::set<::std::string>> const instance{ std::initializer_list<::std::string>{ apache::thrift::StringTraits<std::string>::fromStringLiteral("foo"),
  apache::thrift::StringTraits<std::string>::fromStringLiteral("bar") } };
  return *instance;
}

::std::vector<::std::vector<::std::int32_t>> const& aListOfLists() {
  static folly::Indestructible<::std::vector<::std::vector<::std::int32_t>>> const instance{ std::initializer_list<::std::vector<::std::int32_t>>{ std::initializer_list<::std::int32_t>{ static_cast<::std::int32_t>(1),
  static_cast<::std::int32_t>(3),
  static_cast<::std::int32_t>(5),
  static_cast<::std::int32_t>(7),
  static_cast<::std::int32_t>(9) },
  std::initializer_list<::std::int32_t>{ static_cast<::std::int32_t>(2),
  static_cast<::std::int32_t>(4),
  static_cast<::std::int32_t>(8),
  static_cast<::std::int32_t>(10),
  static_cast<::std::int32_t>(12) } } };
  return *instance;
}

::std::vector<::std::map<::std::string, ::std::int32_t>> const& states() {
  static folly::Indestructible<::std::vector<::std::map<::std::string, ::std::int32_t>>> const instance{ std::initializer_list<::std::map<::std::string, ::std::int32_t>>{ std::initializer_list<::std::map<::std::string, ::std::int32_t>::value_type>{ { apache::thrift::StringTraits<std::string>::fromStringLiteral("San Diego"), static_cast<::std::int32_t>(3211000) },
  { apache::thrift::StringTraits<std::string>::fromStringLiteral("Sacramento"), static_cast<::std::int32_t>(479600) },
  { apache::thrift::StringTraits<std::string>::fromStringLiteral("SF"), static_cast<::std::int32_t>(837400) } },
  std::initializer_list<::std::map<::std::string, ::std::int32_t>::value_type>{ { apache::thrift::StringTraits<std::string>::fromStringLiteral("New York"), static_cast<::std::int32_t>(8406000) },
  { apache::thrift::StringTraits<std::string>::fromStringLiteral("Albany"), static_cast<::std::int32_t>(98400) } } } };
  return *instance;
}

::std::vector<::some::valid::ns::MyEnumA> const& AConstList() {
  static folly::Indestructible<::std::vector<::some::valid::ns::MyEnumA>> const instance{ std::initializer_list<::some::valid::ns::MyEnumA>{  ::some::valid::ns::MyEnumA::fieldA,
   ::some::valid::ns::MyEnumA::fieldB,
  static_cast< ::some::valid::ns::MyEnumA>(3) } };
  return *instance;
}


::std::vector<::std::int32_t> const& ListOfIntsFromEnums() {
  static folly::Indestructible<::std::vector<::std::int32_t>> const instance{ std::initializer_list<::std::int32_t>{ static_cast<::std::int32_t>(2),
  static_cast<::std::int32_t>(1) } };
  return *instance;
}




::std::string_view _fbthrift_schema_9a58f62a41e2535e() {
  return "";
}
::folly::Range<const ::std::string_view*> _fbthrift_schema_9a58f62a41e2535e_includes() {
  return {};
}

} // namespace module_constants
} // namespace some::valid::ns
