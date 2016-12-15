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

#include <thrift/test/gen-cpp/frozen1_constants.h>
#include <thrift/test/gen-cpp/frozen1_types.h>
#include <thrift/test/gen-cpp2/frozen1_constants.h>
#include <thrift/test/gen-cpp2/frozen1_fatal_types.h>

#include <thrift/lib/cpp/Frozen.h>
#include <thrift/lib/cpp2/fatal/reflection.h>

#include <fatal/test/tools.h>
#include <fatal/type/foreach.h>

#include <fstream>
#include <iomanip>
#include <unordered_map>

#include <cassert>

namespace apache {
namespace thrift {

template <typename T>
std::vector<char> serialize(T const &data) {
  std::vector<char> buffer(apache::thrift::frozenSize(data));
  auto const begin = reinterpret_cast<apache::thrift::byte *>(buffer.data());
  auto p = begin;
  apache::thrift::freeze(data, p);
  assert(p == std::next(begin, buffer.size()));
  return buffer;
}

template <typename T, typename TBuffer>
T deserialize(TBuffer &buffer) {
  auto result = apache::thrift::thaw(
    *reinterpret_cast<apache::thrift::Frozen<T> const *>(buffer.data())
  );

  // this should be done before thawing but thrift::frozen doesn't
  // provide an API for measuring needed space without an instance
  // nor a thaw() version that receives available data size
  assert(buffer.size() == apache::thrift::frozenSize(result));

  return result;
}

// value_path
struct value_path {
  template <typename... Args>
  value_path(bool is_reference, Args &&...path):
    path_(folly::to<std::string>(std::forward<Args>(path)...)),
    is_reference_(is_reference)
  {}

  template <typename... Args>
  value_path clone(Args &&...suffix) const {
    return add(is_reference_, std::forward<Args>(suffix)...);
  }

  template <typename... Args>
  value_path add(bool is_reference, Args &&...suffix) const {
    return value_path(
      is_reference,
      path_, is_reference_ ? "->" : ".", std::forward<Args>(suffix)...
    );
  }

  std::string deref() const {
    return folly::to<std::string>(is_reference_ ? "*" : "", path_);
  }

  template <typename... Args>
  std::string member(Args &&...args) const {
    return *clone(std::forward<Args>(args)...);
  }

  std::string const &operator *() const { return path_; }

private:
  std::string const path_;
  bool const is_reference_;
};


// isset_setter //
template <typename = type_class::structure>
struct isset_setter {
  template <typename Cpp1, typename Cpp2>
  static void set(Cpp1 &, Cpp2 const &, std::ofstream &) {}
};

template <>
struct isset_setter<type_class::structure> {
  template <typename Cpp1, typename Cpp2>
  static void set(Cpp1 &instance, Cpp2 const &reference, std::ofstream &out) {
    fatal::foreach<typename reflect_struct<Cpp2>::members>(
      isset_setter(), instance, reference, out
    );
  }

  template <typename Member, std::size_t Index, typename Cpp1, typename Cpp2>
  void operator ()(
    fatal::indexed<Member, Index>,
    Cpp1 &instance,
    Cpp2 const &reference,
    std::ofstream &out
  ) const {
    m<Member>(
      instance, reference, out,
      std::integral_constant<
        bool,
        Member::optional::value == optionality::required
      >()
    );
  }

  template <typename Member, typename Cpp1, typename Cpp2>
  void R(
    Cpp1 &instance,
    Cpp2 const &reference,
    std::ofstream &out,
    std::true_type
  ) const {
    isset_setter<type_class::structure>::set(
      Member::getter::ref(instance),
      Member::getter::ref(reference),
      out
    );
  }

  template <typename Member, typename Cpp1, typename Cpp2>
  void R(Cpp1 &, Cpp2 const &, std::ofstream &, std::false_type) const {}

  template <typename Member, typename Cpp1, typename Cpp2>
  void r(Cpp1 &instance, Cpp2 const &reference, std::ofstream &out) const {
    R<Member>(
      instance, reference, out,
      std::integral_constant<
        bool,
        std::is_same<type_class::structure, typename Member::type_class>::value
      >()
    );
  }

  template <typename Member, typename Cpp1, typename Cpp2>
  void m(
    Cpp1 &instance,
    Cpp2 const &reference,
    std::ofstream &out,
    std::false_type
  ) const {
    if (Member::is_set(reference)) {
      Member::mark_set(instance, true);
      r<Member>(instance, reference, out);
    }
  }

