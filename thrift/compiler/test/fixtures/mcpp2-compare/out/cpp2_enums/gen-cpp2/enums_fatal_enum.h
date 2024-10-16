/**
 * Autogenerated by Thrift for thrift/compiler/test/fixtures/mcpp2-compare/src/enums.thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated @nocommit
 */
#pragma once

#include "thrift/compiler/test/fixtures/mcpp2-compare/gen-cpp2/enums_types.h"
#include "thrift/compiler/test/fixtures/mcpp2-compare/gen-cpp2/enums_fatal.h"

#include <fatal/type/enum.h>

#include <type_traits>

namespace facebook::ns::qwerty {

namespace __fbthrift_refl {
namespace __fbthrift_refl_impl = ::apache::thrift::detail::reflection_impl;

class AnEnumA_enum_traits {
 public:
  using type = ::facebook::ns::qwerty::AnEnumA;

 private:
  struct __fbthrift_value_FIELDA {
    using name = __fbthrift_strings_enums::__fbthrift_hash_add540b98778029e7e62e524687274b838fd038b831b1885b93d5ab01ef056e6;
    using value = std::integral_constant<type, type::FIELDA>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_member {
    using FIELDA = __fbthrift_value_FIELDA;
  };

 public:
  using name = __fbthrift_strings_enums::AnEnumA;
  using member = __fbthrift_member;
  using fields = ::fatal::list<
      member::FIELDA
  >;

  using annotations = __fbthrift_refl_impl::no_annotations;

  static char const *to_string(type e, char const *fallback) {
    switch (e) {
      case type::FIELDA: return "FIELDA";
      default: return fallback;
    }
  }
};

class AnEnumB_enum_traits {
 public:
  using type = ::facebook::ns::qwerty::AnEnumB;

 private:
  struct __fbthrift_value_FIELDA {
    using name = __fbthrift_strings_enums::__fbthrift_hash_add540b98778029e7e62e524687274b838fd038b831b1885b93d5ab01ef056e6;
    using value = std::integral_constant<type, type::FIELDA>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_value_FIELDB {
    using name = __fbthrift_strings_enums::__fbthrift_hash_148e782df34993fa930a9b389d5a4e2e92f13610cf6adbeab2abf6e095f5ef89;
    using value = std::integral_constant<type, type::FIELDB>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_member {
    using FIELDA = __fbthrift_value_FIELDA;
    using FIELDB = __fbthrift_value_FIELDB;
  };

 public:
  using name = __fbthrift_strings_enums::AnEnumB;
  using member = __fbthrift_member;
  using fields = ::fatal::list<
      member::FIELDA,
      member::FIELDB
  >;

  using annotations = __fbthrift_refl_impl::no_annotations;

  static char const *to_string(type e, char const *fallback) {
    switch (e) {
      case type::FIELDA: return "FIELDA";
      case type::FIELDB: return "FIELDB";
      default: return fallback;
    }
  }
};

class AnEnumC_enum_traits {
 public:
  using type = ::facebook::ns::qwerty::AnEnumC;

 private:
  struct __fbthrift_value_FIELDC {
    using name = __fbthrift_strings_enums::__fbthrift_hash_c907b51dbf720f8a71d2fe895168681ecaffaeebee9402b24e9da11816eda10e;
    using value = std::integral_constant<type, type::FIELDC>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_member {
    using FIELDC = __fbthrift_value_FIELDC;
  };

 public:
  using name = __fbthrift_strings_enums::AnEnumC;
  using member = __fbthrift_member;
  using fields = ::fatal::list<
      member::FIELDC
  >;

  using annotations = __fbthrift_refl_impl::no_annotations;

  static char const *to_string(type e, char const *fallback) {
    switch (e) {
      case type::FIELDC: return "FIELDC";
      default: return fallback;
    }
  }
};

class AnEnumD_enum_traits {
 public:
  using type = ::facebook::ns::qwerty::AnEnumD;

 private:
  struct __fbthrift_value_FIELDD {
    using name = __fbthrift_strings_enums::__fbthrift_hash_40d9038a74e2f4da3d23c9054a5c003163af248d9a0c6133ac130acfc5d4122d;
    using value = std::integral_constant<type, type::FIELDD>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_member {
    using FIELDD = __fbthrift_value_FIELDD;
  };

 public:
  using name = __fbthrift_strings_enums::AnEnumD;
  using member = __fbthrift_member;
  using fields = ::fatal::list<
      member::FIELDD
  >;

  using annotations = __fbthrift_refl_impl::no_annotations;

  static char const *to_string(type e, char const *fallback) {
    switch (e) {
      case type::FIELDD: return "FIELDD";
      default: return fallback;
    }
  }
};

class AnEnumE_enum_traits {
 public:
  using type = ::facebook::ns::qwerty::AnEnumE;

