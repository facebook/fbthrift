/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_
#define THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_ 1

#include <stdexcept>

#include <fatal/type/enum.h>
#include <fatal/type/search.h>
#include <fatal/type/variant_traits.h>
#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp2/reflection/container_traits.h>

namespace apache {
namespace thrift {
namespace detail {

template <>
struct dynamic_converter_impl<type_class::enumeration> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format format) {
    switch (format) {
      case dynamic_format::PORTABLE: {
        auto const s = TEnumTraits<T>::findName(input);
        if (!s) {
          throw std::invalid_argument("invalid enum value");
        }
        out = s;
        break;
      }
      case dynamic_format::JSON_1:
        out = static_cast<typename fatal::enum_traits<T>::int_type>(input);
        break;
      default:
        assert("to_dynamic: unsupported format" == nullptr);
        break;
    }
  }

  template <typename T>
  static void from_portable(T& out, folly::dynamic const& input) {
    auto const& value = input.asString();

    if (!TEnumTraits<T>::findValue(value.c_str(), &out)) {
      throw std::invalid_argument("unrecognized enum value");
    }
  }

  template <typename T>
  static void from_json_1(T& out, folly::dynamic const& input) {
    out = static_cast<T>(input.asInt());
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
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
  static void to(folly::dynamic& out, T const& input_, dynamic_format format) {
    thrift_list_traits_adapter<T const> input{input_};

    out = folly::dynamic::array;

    for (auto const& i : input) {
      folly::dynamic value(folly::dynamic::object);
      dynamic_converter_impl<ValueTypeClass>::to(value, i, format);
      out.push_back(std::move(value));
    }
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
    if (input.empty()) {
      return;
    }

    for (auto const& i : input) {
      out.emplace_back();
      dynamic_converter_impl<ValueTypeClass>::from(
          out.back(), i, format, adherence);
    }
  }
};

template <typename KeyTypeClass, typename MappedTypeClass>
struct dynamic_converter_impl<type_class::map<KeyTypeClass, MappedTypeClass>> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input_, dynamic_format format) {
    using traits = thrift_map_traits<T>;
    thrift_map_traits_adapter<T const> input{input_};

    out = folly::dynamic::object;

    for (auto const& i : input) {
      folly::dynamic key(folly::dynamic::object);
      dynamic_converter_impl<KeyTypeClass>::to(key, traits::key(i), format);
      dynamic_converter_impl<MappedTypeClass>::to(
          out[std::move(key)], traits::mapped(i), format);
    }
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
    using traits = thrift_map_traits<T>;

    if (input.empty()) {
      return;
    }

    for (auto const& i : input.items()) {
      typename traits::key_type key;
      dynamic_converter_impl<KeyTypeClass>::from(
          key, i.first, format, adherence);

      dynamic_converter_impl<MappedTypeClass>::from(
          out[std::move(key)], i.second, format, adherence);
    }
  }
};

template <typename ValueTypeClass>
struct dynamic_converter_impl<type_class::set<ValueTypeClass>> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input_, dynamic_format format) {
    thrift_set_traits_adapter<T const> input{input_};

    out = folly::dynamic::array;

    for (auto const& i : input) {
      folly::dynamic value(folly::dynamic::object);
      dynamic_converter_impl<ValueTypeClass>::to(value, i, format);
      out.push_back(std::move(value));
    }
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
    using traits = thrift_set_traits<T>;

    if (input.empty()) {
      return;
    }

    for (auto const& i : input) {
      typename traits::value_type value;
      dynamic_converter_impl<ValueTypeClass>::from(value, i, format, adherence);
      out.insert(std::move(value));
    }
  }
};

template <>
struct dynamic_converter_impl<type_class::variant> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format format) {
    using descriptors = typename fatal::variant_traits<T>::descriptors;

    out = folly::dynamic::object;