  template <typename Member, typename Cpp1, typename Cpp2>
  void m(
    Cpp1 &instance,
    Cpp2 const &reference,
    std::ofstream &out,
    std::true_type
  ) const {
    r<Member>(instance, reference, out);
  }
};

// TODO: MUST ALSO FOLLOW CONTAINERS: view.somestructvector[0].__isset.a
// isset_test //
template <typename = type_class::structure>
struct isset_test {
  template <typename Cpp2>
  static void gen(Cpp2 const &, value_path const &, std::ofstream &) {}
};

template <>
struct isset_test<type_class::structure> {
  template <typename Cpp2>
  static void gen(
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out
  ) {
    fatal::foreach<typename reflect_struct<Cpp2>::members>(
      isset_test(), reference, path, out
    );
  }

  template <typename Member, std::size_t Index, typename Cpp2>
  void operator ()(
    fatal::indexed<Member, Index>,
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out
  ) const {
    m<Member>(
      reference, path, out,
      std::integral_constant<
        bool,
        Member::optional::value == optionality::required
      >()
    );
  }

  template <typename Member, typename Cpp2>
  void R(
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out,
    std::true_type
  ) const {
    isset_test<typename Member::type_class>::gen(
      Member::getter::ref(reference),
      path.add(true, fatal::z_data<typename Member::name>()),
      out
    );
  }

  template <typename Member, typename Cpp2>
  void R(
    Cpp2 const &,
    value_path const &,
    std::ofstream &,
    std::false_type
  ) const {}

  template <typename Member, typename Cpp2>
  void r(
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out
  ) const {
    R<Member>(
      reference, path, out,
      std::integral_constant<
        bool,
        std::is_same<type_class::structure, typename Member::type_class>::value
      >()
    );
  }

  template <typename Member, typename Cpp2>
  void m(
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out,
    std::false_type
  ) const {
    out << "  EXPECT_EQ(" << std::boolalpha << Member::is_set(reference)
      << ", " << path.member("__isset->")
      << fatal::z_data<typename Member::name>() << ");\n";

    r<Member>(reference, path, out);
  }

  template <typename Member, typename Cpp2>
  void m(
    Cpp2 const &reference,
    value_path const &path,
    std::ofstream &out,
    std::true_type
  ) const {
    r<Member>(reference, path, out);
  }
};

// value_test //
struct value_test_context {
  std::size_t id() { return id_++; }

private:
  std::size_t id_ = 0;
};

template <typename Cpp2, typename>
struct value_test {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &,
    std::ofstream &out
  ) {
    out << indentation << "EXPECT_EQ(";
    if (std::is_same<bool, Cpp2>::value) {
      out << std::boolalpha << frozen;
    } else if (std::is_integral<Cpp2>::value && sizeof(Cpp2) == 1) {
      out << "\'\\x" << fatal::least_significant_hex_digit(frozen >> 4)
        << fatal::least_significant_hex_digit(frozen) << '\'';
    } else {
      out << frozen;
    }
    out << ", " << path.deref() << ");\n";
  }
};

template <typename Cpp2>
struct value_test<Cpp2, type_class::floating_point> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &,
    std::ofstream &out
  ) {
    out << indentation << "EXPECT_NEAR(" << frozen << ", " << path.deref()
      << ", .001);\n";
  }
};

template <typename Cpp2>
struct value_test<Cpp2, type_class::enumeration> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &,
    std::ofstream &out
  ) {
    out << indentation << "EXPECT_EQ("
      << fatal::z_data<typename fatal::enum_traits<Cpp2>::name>() << "::"
      << fatal::enum_to_string(static_cast<Cpp2>(frozen)) << ", "
      << path.deref() << ");\n";
  }
};

