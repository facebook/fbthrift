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
#ifndef THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_
#define THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_ 1

#include <thrift/lib/cpp2/fatal/container_traits.h>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/variant_traits.h>

#include <stdexcept>

namespace apache { namespace thrift { namespace detail {

template <>
struct dynamic_converter_impl<type_class::enumeration> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    switch (format) {
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
        assert("to_dynamic: unsupported format" == nullptr);
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
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
    switch (adherence) {
      case format_adherence::STRICT:
        switch (format) {
          case dynamic_format::PORTABLE:
            from_portable(out, input);
            break;
          case dynamic_format::JSON_1:
            from_json_1(out, input);
            break;
          default:
            assert("from_dynamic (STRICT): unsupported format" == nullptr);
            break;
        }
        break;

      case format_adherence::LENIENT:
        switch (format) {
          case dynamic_format::PORTABLE:
          case dynamic_format::JSON_1:
            if (input.isInt()) {
              from_json_1(out, input);
            } else {
              from_portable(out, input);
            }
            break;
          default:
            assert("from_dynamic (LENIENT): unsupported format" == nullptr);
            break;
        }
        break;

      default:
        assert("from_dynamic: unsupported format adherence" == nullptr);
        break;
    }
  }
};

template <typename ValueTypeClass>
struct dynamic_converter_impl<type_class::list<ValueTypeClass>> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    using traits = thrift_list_traits<T>;

    out = folly::dynamic::array;

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      folly::dynamic value(folly::dynamic::object);
      dynamic_converter_impl<ValueTypeClass>::to(value, *i, format);
      out.push_back(std::move(value));
    }
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
    for (auto const &i: input) {
      out.emplace_back();
      dynamic_converter_impl<ValueTypeClass>::from(
        out.back(), i, format, adherence
      );
    }
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct dynamic_converter_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    using traits = thrift_map_traits<T>;

    out = folly::dynamic::object;

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      folly::dynamic key(folly::dynamic::object);
      dynamic_converter_impl<KeyTypeClass>::to(key, traits::key(i), format);
      dynamic_converter_impl<MappedTypeClass>::to(
        out[std::move(key)], traits::mapped(i), format
      );
    }
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
    using traits = thrift_map_traits<T>;

    for (auto const &i: input.items()) {
      typename traits::key_type key;
      dynamic_converter_impl<KeyTypeClass>::from(
        key, i.first, format, adherence
      );

      dynamic_converter_impl<MappedTypeClass>::from(
        out[std::move(key)],
        i.second,
        format,
        adherence
      );
    }
  }
};

template <typename ValueTypeClass>
struct dynamic_converter_impl<type_class::set<ValueTypeClass>> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    using traits = thrift_set_traits<T>;

    out = folly::dynamic::array;

    for (auto i = traits::begin(input), e = traits::end(input); i != e; ++i) {
      folly::dynamic value(folly::dynamic::object);
      dynamic_converter_impl<ValueTypeClass>::to(value, *i, format);
      out.push_back(std::move(value));
    }
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
    using traits = thrift_set_traits<T>;

    for (auto const &i: input) {
      typename traits::value_type value;
      dynamic_converter_impl<ValueTypeClass>::from(value, i, format, adherence);
      out.emplace(std::move(value));
    }
  }
};

struct to_dynamic_variant_visitor {
  template <typename Descriptor, std::size_t Index, typename T>
  void operator ()(
    fatal::indexed<Descriptor, Index>,
    folly::dynamic &out,
    T const &input,
    dynamic_format format
  ) const {
    dynamic_converter_impl<typename Descriptor::metadata::type_class>::to(
      out[fatal::enum_to_string(input.getType())],
      typename Descriptor::getter()(input),
      format
    );
  }
};

template <typename VariantTraits>
struct from_dynamic_variant_visitor {
  template <typename T, typename Field>
  void operator ()(
    fatal::tag<Field>,
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) const {
    using id = typename Field::value;
    using descriptor = typename VariantTraits::by_id::template descriptor<id>;

    VariantTraits::by_id::template set<id>(out);
    dynamic_converter_impl<typename descriptor::metadata::type_class>::from(
      VariantTraits::by_id::template get<id>(out), input, format, adherence
    );
  }
};