    fatal::scalar_search<descriptors, fatal::get_type::id>(
        input.getType(), [&](auto indexed) {
          using descriptor = decltype(fatal::tag_type(indexed));
          dynamic_converter_impl<typename descriptor::metadata::type_class>::to(
              out[fatal::enum_to_string(input.getType(), nullptr)],
              typename descriptor::getter()(input),
              format);
        });
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
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
      auto const& entry = i->second;
      bool const found = fatal::trie_find<
          typename id_traits::fields,
          fatal::get_type::name>(type.begin(), type.end(), [&](auto tag) {
        using field = decltype(fatal::tag_type(tag));
        using id = typename field::value;
        using descriptor =
            typename variant_traits::by_id::template descriptor<id>;

        variant_traits::by_id::template set<id>(out);
        dynamic_converter_impl<typename descriptor::metadata::type_class>::from(
            variant_traits::by_id::template get<id>(out),
            entry,
            format,
            adherence);
      });

      if (!found) {
        throw std::invalid_argument("unrecognized variant type");
      }
    }
  }
};

template <>
struct dynamic_converter_impl<type_class::structure> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format format) {
    out = folly::dynamic::object;
    fatal::foreach<typename reflect_struct<T>::members>([&](auto indexed) {
      using member = decltype(fatal::tag_type(indexed));
      using impl = dynamic_converter_impl<typename member::type_class>;

      static_assert(
          fatal::is_complete<impl>::value, "to_dynamic: unsupported type");

      if (member::optional::value == optionality::optional &&
          !member::is_set(input)) {
        return;
      }

      impl::to(
          out[folly::StringPiece(
              fatal::z_data<typename member::name>(),
              fatal::size<typename member::name>::value)],
          member::getter::ref(input),
          format);
    });
  }

  template <typename T>
  static void from(
      T& out,
      folly::dynamic const& input,
      dynamic_format format,
      format_adherence adherence) {
    for (auto const& i : input.items()) {
      auto const member_name = i.first.stringPiece();
      fatal::
          trie_find<typename reflect_struct<T>::members, fatal::get_type::name>(
              member_name.begin(), member_name.end(), [&](auto tag) {
                using member = decltype(fatal::tag_type(tag));
                member::mark_set(out, true);
                dynamic_converter_impl<typename member::type_class>::from(
                    member::getter::ref(out), i.second, format, adherence);
              });
    }
  }
};

template <>
struct dynamic_converter_impl<type_class::string> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format) {
    out = input;
  }

  static void from(
      std::string& out,
      folly::dynamic const& input,
      dynamic_format,
      format_adherence) {
    out = input.asString();
  }

  template <typename T>
  static void
  from(T& out, folly::dynamic const& input, dynamic_format, format_adherence) {
    out = input.asString();
  }
};

template <>
struct dynamic_converter_impl<type_class::binary> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format) {
    out = folly::to<std::string>(input);
  }

  static void from(
      std::string& out,
      folly::dynamic const& input,
      dynamic_format,
      format_adherence) {
    out = input.asString();
  }

  template <typename T>
  static void
  from(T& out, folly::dynamic const& input, dynamic_format, format_adherence) {
    out = input.asString();
  }
};

template <>
struct dynamic_converter_impl<type_class::floating_point> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format) {
    out = static_cast<double>(input);
  }

  template <typename T>
  static void
  from(T& out, folly::dynamic const& input, dynamic_format, format_adherence) {
    out = static_cast<T>(input.asDouble());
  }
};

template <>
struct dynamic_converter_impl<type_class::integral> {
  template <typename T>
  static void to(folly::dynamic& out, T const& input, dynamic_format) {
    out = input;
  }

  static void from(
      bool& out,
      folly::dynamic const& input,
      dynamic_format,
      format_adherence) {
    out = input.asBool();
  }

  template <typename T>
  static void
  from(T& out, folly::dynamic const& input, dynamic_format, format_adherence) {
    out = static_cast<T>(input.asInt());
  }
};

} // namespace detail
} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_FOLLY_DYNAMIC_INL_POST_H_