 private:
  struct __fbthrift_value_FIELDA {
    using name = __fbthrift_strings_enums::__fbthrift_hash_add540b98778029e7e62e524687274b838fd038b831b1885b93d5ab01ef056e6;
    using value = std::integral_constant<type, type::FIELDA>;
    using annotations = __fbthrift_refl_impl::no_annotations;
  };

  struct __fbthrift_member {
    using FIELDA = __fbthrift_value_FIELDA;
  };

 public:
  using name = __fbthrift_strings_enums::AnEnumE;
  using member = __fbthrift_member;
  using fields = ::fatal::list<
      member::FIELDA
  >;

  using annotations = __fbthrift_refl_impl::no_annotations;

  static char const *to_string(type e, char const *fallback) {
    switch (e) {
      case type::FIELDA: return "FIELDA";
      default: return fallback;
    }
  }
};

} // __fbthrift_refl

FATAL_REGISTER_ENUM_TRAITS(
  __fbthrift_refl::AnEnumA_enum_traits,
  ::apache::thrift::detail::type_common_metadata_impl<
      enums_tags::module,
      ::apache::thrift::reflected_annotations<__fbthrift_refl::AnEnumA_enum_traits::annotations>,
      static_cast<::apache::thrift::legacy_type_id_t>(1369557940062611496ull)
  >
);
FATAL_REGISTER_ENUM_TRAITS(
  __fbthrift_refl::AnEnumB_enum_traits,
  ::apache::thrift::detail::type_common_metadata_impl<
      enums_tags::module,
      ::apache::thrift::reflected_annotations<__fbthrift_refl::AnEnumB_enum_traits::annotations>,
      static_cast<::apache::thrift::legacy_type_id_t>(18439895483441837800ull)
  >
);
FATAL_REGISTER_ENUM_TRAITS(
  __fbthrift_refl::AnEnumC_enum_traits,
  ::apache::thrift::detail::type_common_metadata_impl<
      enums_tags::module,
      ::apache::thrift::reflected_annotations<__fbthrift_refl::AnEnumC_enum_traits::annotations>,
      static_cast<::apache::thrift::legacy_type_id_t>(13460746706677500392ull)
  >
);
FATAL_REGISTER_ENUM_TRAITS(
  __fbthrift_refl::AnEnumD_enum_traits,
  ::apache::thrift::detail::type_common_metadata_impl<
      enums_tags::module,
      ::apache::thrift::reflected_annotations<__fbthrift_refl::AnEnumD_enum_traits::annotations>,
      static_cast<::apache::thrift::legacy_type_id_t>(620540081711658024ull)
  >
);
FATAL_REGISTER_ENUM_TRAITS(
  __fbthrift_refl::AnEnumE_enum_traits,
  ::apache::thrift::detail::type_common_metadata_impl<
      enums_tags::module,
      ::apache::thrift::reflected_annotations<__fbthrift_refl::AnEnumE_enum_traits::annotations>,
      static_cast<::apache::thrift::legacy_type_id_t>(2060347478177142664ull)
  >
);
} // namespace facebook::ns::qwerty
namespace apache::thrift::detail {
template <>
struct ExtraEnumTraits<::facebook::ns::qwerty::AnEnumA> {
  using module = ::facebook::ns::qwerty::enums_tags::module;
};
template<>
inline constexpr bool kHasExtraEnumTraits<::facebook::ns::qwerty::AnEnumA> = true;
template <>
struct ExtraEnumTraits<::facebook::ns::qwerty::AnEnumB> {
  using module = ::facebook::ns::qwerty::enums_tags::module;
};
template<>
inline constexpr bool kHasExtraEnumTraits<::facebook::ns::qwerty::AnEnumB> = true;
template <>
struct ExtraEnumTraits<::facebook::ns::qwerty::AnEnumC> {
  using module = ::facebook::ns::qwerty::enums_tags::module;
};
template<>
inline constexpr bool kHasExtraEnumTraits<::facebook::ns::qwerty::AnEnumC> = true;
template <>
struct ExtraEnumTraits<::facebook::ns::qwerty::AnEnumD> {
  using module = ::facebook::ns::qwerty::enums_tags::module;
};
template<>
inline constexpr bool kHasExtraEnumTraits<::facebook::ns::qwerty::AnEnumD> = true;
template <>
struct ExtraEnumTraits<::facebook::ns::qwerty::AnEnumE> {
  using module = ::facebook::ns::qwerty::enums_tags::module;
};
template<>
inline constexpr bool kHasExtraEnumTraits<::facebook::ns::qwerty::AnEnumE> = true;
}
