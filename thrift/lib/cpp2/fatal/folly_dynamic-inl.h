/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_
#define THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <fatal/type/enum.h>
#include <fatal/type/variant_traits.h>

#include <initializer_list>
#include <stdexcept>

namespace apache { namespace thrift { namespace detail {

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::enumeration, Strict> {
  static_assert(
    Format == dynamic_format::PORTABLE || Format == dynamic_format::JSON_1,
    "to_dynamic/from_dynamic: unsupported format"
  );

  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    switch (Format) {
      case dynamic_format::PORTABLE: {
          auto const s = fatal::enum_to_string(input);
          if (!s) {
            throw std::invalid_argument("invalid enum value");
          }
          out = s;
        }
        break;
      case dynamic_format::JSON_1:
        out = static_cast<typename fatal::enum_traits<T>::int_type>(input);
        break;
      default:
        assert(false);
        break;
    }
  }

  template <typename T>
  static void from_portable(T &out, folly::dynamic const &input) {
    auto const &value = input.asString();
    out = fatal::enum_traits<T>::parse(value.begin(), value.end());
  }

  template <typename T>
  static void from_json_1(T &out, folly::dynamic const &input) {
    out = static_cast<T>(input.asInt());
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    if (Strict) {
      switch (Format) {
        case dynamic_format::PORTABLE:
          from_portable(out, input);
          break;
        case dynamic_format::JSON_1:
          from_json_1(out, input);
          break;
        default:
          assert(false);
          break;
      }
    } else {
      switch (Format) {
        case dynamic_format::PORTABLE:
        case dynamic_format::JSON_1:
          if (input.isInt()) {
            from_json_1(out, input);
          } else {
            from_portable(out, input);
          }
          break;
        default:
          assert(false);
          break;
      }
    }
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::list, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    using traits = thrift_list_traits<T>;

    out = std::initializer_list<folly::dynamic>{};

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      out.push_back(to_dynamic<Format>(*i));
    }
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    using traits = thrift_list_traits<T>;

    for (auto const &i: input) {
      out.push_back(
        from_dynamic<Format, typename traits::value_type, Strict>(i)
      );
    }
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::map, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    using traits = thrift_map_traits<T>;

    out = folly::dynamic::object;

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      to_dynamic<Format>(
        out[to_dynamic<Format>(traits::key(i))],
        traits::mapped(i)
      );
    }
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    using traits = thrift_map_traits<T>;

    for (auto const &i: input.items()) {
      from_dynamic<Format, Strict>(
        out[from_dynamic<Format, typename traits::key_type, Strict>(i.first)],
        i.second
      );
    }
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::set, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    using traits = thrift_set_traits<T>;

    out = std::initializer_list<folly::dynamic>{};

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      out.push_back(to_dynamic<Format>(*i));
    }
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    using traits = thrift_set_traits<T>;

    for (auto const &i: input) {
      out.insert(
        from_dynamic<Format, typename traits::value_type, Strict>(i)
      );
    }
  }
};

template <dynamic_format Format>
struct to_dynamic_variant_visitor {
  template <
    typename Id, typename Descriptor, std::size_t Index,
    typename Needle, typename T
  >
  void operator ()(
    fatal::indexed_type_pair_tag<Id, Descriptor, Index>,
    Needle,
    folly::dynamic &out,
    T const &input
  ) const {
    to_dynamic<Format>(
      out[fatal::enum_to_string(input.getType())],
      typename Descriptor::getter()(input)
    );
  }
};

template <dynamic_format Format, bool Strict, typename VariantTraits>
struct from_dynamic_variant_visitor {
  template <typename T, typename IdName>
  void operator ()(
    fatal::type_tag<IdName>,
    T &out,
    folly::dynamic const &input
  ) const {
    using id_traits = fatal::enum_traits<typename VariantTraits::id>;
    using id = typename id_traits::name_to_value::template get<IdName>;
    using type = typename VariantTraits::by_id::template type<id>;

    VariantTraits::by_id::template set<id>(
      out,
      from_dynamic<Format, type, Strict>(input)
    );
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::variant, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    using traits = fatal::variant_traits<T>;

    out = folly::dynamic::object;

    traits::by_id::map::template binary_search<>::exact(
      input.getType(),
      to_dynamic_variant_visitor<Format>(),
      out,
      input
    );
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    using variant_traits = fatal::variant_traits<T>;
    using id_traits = fatal::enum_traits<typename variant_traits::id>;

    if (!input.isObject() || input.size() > 1) {
      throw std::invalid_argument("unexpected additional fields for a variant");
    }

    auto items = input.items();
    auto i = items.begin();
    if (i == items.end()) {
      variant_traits::clear(out);
    } else {
      auto const type = i->first.stringPiece();
      bool const found = id_traits::prefix_tree::template match<>::exact(
        type.begin(), type.end(),
        from_dynamic_variant_visitor<Format, Strict, variant_traits>(),
        out, i->second
      );

      if (!found) {
        throw std::invalid_argument("unrecognized variant type");
      }
    }
  }
};

template <dynamic_format Format>
struct to_dynamic_struct_visitor {
  template <typename MemberInfo, std::size_t Index, typename T>
  void operator ()(
    fatal::indexed_type_tag<MemberInfo, Index>,
    folly::dynamic &out,
    T const &input
  ) const {
    using impl = dynamic_converter_impl<
      Format, MemberInfo::category::value, true
    >;

    static_assert(
      fatal::is_complete<impl>::value,
      "to_dynamic: unsupported type"
    );

    impl::to(
      out[folly::StringPiece(MemberInfo::name::data(), MemberInfo::name::size)],
      MemberInfo::getter::ref(input)
    );
  }
};

template <dynamic_format Format, bool Strict>
struct from_dynamic_struct_visitor {
  template <typename Member, typename T>
  void operator ()(
    fatal::type_tag<Member>,
    T &out,
    folly::dynamic const &input
  ) const {
    using getter = typename reflect_struct<T>::getters::template get<Member>;
    from_dynamic<Format, Strict>(getter::ref(out), input);
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::structure, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    out = folly::dynamic::object;
    reflect_struct<T>::members::mapped::foreach(
      to_dynamic_struct_visitor<Format>(),
      out, input
    );
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    using trie = typename reflect_struct<T>::members::keys::template apply<
      fatal::build_type_prefix_tree<>::from
    >;

    for (auto const &i: input.items()) {
      auto const member = i.first.stringPiece();
      trie::template match<>::exact(
        member.begin(), member.end(),
        from_dynamic_struct_visitor<Format, Strict>(),
        out, i.second
      );
    }
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::string, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    out = input;
  }

  static void from(std::string &out, folly::dynamic const &input) {
    out = input.asString().toStdString();
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    out = input.asString();
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::floating_point, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    out = static_cast<double>(input);
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    out = static_cast<T>(input.asDouble());
  }
};

template <dynamic_format Format, bool Strict>
struct dynamic_converter_impl<Format, thrift_category::integral, Strict> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input) {
    out = input;
  }

  static void from(bool &out, folly::dynamic const &input) {
    out = input.asBool();
  }

  template <typename T>
  static void from(T &out, folly::dynamic const &input) {
    out = static_cast<T>(input.asInt());
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_FOLLY_DYNAMIC_INL_H_