template <typename Cpp2>
struct value_test<Cpp2, type_class::structure> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) {
    fatal::foreach<typename reflect_struct<Cpp2>::members>(
      value_test(),
      frozen, path, indentation, context, out
    );
  }

  template <typename Member, typename Cpp1>
  void m(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) const {
    value_test<typename Member::type, typename Member::type_class>::gen(
      frozen,
      path.add(true, fatal::z_data<typename Member::name>()),
      indentation, context, out
    );
  }

  template <typename Member, typename Cpp1>
  void M(
    std::true_type,
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) const {
    m<Member>(Member::getter::copy(frozen), path, indentation, context, out);
  }

  template <typename Member, typename Cpp1>
  void M(
    std::false_type,
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) const {
    m<Member>(Member::getter::ref(frozen), path, indentation, context, out);
  }

  template <typename Member, std::size_t Index, typename Cpp1>
  void operator ()(
    fatal::indexed<Member, Index>,
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) const {
    if (Member::is_set(frozen)) {
      M<Member>(
        std::integral_constant<
          bool,
          std::is_arithmetic<typename Member::type>::value
        >(),
        frozen, path, indentation, context, out
      );
    }
  }
};

template <typename Cpp2>
struct value_test<Cpp2, type_class::string> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &,
    std::ofstream &out
  ) {
    out << indentation << "ASSERT_EQ(" << frozen.size() << ", "
      << path.member("size()") << ");\n";
    out << indentation << "EXPECT_TRUE(std::equal("
      << path.member("begin()") << ", " << path.member("end()") << ", "
      << fatal::data_as_literal(frozen) << "));\n";
  }
};

template <typename Cpp2, typename Value>
struct value_test<Cpp2, type_class::list<Value>> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) {
    out << indentation << "ASSERT_EQ(" << frozen.size() << ", "
      << path.member("size()") << ");\n";
    value_path const i(true, "i_", context.id());
    out << indentation << "{ // list\n";
    out << indentation << "  auto " << *i << " = "
      << path.member("begin()") << ";\n";
    for (auto j = frozen.begin(); j != frozen.end(); ++j) {
      out << indentation << "  ASSERT_NE(" << path.member("end()") << ", "
        << *i << ");\n";
      value_test<typename Cpp2::value_type, Value>::gen(
        *j, i, indentation + "  ", context, out
      );
      out << indentation << "  ++" << *i << ";\n";
    }
    out << indentation << "  EXPECT_EQ(" << path.member("end()") << ", "
      << *i << ");\n";
    out << indentation << "}\n";
  }
};

template <typename Cpp2, typename Value>
struct value_test<Cpp2, type_class::set<Value>> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) {
    out << indentation << "ASSERT_EQ(" << frozen.size() << ", "
      << path.member("size()") << ");\n";
    value_path const i(true, "i_", context.id());
    out << indentation << "{ // set\n";
    out << indentation << "  auto " << *i << " = "
      << path.member("begin()") << ";\n";
    for (auto j = frozen.begin(); j != frozen.end(); ++j) {
      out << indentation << "  ASSERT_NE(" << path.member("end()") << ", "
        << *i << ");\n";
      value_test<typename Cpp2::value_type, Value>::gen(
        *j, i, indentation + "  ", context, out
      );
      out << indentation << "  ++" << *i << ";\n";
    }
    out << indentation << "  EXPECT_EQ(" << path.member("end()") << ", "
      << *i << ");\n";
    out << indentation << "}\n";
  }
};

template <typename Cpp2, typename Key, typename Value>
struct value_test<Cpp2, type_class::map<Key, Value>> {
  template <typename Cpp1>
  static void gen(
    Cpp1 const &frozen,
    value_path const &path,
    std::string const &indentation,
    value_test_context &context,
    std::ofstream &out
  ) {
    out << indentation << "ASSERT_EQ(" << frozen.size() << ", "
      << path.member("size()") << ");\n";
    value_path const i(true, "i_", context.id());
    out << indentation << "{ // map\n";
    out << indentation << "  auto " << *i << " = "
      << path.member("begin()") << ";\n";
    for (auto j = frozen.begin(); j != frozen.end(); ++j) {
      out << indentation << "  ASSERT_NE(" << path.member("end()") << ", "
        << *i << ");\n";
      value_test<typename Cpp2::key_type, Key>::gen(
        j->first, i.add(false, "first"), indentation + "  ", context, out
      );
      value_test<typename Cpp2::mapped_type, Value>::gen(
        j->second, i.add(false, "second"), indentation + "  ", context, out
      );
      out << indentation << "  ++" << *i << ";\n";
    }
    out << indentation << "  EXPECT_EQ(" << path.member("end()") << ", "
      << *i << ");\n";
    out << indentation << "}\n";
  }
};

template <typename Cpp2>
struct has_members_test {
  static void gen(bool has_isset, std::ofstream &out) {
    fatal::foreach<typename reflect_struct<Cpp2>::members>(
      has_members_test(),
      has_isset,
      out
    );
  }