template <>
struct dynamic_converter_impl<type_class::variant> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    using namespace fatal;

    out = folly::dynamic::object;

    sorted_search<
      sort<typename variant_traits<T>::descriptors, less, get_type::id>,
      get_type::id::apply
    >(
      input.getType(),
      to_dynamic_variant_visitor(),
      out,
      input,
      format
    );
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
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
      bool const found = fatal::trie_find<
        typename id_traits::fields, fatal::get_type::name
      >(
        type.begin(), type.end(),
        from_dynamic_variant_visitor<variant_traits>(),
        out, i->second,
        format, adherence
      );

      if (!found) {
        throw std::invalid_argument("unrecognized variant type");
      }
    }
  }
};

struct to_dynamic_struct_visitor {
  template <typename MemberInfo, std::size_t Index, typename T>
  void operator ()(
    fatal::indexed<MemberInfo, Index>,
    folly::dynamic &out,
    T const &input,
    dynamic_format format
  ) const {
    using impl = dynamic_converter_impl<typename MemberInfo::type_class>;

    static_assert(
      fatal::is_complete<impl>::value,
      "to_dynamic: unsupported type"
    );

    if (MemberInfo::optional::value == optionality::optional &&
        !MemberInfo::is_set(input)) {
      return;
    }

    impl::to(
      out[folly::StringPiece(
        fatal::z_data<typename MemberInfo::name>(),
        fatal::size<typename MemberInfo::name>::value
      )],
      MemberInfo::getter::ref(input),
      format
    );
  }
};

struct from_dynamic_struct_visitor {
  using required = std::integral_constant<optionality, optionality::required>;

  template <typename Owner, typename Getter, typename Optionality>
  typename std::enable_if<Optionality::value == optionality::required>::type
  assign_is_set(Owner &, bool) const {}

  template <typename Owner, typename Getter, typename Optionality>
  typename std::enable_if<Optionality::value != optionality::required>::type
  assign_is_set(Owner &owner, bool v) const { Getter::ref(owner.__isset) = v; }

  template <typename Member, typename T>
  void operator ()(
    fatal::tag<Member>,
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) const {
    using rgetter = typename Member::getter;
    assign_is_set<T, rgetter, typename Member::optional>(out, true);
    dynamic_converter_impl<typename Member::type_class>::from(
      rgetter::ref(out), input, format, adherence
    );
  }
};

template <>
struct dynamic_converter_impl<type_class::structure> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format format) {
    out = folly::dynamic::object;
    fatal::foreach<typename reflect_struct<T>::members>(
      to_dynamic_struct_visitor(),
      out, input, format
    );
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format format,
    format_adherence adherence
  ) {
    for (auto const &i: input.items()) {
      using namespace fatal;
      auto const member = i.first.stringPiece();
      trie_find<typename reflect_struct<T>::members, get_type::name>(
        member.begin(), member.end(),
        from_dynamic_struct_visitor(),
        out, i.second,
        format, adherence
      );
    }
  }
};

template <>
struct dynamic_converter_impl<type_class::string> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format) {
    out = input;
  }

  static void from(
    std::string &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = input.asString();
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = input.asString();
  }
};

template <>
struct dynamic_converter_impl<type_class::binary> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format) {
    out = folly::to<std::string>(input);
  }

  static void from(
    std::string &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = input.asString();
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = input.asString();
  }
};

template <>
struct dynamic_converter_impl<type_class::floating_point> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format) {
    out = static_cast<double>(input);
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = static_cast<T>(input.asDouble());
  }
};

template <>
struct dynamic_converter_impl<type_class::integral> {
  template <typename T>
  static void to(folly::dynamic &out, T const &input, dynamic_format) {
    out = input;
  }

  static void from(
    bool &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = input.asBool();
  }

  template <typename T>
  static void from(
    T &out,
    folly::dynamic const &input,
    dynamic_format,
    format_adherence
  ) {
    out = static_cast<T>(input.asInt());
  }
};

}}} // apache::thrift::detail

#endif // THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_