  template <typename Member, std::size_t Index>
  void operator ()(
    fatal::indexed<Member, Index>,
    bool has_isset,
    std::ofstream &out
  ) {
    using info = reflect_struct<Cpp2>;
    out << "  static_assert(info::member::"
      << fatal::z_data<typename Member::name>() << "::getter::has<"
      << fatal::z_data<typename info::name>() << ">::value, \"\");\n";
    if (has_isset) {
      out << "  static_assert("
        << (Member::optional::value == optionality::required ? "!" : "")
        << "info::member::" << fatal::z_data<typename Member::name>()
        << "::getter::has<isset>::value, \"\");\n";
    }
  }
};

} // namespace thrift {
} // namespace apache {

namespace test_cpp1 {
namespace cpp_frozen1 {

template <typename Member>
using not_required = std::integral_constant<
  bool,
  Member::optional::value != apache::thrift::optionality::required
>;

class isset_info {
  template <
    typename T,
    typename = decltype(std::declval<apache::thrift::Frozen<T>>().__isset)
  >
  static constexpr std::size_t size_sfinae(T *) {
    return sizeof(std::declval<apache::thrift::Frozen<T>>().__isset);
  }

  template <typename...>
  static constexpr std::size_t size_sfinae(...) { return 0; }

  template <
    typename T,
    typename = decltype(std::declval<apache::thrift::Frozen<T>>().__isset)
  >
  static constexpr std::size_t alignment_sfinae(T *) {
    return alignof(decltype(std::declval<apache::thrift::Frozen<T>>().__isset));
  }

  template <typename...>
  static constexpr std::size_t alignment_sfinae(...) { return 0; }

public:
  template <typename T>
  static constexpr std::size_t size() {
    return size_sfinae(static_cast<T *>(nullptr));
  }

  template <typename T>
  static constexpr std::size_t alignment() {
    return alignment_sfinae(static_cast<T *>(nullptr));
  }
};

// test generator //
struct test_generator {
  explicit test_generator(std::string const &output_path): out_(output_path) {}

  template <typename Cpp1, typename Cpp2>
  void gen_test(char const *name, Cpp1 instance, Cpp2 const &reference) {
    using namespace apache::thrift;

    isset_setter<>::set(instance, reference, out_);

    using info = reflect_struct<Cpp2>;
    auto const type = fatal::z_data<typename info::name>();
    auto const serialized = serialize(instance);
    auto const data = fatal::data_as_literal(serialized);
    value_path const path(false, "view");
    auto const &frozen = *reinterpret_cast<Frozen<Cpp1> const *>(
      serialized.data()
    );
    bool const has_isset = !fatal::empty<
      fatal::filter<typename info::members, fatal::applier<not_required>>
    >::value;

    out_ << "TEST(frozen1, " << name << ") {\n";

    out_ << "  auto &view = apache::thrift::frozenView<" << type << ">(";
    out_ << "\n  // ";
    for (std::size_t i = 0; i < serialized.size(); ++i) {
      out_ << std::setw(4) << i;
    }
    out_ << "\n    " << data << "\n  );\n\n";

    // checks for the structure size and alignment
    out_ << "  ASSERT_EQ(" << alignof(Frozen<Cpp1>)
      << ", apache::thrift::FrozenSizeOf<decltype(view)>::alignment());\n";
    out_ << "  ASSERT_EQ(" << sizeof(Frozen<Cpp1>)
      << ", apache::thrift::FrozenSizeOf<decltype(view)>::size());\n";

    if (isset_info::size<Cpp1>()) {
      out_ << "  ASSERT_EQ(" << isset_info::alignment<Cpp1>()
        << ", apache::thrift::FrozenSizeOf<decltype(*view.__isset)>"
        "::alignment());\n";
      out_ << "  ASSERT_EQ(" << isset_info::size<Cpp1>()
        << ", apache::thrift::FrozenSizeOf<decltype(*view.__isset)>"
        "::size());\n";
    }
    out_ << '\n';

    if (has_isset) {
      isset_test<>::gen(reference, path, out_);
      out_ << '\n';
    }

    value_test_context context;
    value_test<Cpp2, reflect_type_class<Cpp2>>::gen(
      frozen, path, "  ", context, out_
    );

    out_ << "\n  using info = apache::thrift::reflect_struct<"
      << fatal::z_data<typename info::name>() << ">;\n";
    if (has_isset) {
      out_ << "  using isset = typename std::decay<decltype(*view.__isset)"
        ">::type;\n";
    }
    has_members_test<Cpp2>::gen(has_isset, out_);
    out_ << "}\n\n";
  }

# define TO_BLOB(...) \
  gen_test( \
    #__VA_ARGS__, \
    test_cpp1::cpp_frozen1::frozen1_constants::__VA_ARGS__(), \
    test_cpp2::cpp_frozen1::frozen1_constants::__VA_ARGS__() \
  )

  void gen_tests() {
    TO_BLOB(struct_bool_f_c);
    TO_BLOB(struct_bool_t_c);
    TO_BLOB(struct_byte_c);
    TO_BLOB(struct_e1_c);
    TO_BLOB(struct_i16_c);
    TO_BLOB(struct_i32_c);
    TO_BLOB(struct_i64_c);
    TO_BLOB(struct_flt_c);
    TO_BLOB(struct_dbl_c);
    TO_BLOB(struct_str_c);
    TO_BLOB(struct_lst_i32_c);
    TO_BLOB(struct_lst_str_c);
    TO_BLOB(struct_req_lst_i32_c);
    TO_BLOB(struct_req_lst_str_c);
    TO_BLOB(struct_set_i32_c);
    TO_BLOB(struct_set_str_c);
    TO_BLOB(struct_req_set_i32_c);
    TO_BLOB(struct_req_set_str_c);
    TO_BLOB(struct_map_i32_i32_c);
    TO_BLOB(struct_map_i32_str_c);
    TO_BLOB(struct_map_str_str_c);
    TO_BLOB(struct_req_map_i32_i32_c);
    TO_BLOB(struct_req_map_i32_str_c);
    TO_BLOB(struct_req_map_str_str_c);
    TO_BLOB(struct_hmap_i32_i32_c);
    TO_BLOB(struct_hmap_i32_str_c);
    TO_BLOB(struct_hmap_str_str_c);
    TO_BLOB(struct_req_hmap_i32_i32_c);
    TO_BLOB(struct_req_hmap_i32_str_c);
    TO_BLOB(struct_req_hmap_str_str_c);
    TO_BLOB(int_fields_0_c);
    TO_BLOB(int_fields_1_c);
    TO_BLOB(int_fields_2_c);
    TO_BLOB(int_fields_3_c);
    TO_BLOB(int_fields_4_c);
    TO_BLOB(int_fields_5_c);
    TO_BLOB(int_fields_6_c);
    TO_BLOB(int_fields_7_c);
    TO_BLOB(int_fields_10_c);
    TO_BLOB(int_fields_11_c);
    TO_BLOB(int_fields_12_c);
    TO_BLOB(int_fields_13_c);
    TO_BLOB(int_fields_14_c);
    TO_BLOB(int_fields_15_c);
    TO_BLOB(int_fields_16_c);
    TO_BLOB(int_fields_17_c);
    TO_BLOB(basic_fields_c);
    TO_BLOB(multi_fields_sparse_c);
    TO_BLOB(padded_booleans_0_c);
    TO_BLOB(unpadded_booleans_0_c);
    TO_BLOB(nested_c);
    TO_BLOB(variable_nested_c);
    TO_BLOB(variable_nested_lst_c);
    TO_BLOB(variable_nested_req_lst_c);
    TO_BLOB(variable_nested_map_c);
    TO_BLOB(variable_nested_req_map_c);
    TO_BLOB(variable_nested_hmap_c);
    TO_BLOB(variable_nested_req_hmap_c);
    TO_BLOB(amalgamation_c);
  }

private:
  template <typename T>
  std::size_t get_id() {
    return ids_[&typeid(T)]++;
  }

  std::unordered_map<std::type_info const *, std::size_t> ids_;
  std::ofstream out_;
};

} // namespace cpp_frozen1 {
} // namespace test_cpp2 {

int main() {
  using namespace test_cpp1::cpp_frozen1;

  std::string const install_dir(::getenv("INSTALL_DIR"));

  test_generator generator(
    folly::to<std::string>(
      install_dir,
      install_dir.empty() ? "." : "",
      "/generated_frozen1_test.h"
    )
  );

  generator.gen_tests();

  return 0;
}
