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

#pragma once

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <thrift/compiler/detail/mustache/mstch.h>
#include <thrift/compiler/generate/t_generator.h>

namespace apache {
namespace thrift {
namespace compiler {

class mstch_base;
class mstch_generators;

enum ELEMENT_POSITION {
  NONE = 0,
  FIRST = 1,
  LAST = 2,
  FIRST_AND_LAST = 3,
};

struct mstch_cache {
  std::map<std::string, std::string> parsed_options_;
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> enums_;
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> structs_;
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> services_;
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> programs_;

  void clear() {
    enums_.clear();
    structs_.clear();
    services_.clear();
    programs_.clear();
  }
};

class enum_value_generator {
 public:
  virtual ~enum_value_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum_value const* enum_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class enum_generator {
 public:
  virtual ~enum_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class const_value_generator {
 public:
  const_value_generator() = default;
  virtual ~const_value_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_const_value const* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const;
  virtual std::shared_ptr<mstch_base> generate(
      std::pair<t_const_value*, t_const_value*> const& value_pair,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      std::pair<const t_type*, const t_type*> const& expected_types = {
          nullptr, nullptr}) const;
};

class type_generator {
 public:
  type_generator() = default;
  virtual ~type_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

struct field_generator_context {
  const t_struct* strct = nullptr;
  const t_field* prev = nullptr;
  const t_field* next = nullptr;
  int isset_index = -1;
};

class field_generator {
 public:
  field_generator() = default;
  virtual ~field_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      field_generator_context const* field_context = nullptr) const;
};

class annotation_generator {
 public:
  annotation_generator() = default;
  virtual ~annotation_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      const t_annotation& annotation,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class structured_annotation_generator {
 public:
  structured_annotation_generator() = default;
  virtual ~structured_annotation_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      const t_const* annotValue,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class struct_generator {
 public:
  struct_generator() = default;
  virtual ~struct_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class function_generator {
 public:
  function_generator() = default;
  virtual ~function_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class service_generator {
 public:
  service_generator() = default;
  virtual ~service_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;

  std::shared_ptr<mstch_base> generate_cached(
      t_program const* program,
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const {
    std::string service_id = program->path() + service->get_name();
    auto itr = cache->services_.find(service_id);
    if (itr == cache->services_.end()) {
      itr = cache->services_.emplace_hint(
          itr, service_id, generate(service, generators, cache, pos, index));
    }
    return itr->second;
  }
};

class typedef_generator {
 public:
  typedef_generator() = default;
  virtual ~typedef_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_typedef const* typedf,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;
};

class const_generator {
 public:
  const_generator() = default;
  virtual ~const_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_const const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr,
      const std::string& field_name = std::string()) const;
};

class program_generator {
 public:
  program_generator() = default;
  virtual ~program_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const;

  std::shared_ptr<mstch_base> generate_cached(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) {
    const auto& id = program->path();
    auto itr = cache->programs_.find(id);
    if (itr == cache->programs_.end()) {
      itr = cache->programs_.emplace_hint(
          itr, id, generate(program, generators, cache, pos, index));
    }
    return itr->second;
  }
};

class mstch_generators {
 public:
  mstch_generators()
      : enum_value_generator_(std::make_unique<enum_value_generator>()),
        enum_generator_(std::make_unique<enum_generator>()),
        const_value_generator_(std::make_unique<const_value_generator>()),
        type_generator_(std::make_unique<type_generator>()),
        field_generator_(std::make_unique<field_generator>()),
        annotation_generator_(std::make_unique<annotation_generator>()),
        structured_annotation_generator_(
            std::make_unique<structured_annotation_generator>()),
        struct_generator_(std::make_unique<struct_generator>()),
        function_generator_(std::make_unique<function_generator>()),
        service_generator_(std::make_unique<service_generator>()),
        typedef_generator_(std::make_unique<typedef_generator>()),
        const_generator_(std::make_unique<const_generator>()),
        program_generator_(std::make_unique<program_generator>()) {}
  ~mstch_generators() = default;

  void set_enum_value_generator(std::unique_ptr<enum_value_generator> g) {
    enum_value_generator_ = std::move(g);
  }

  void set_enum_generator(std::unique_ptr<enum_generator> g) {
    enum_generator_ = std::move(g);
  }

  void set_const_value_generator(std::unique_ptr<const_value_generator> g) {
    const_value_generator_ = std::move(g);
  }

  void set_type_generator(std::unique_ptr<type_generator> g) {
    type_generator_ = std::move(g);
  }

  void set_field_generator(std::unique_ptr<field_generator> g) {
    field_generator_ = std::move(g);
  }

  void set_annotation_generator(std::unique_ptr<annotation_generator> g) {
    annotation_generator_ = std::move(g);
  }

  void set_struct_generator(std::unique_ptr<struct_generator> g) {
    struct_generator_ = std::move(g);
  }

  void set_function_generator(std::unique_ptr<function_generator> g) {
    function_generator_ = std::move(g);
  }

  void set_service_generator(std::unique_ptr<service_generator> g) {
    service_generator_ = std::move(g);
  }

  void set_typedef_generator(std::unique_ptr<typedef_generator> g) {
    typedef_generator_ = std::move(g);
  }

  void set_const_generator(std::unique_ptr<const_generator> g) {
    const_generator_ = std::move(g);
  }

  void set_program_generator(std::unique_ptr<program_generator> g) {
    program_generator_ = std::move(g);
  }

  std::unique_ptr<enum_value_generator> enum_value_generator_;
  std::unique_ptr<enum_generator> enum_generator_;
  std::unique_ptr<const_value_generator> const_value_generator_;
  std::unique_ptr<type_generator> type_generator_;
  std::unique_ptr<field_generator> field_generator_;
  std::unique_ptr<annotation_generator> annotation_generator_;
  std::unique_ptr<structured_annotation_generator>
      structured_annotation_generator_;
  std::unique_ptr<struct_generator> struct_generator_;
  std::unique_ptr<function_generator> function_generator_;
  std::unique_ptr<service_generator> service_generator_;
  std::unique_ptr<typedef_generator> typedef_generator_;
  std::unique_ptr<const_generator> const_generator_;
  std::unique_ptr<program_generator> program_generator_;
};

class mstch_base : public mstch::object {
 protected:
  // A range of t_field* to avoid copying between std::vector<t_field*>
  // and std::vector<t_field const*>.
  class field_range {
   public:
    /* implicit */ field_range(const std::vector<t_field*>& fields) noexcept
        : begin_(const_cast<const t_field* const*>(fields.data())),
          end_(const_cast<const t_field* const*>(
              fields.data() + fields.size())) {}
    /* implicit */ field_range(
        const std::vector<t_field const*>& fields) noexcept
        : begin_(fields.data()), end_(fields.data() + fields.size()) {}
    constexpr size_t size() const noexcept { return end_ - begin_; }
    constexpr const t_field* const* begin() const noexcept { return begin_; }
    constexpr const t_field* const* end() const noexcept { return end_; }

   private:
    const t_field* const* begin_;
    const t_field* const* end_;
  };

 public:
  mstch_base(
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : generators_(generators), cache_(cache), pos_(pos) {
    register_methods(
        this,
        {
            {"first?", &mstch_base::first},
            {"last?", &mstch_base::last},
        });
  }
  virtual ~mstch_base() = default;

  mstch::node first() {
    return pos_ == ELEMENT_POSITION::FIRST ||
        pos_ == ELEMENT_POSITION::FIRST_AND_LAST;
  }
  mstch::node last() {
    return pos_ == ELEMENT_POSITION::LAST ||
        pos_ == ELEMENT_POSITION::FIRST_AND_LAST;
  }

  mstch::node annotations(t_named const* annotated) {
    return generate_annotations(annotated->annotations());
  }

  mstch::node structured_annotations(t_named const* annotated) {
    return generate_elements(
        annotated->structured_annotations(),
        generators_->structured_annotation_generator_.get());
  }

  static ELEMENT_POSITION element_position(size_t index, size_t length) {
    ELEMENT_POSITION pos = ELEMENT_POSITION::NONE;
    if (index == 0) {
      pos = ELEMENT_POSITION::FIRST;
    }
    if (index == length - 1) {
      pos = ELEMENT_POSITION::LAST;
    }
    if (length == 1) {
      pos = ELEMENT_POSITION::FIRST_AND_LAST;
    }
    return pos;
  }

  template <typename Container, typename Generator, typename... Args>
  mstch::array generate_elements(
      Container const& container,
      Generator const* generator,
      Args const&... args) {
    mstch::array a;
    size_t i = 0;
    for (auto&& element : container) {
      auto pos = element_position(i, container.size());
      a.push_back(
          generator->generate(element, generators_, cache_, pos, i, args...));
      ++i;
    }
    return a;
  }

  template <typename C, typename... Args>
  mstch::array generate_services(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->service_generator_.get(), args...);
  }

  template <typename C, typename... Args>
  mstch::array generate_annotations(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->annotation_generator_.get(), args...);
  }

  template <typename C, typename... Args>
  mstch::array generate_enum_values(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->enum_value_generator_.get(), args...);
  }

  template <typename C, typename... Args>
  mstch::array generate_consts(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->const_value_generator_.get(), args...);
  }

  virtual mstch::array generate_fields(const field_range& fields) {
    return generate_elements(fields, generators_->field_generator_.get());
  }

  template <typename C, typename... Args>
  mstch::array generate_functions(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->function_generator_.get(), args...);
  }

  template <typename C, typename... Args>
  mstch::array generate_typedefs(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->typedef_generator_.get(), args...);
  }

  template <typename C, typename... Args>
  mstch::array generate_types(C const& container, Args const&... args) {
    return generate_elements(
        container, generators_->type_generator_.get(), args...);
  }

  template <typename Item, typename Generator, typename Cache>
  mstch::node generate_element_cached(
      Item const& item,
      Generator const* generator,
      Cache& c,
      std::string const& id,
      size_t element_index,
      size_t element_count) {
    std::string elem_id = id + item->get_name();
    auto pos = element_position(element_index, element_count);
    auto itr = c.find(elem_id);
    if (itr == c.end()) {
      itr = c.emplace_hint(
          itr,
          elem_id,
          generator->generate(item, generators_, cache_, pos, element_index));
    }
    return itr->second;
  }

  template <typename Container, typename Generator, typename Cache>
  mstch::array generate_elements_cached(
      Container const& container,
      Generator const* generator,
      Cache& c,
      std::string const& id) {
    mstch::array a;
    for (size_t i = 0; i < container.size(); ++i) {
      a.push_back(generate_element_cached(
          container[i], generator, c, id, i, container.size()));
    }
    return a;
  }

  bool has_option(const std::string& option) const;
  std::string get_option(const std::string& option) const;

  // Registers has_option(option) under the given name.
  void register_has_option(std::string key, std::string option);

 protected:
  std::shared_ptr<mstch_generators const> generators_;
  std::shared_ptr<mstch_cache> cache_;
  ELEMENT_POSITION const pos_;
};

class mstch_enum_value : public mstch_base {
 public:
  using node_type = t_enum_value;
  mstch_enum_value(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), enm_value_(enm_value) {
    register_methods(
        this,
        {
            {"enum_value:name", &mstch_enum_value::name},
            {"enum_value:value", &mstch_enum_value::value},
        });
  }
  mstch::node name() { return enm_value_->get_name(); }
  mstch::node value() { return std::to_string(enm_value_->get_value()); }

 protected:
  t_enum_value const* enm_value_;
};

class mstch_enum : public mstch_base {
 public:
  using node_type = t_enum;
  mstch_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), enm_(enm) {
    register_methods(
        this,
        {
            {"enum:name", &mstch_enum::name},
            {"enum:values", &mstch_enum::values},
            {"enum:structured_annotations",
             &mstch_enum::structured_annotations},
        });
  }

  mstch::node name() { return enm_->get_name(); }
  mstch::node values();
  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(enm_);
  }

 protected:
  t_enum const* enm_;
};

class mstch_const_value : public mstch_base {
 public:
  using cv = t_const_value::t_const_value_type;
  mstch_const_value(
      t_const_value const* const_value,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_base(generators, cache, pos),
        const_value_(const_value),
        current_const_(current_const),
        expected_type_(expected_type),
        type_(const_value->get_type()),
        index_(index) {
    register_methods(
        this,
        {
            {"value:bool?", &mstch_const_value::is_bool},
            {"value:double?", &mstch_const_value::is_double},
            {"value:integer?", &mstch_const_value::is_integer},
            {"value:enum?", &mstch_const_value::is_enum},
            {"value:enum_value?", &mstch_const_value::has_enum_value},
            {"value:string?", &mstch_const_value::is_string},
            {"value:string_multi_line?",
             &mstch_const_value::is_string_multi_line},
            {"value:base?", &mstch_const_value::is_base},
            {"value:map?", &mstch_const_value::is_map},
            {"value:list?", &mstch_const_value::is_list},
            {"value:container?", &mstch_const_value::is_container},
            {"value:empty_container?", &mstch_const_value::is_empty_container},
            {"value:value", &mstch_const_value::value},
            {"value:integer_value", &mstch_const_value::integer_value},
            {"value:double_value", &mstch_const_value::double_value},
            {"value:bool_value", &mstch_const_value::bool_value},
            {"value:nonzero?", &mstch_const_value::is_non_zero},
            {"value:enum_name", &mstch_const_value::enum_name},
            {"value:enum_value_name", &mstch_const_value::enum_value_name},
            {"value:string_value", &mstch_const_value::string_value},
            {"value:list_elements", &mstch_const_value::list_elems},
            {"value:map_elements", &mstch_const_value::map_elems},
            {"value:const_struct", &mstch_const_value::const_struct},
            {"value:const_struct?", &mstch_const_value::is_const_struct},
            {"value:const_struct_type", &mstch_const_value::const_struct_type},
            {"value:referenceable?", &mstch_const_value::referenceable},
            {"value:owning_const", &mstch_const_value::owning_const},
            {"value:enable_referencing",
             &mstch_const_value::enable_referencing},
        });
  }

  std::string format_double_string(const double d) {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::digits10) << d;
    return oss.str();
  }
  mstch::node is_bool() { return type_ == cv::CV_BOOL; }
  mstch::node is_double() { return type_ == cv::CV_DOUBLE; }
  mstch::node is_integer() {
    return type_ == cv::CV_INTEGER && !const_value_->is_enum();
  }
  mstch::node is_enum() {
    return type_ == cv::CV_INTEGER && const_value_->is_enum();
  }
  mstch::node has_enum_value() {
    return const_value_->get_enum_value() != nullptr;
  }
  mstch::node is_string() { return type_ == cv::CV_STRING; }
  mstch::node is_string_multi_line() {
    return type_ == cv::CV_STRING &&
        const_value_->get_string().find("\n") != std::string::npos;
  }
  mstch::node is_base() {
    return type_ == cv::CV_BOOL || type_ == cv::CV_DOUBLE ||
        type_ == cv::CV_INTEGER || type_ == cv::CV_STRING;
  }
  mstch::node is_map() { return type_ == cv::CV_MAP; }
  mstch::node is_list() { return type_ == cv::CV_LIST; }
  mstch::node is_container() {
    return type_ == cv::CV_MAP || type_ == cv::CV_LIST;
  }
  mstch::node is_empty_container() {
    return (type_ == cv::CV_MAP && const_value_->get_map().empty()) ||
        (type_ == cv::CV_LIST && const_value_->get_list().empty());
  }
  mstch::node value();
  mstch::node integer_value();
  mstch::node double_value();
  mstch::node bool_value();
  mstch::node is_non_zero();
  mstch::node enum_name();
  mstch::node enum_value_name();
  mstch::node string_value();
  mstch::node list_elems();
  mstch::node map_elems();
  mstch::node const_struct();
  mstch::node referenceable() {
    return current_const_ && const_value_->get_owner() &&
        current_const_ != const_value_->get_owner() && same_type_as_expected();
  }
  mstch::node owning_const();
  mstch::node enable_referencing() {
    return mstch::map{{"value:enable_referencing?", true}};
  }
  mstch::node is_const_struct();
  mstch::node const_struct_type();

 protected:
  t_const_value const* const_value_;
  t_const const* current_const_;
  t_type const* expected_type_;
  cv const type_;
  int32_t index_;

  virtual bool same_type_as_expected() const { return false; }
};

class mstch_const_value_key_mapped_pair : public mstch_base {
 public:
  mstch_const_value_key_mapped_pair(
      std::pair<t_const_value*, t_const_value*> const& pair_values,
      t_const const* current_const,
      std::pair<const t_type*, const t_type*> const& expected_types,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_base(generators, cache, pos),
        pair_(pair_values),
        current_const_(current_const),
        expected_types_(expected_types),
        index_(index) {
    register_methods(
        this,
        {
            {"element:key", &mstch_const_value_key_mapped_pair::element_key},
            {"element:value",
             &mstch_const_value_key_mapped_pair::element_value},
        });
  }
  mstch::node element_key();
  mstch::node element_value();

 protected:
  std::pair<t_const_value*, t_const_value*> const pair_;
  t_const const* current_const_;
  std::pair<const t_type*, const t_type*> const expected_types_;
  int32_t index_;
};

class mstch_type : public mstch_base {
 public:
  mstch_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos),
        type_(type),
        resolved_type_(type->get_true_type()) {
    register_methods(
        this,
        {
            {"type:name", &mstch_type::name},
            {"type:void?", &mstch_type::is_void},
            {"type:string?", &mstch_type::is_string},
            {"type:binary?", &mstch_type::is_binary},
            {"type:bool?", &mstch_type::is_bool},
            {"type:byte?", &mstch_type::is_byte},
            {"type:i16?", &mstch_type::is_i16},
            {"type:i32?", &mstch_type::is_i32},
            {"type:i64?", &mstch_type::is_i64},
            {"type:double?", &mstch_type::is_double},
            {"type:float?", &mstch_type::is_float},
            {"type:floating_point?", &mstch_type::is_floating_point},
            {"type:struct?", &mstch_type::is_struct},
            {"type:union?", &mstch_type::is_union},
            {"type:enum?", &mstch_type::is_enum},
            {"type:sink?", &mstch_type::is_sink},
            {"type:sink_has_first_response?",
             &mstch_type::sink_has_first_response},
            {"type:stream_or_sink?", &mstch_type::is_stream_or_sink},
            {"type:streamresponse?", &mstch_type::is_streamresponse},
            {"type:stream_has_first_response?",
             &mstch_type::stream_has_first_response},
            {"type:service?", &mstch_type::is_service},
            {"type:base?", &mstch_type::is_base},
            {"type:container?", &mstch_type::is_container},
            {"type:list?", &mstch_type::is_list},
            {"type:set?", &mstch_type::is_set},
            {"type:map?", &mstch_type::is_map},
            {"type:typedef?", &mstch_type::is_typedef},
            {"type:struct", &mstch_type::get_struct},
            {"type:enum", &mstch_type::get_enum},
            {"type:list_elem_type", &mstch_type::get_list_type},
            {"type:set_elem_type", &mstch_type::get_set_type},
            {"type:sink_elem_type", &mstch_type::get_sink_elem_type},
            {"type:sink_final_response_type",
             &mstch_type::get_sink_final_reponse_type},
            {"type:sink_first_response_type",
             &mstch_type::get_sink_first_response_type},
            {"type:stream_elem_type", &mstch_type::get_stream_elem_type},
            {"type:stream_first_response_type",
             &mstch_type::get_stream_first_response_type},
            {"type:key_type", &mstch_type::get_key_type},
            {"type:value_type", &mstch_type::get_value_type},
            {"type:typedef_type", &mstch_type::get_typedef_type},
            {"type:typedef", &mstch_type::get_typedef},
            {"type:interaction?", &mstch_type::is_interaction},
        });
  }

  mstch::node name() { return type_->get_name(); }
  mstch::node is_void() { return resolved_type_->is_void(); }
  mstch::node is_string() { return resolved_type_->is_string(); }
  mstch::node is_binary() { return resolved_type_->is_binary(); }
  mstch::node is_bool() { return resolved_type_->is_bool(); }
  mstch::node is_byte() { return resolved_type_->is_byte(); }
  mstch::node is_i16() { return resolved_type_->is_i16(); }
  mstch::node is_i32() { return resolved_type_->is_i32(); }
  mstch::node is_i64() { return resolved_type_->is_i64(); }
  mstch::node is_double() { return resolved_type_->is_double(); }
  mstch::node is_float() { return resolved_type_->is_float(); }
  mstch::node is_floating_point() {
    return resolved_type_->is_floating_point();
  }
  mstch::node is_struct() {
    return resolved_type_->is_struct() || resolved_type_->is_xception();
  }
  mstch::node is_union() { return resolved_type_->is_union(); }
  mstch::node is_enum() { return resolved_type_->is_enum(); }
  mstch::node is_sink() { return resolved_type_->is_sink(); }
  mstch::node sink_has_first_response() {
    return resolved_type_->is_sink() &&
        dynamic_cast<const t_sink*>(resolved_type_)->sink_has_first_response();
  }
  mstch::node is_stream_or_sink() {
    return resolved_type_->is_streamresponse() || resolved_type_->is_sink();
  }
  mstch::node is_streamresponse() {
    return resolved_type_->is_streamresponse();
  }
  mstch::node stream_has_first_response() {
    return resolved_type_->is_streamresponse() &&
        dynamic_cast<const t_stream_response*>(resolved_type_)
            ->first_response_type() != boost::none;
  }
  mstch::node is_service() { return resolved_type_->is_service(); }
  mstch::node is_base() { return resolved_type_->is_base_type(); }
  mstch::node is_container() { return resolved_type_->is_container(); }
  mstch::node is_list() { return resolved_type_->is_list(); }
  mstch::node is_set() { return resolved_type_->is_set(); }
  mstch::node is_map() { return resolved_type_->is_map(); }
  mstch::node is_typedef() { return type_->is_typedef(); }
  virtual std::string get_type_namespace(t_program const*) { return ""; }
  mstch::node get_struct();
  mstch::node get_enum();
  mstch::node get_list_type();
  mstch::node get_set_type();
  mstch::node get_key_type();
  mstch::node get_value_type();
  mstch::node get_typedef_type();
  mstch::node get_typedef();
  mstch::node get_sink_first_response_type();
  mstch::node get_sink_elem_type();
  mstch::node get_sink_final_reponse_type();
  mstch::node get_stream_elem_type();
  mstch::node get_stream_first_response_type();
  mstch::node is_interaction() { return type_->is_service(); }

 protected:
  t_type const* type_;
  t_type const* resolved_type_;
};

class mstch_field : public mstch_base {
 public:
  mstch_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      field_generator_context const* field_context)
      : mstch_base(generators, cache, pos),
        field_(field),
        index_(index),
        field_context_(field_context) {
    register_methods(
        this,
        {
            {"field:name", &mstch_field::name},
            {"field:key", &mstch_field::key},
            {"field:value", &mstch_field::value},
            {"field:type", &mstch_field::type},
            {"field:index", &mstch_field::index},
            {"field:required?", &mstch_field::is_required},
            {"field:optional?", &mstch_field::is_optional},
            {"field:opt_in_req_out?", &mstch_field::is_optInReqOut},
            {"field:annotations", &mstch_field::annotations},
            {"field:structured_annotations",
             &mstch_field::structured_annotations},
        });
  }
  mstch::node name() { return field_->get_name(); }
  mstch::node key() { return std::to_string(field_->get_key()); }
  mstch::node value();
  mstch::node type();
  mstch::node index() { return std::to_string(index_); }
  mstch::node is_required() {
    return field_->get_req() == t_field::e_req::required;
  }
  mstch::node is_optional() {
    return field_->get_req() == t_field::e_req::optional;
  }
  mstch::node is_optInReqOut() {
    return field_->get_req() == t_field::e_req::opt_in_req_out;
  }
  mstch::node annotations() { return mstch_base::annotations(field_); }
  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(field_);
  }

 protected:
  t_field const* field_;
  int32_t index_;
  field_generator_context const* field_context_;
};

class mstch_annotation : public mstch_base {
 public:
  mstch_annotation(
      const std::string& key,
      const annotation_value& val,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_base(generators, cache, pos),
        key_(key),
        val_(val),
        index_(index) {
    register_methods(
        this,
        {
            {"annotation:key", &mstch_annotation::key},
            {"annotation:value", &mstch_annotation::value},
        });
  }
  mstch::node key() { return key_; }
  mstch::node value() { return val_.value; }

 protected:
  const std::string key_;
  const annotation_value val_;
  int32_t index_;
};

class mstch_structured_annotation : public mstch_base {
 public:
  mstch_structured_annotation(
      const t_const& cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_base(generators, cache, pos), cnst_(cnst), index_(index) {
    register_methods(
        this,
        {{"structured_annotation:const",
          &mstch_structured_annotation::constant},
         {"structured_annotation:const_struct?",
          &mstch_structured_annotation::is_const_struct}});
  }
  mstch::node constant() {
    return generators_->const_generator_->generate(
        &cnst_,
        generators_,
        cache_,
        pos_,
        index_,
        &cnst_,
        cnst_.type()->get_true_type());
  }

  mstch::node is_const_struct() {
    return cnst_.type()->get_true_type()->is_struct();
  }

 protected:
  const t_const& cnst_;
  int32_t index_;
};

class mstch_struct : public mstch_base {
 public:
  mstch_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), strct_(strct) {
    register_methods(
        this,
        {
            {"struct:name", &mstch_struct::name},
            {"struct:fields?", &mstch_struct::has_fields},
            {"struct:fields", &mstch_struct::fields},
            {"struct:exception?", &mstch_struct::is_exception},
            {"struct:union?", &mstch_struct::is_union},
            {"struct:plain?", &mstch_struct::is_plain},
            {"struct:annotations", &mstch_struct::annotations},
            {"struct:thrift_uri", &mstch_struct::thrift_uri},
            {"struct:structured_annotations",
             &mstch_struct::structured_annotations},
            {"struct:exception_kind", &mstch_struct::exception_kind},
            {"struct:exception_safety", &mstch_struct::exception_safety},
            {"struct:exception_blame", &mstch_struct::exception_blame},
        });

    // Populate field_context_generator for each field.
    auto ctx = field_generator_context{};
    ctx.strct = strct_;
    auto fields = strct->fields();
    for (auto it = fields.begin(); it != fields.end(); it++) {
      const auto* field = &*it;
      if (cpp2::field_has_isset(field)) {
        ctx.isset_index++;
      }
      ctx.next = (it + 1) != fields.end() ? &*(it + 1) : nullptr;
      context_map[field] = ctx;
      ctx.prev = field;
    }
  }
  mstch::node name() { return strct_->get_name(); }
  mstch::node has_fields() { return strct_->has_fields(); }
  mstch::node fields();
  mstch::node is_exception() { return strct_->is_xception(); }
  mstch::node is_union() { return strct_->is_union(); }
  mstch::node is_plain() {
    return !strct_->is_xception() && !strct_->is_union();
  }
  mstch::node annotations() { return mstch_base::annotations(strct_); }
  mstch::node thrift_uri();
  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(strct_);
  }

  mstch::node exception_safety();

  mstch::node exception_blame();

  mstch::node exception_kind();

  mstch::array generate_fields(const field_range& fields) override {
    mstch::array a;
    size_t i = 0;
    for (const auto* field : fields) {
      auto pos = element_position(i, fields.size());
      a.push_back(generators_->field_generator_.get()->generate(
          field, generators_, cache_, pos, i, &context_map[field]));
      ++i;
    }
    return a;
  }

 protected:
  t_struct const* strct_;
  // Although mstch_fields can be generated from different orders than the IDL
  // order, field_generator_context should be always computed in the IDL order,
  // as the context does not change by reordering. Without the map, each
  // different reordering recomputes field_generator_context, and each
  // field takes O(N) to loop through node_list_view<t_field> or
  // std::vector<t_field*> to find the exact t_field to compute
  // field_generator_context.
  std::unordered_map<t_field const*, field_generator_context> context_map;
};

class mstch_function : public mstch_base {
 public:
  mstch_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), function_(function) {
    register_methods(
        this,
        {
            {"function:name", &mstch_function::name},
            {"function:oneway?", &mstch_function::oneway},
            {"function:return_type", &mstch_function::return_type},
            {"function:exceptions", &mstch_function::exceptions},
            {"function:stream_exceptions", &mstch_function::stream_exceptions},
            {"function:sink_exceptions", &mstch_function::sink_exceptions},
            {"function:sink_final_response_exceptions",
             &mstch_function::sink_final_response_exceptions},
            {"function:exceptions?", &mstch_function::has_exceptions},
            {"function:stream_exceptions?",
             &mstch_function::has_streamexceptions},
            {"function:sink_exceptions?", &mstch_function::has_sinkexceptions},
            {"function:sink_final_response_exceptions?",
             &mstch_function::has_sink_final_response_exceptions},
            {"function:args", &mstch_function::arg_list},
            {"function:comma", &mstch_function::has_args},
            {"function:priority", &mstch_function::priority},
            {"function:returns_sink?", &mstch_function::returns_sink},
            {"function:returns_streams?", &mstch_function::returns_stream},
            {"function:returns_stream?", &mstch_function::returns_stream},
            {"function:stream_has_first_response?",
             &mstch_function::stream_has_first_response},
            {"function:annotations", &mstch_function::annotations},
            {"function:starts_interaction?",
             &mstch_function::starts_interaction},
            {"function:structured_annotations",
             &mstch_function::structured_annotations},
            {"function:qualifier", &mstch_function::qualifier},
        });
  }

  mstch::node name() { return function_->get_name(); }
  mstch::node oneway() {
    return function_->qualifier() == t_function_qualifier::one_way;
  }
  mstch::node has_exceptions() {
    return function_->get_xceptions()->has_fields();
  }
  mstch::node has_streamexceptions() {
    return function_->get_stream_xceptions()->has_fields();
  }
  mstch::node has_sinkexceptions() {
    return function_->get_sink_xceptions()->has_fields();
  }
  mstch::node has_sink_final_response_exceptions() {
    return function_->get_sink_final_response_xceptions()->has_fields();
  }
  mstch::node stream_has_first_response() {
    const auto& rettype = *function_->return_type();
    auto stream = dynamic_cast<const t_stream_response*>(&rettype);
    return stream && stream->first_response_type() != boost::none;
  }
  mstch::node has_args() {
    if (function_->get_paramlist()->has_fields()) {
      return std::string(", ");
    }
    return std::string();
  }
  mstch::node priority() {
    return function_->get_annotation("priority", "NORMAL");
  }
  mstch::node returns_sink() { return function_->returns_sink(); }
  mstch::node annotations() { return mstch_base::annotations(function_); }

  mstch::node return_type();
  mstch::node exceptions();
  mstch::node stream_exceptions();
  mstch::node sink_exceptions();
  mstch::node sink_final_response_exceptions();
  mstch::node arg_list();
  mstch::node returns_stream();
  mstch::node starts_interaction() {
    return function_->get_returntype()->is_service();
  }

  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(function_);
  }

  mstch::node qualifier() {
    auto q = function_->qualifier();
    switch (q) {
      case t_function_qualifier::one_way:
        return std::string("OneWay");
      case t_function_qualifier::idempotent:
        return std::string("Idempotent");
      case t_function_qualifier::read_only:
        return std::string("ReadOnly");
      default:
        return std::string("Unspecified");
    }
  }

 protected:
  t_function const* function_;
};

class mstch_service : public mstch_base {
 public:
  mstch_service(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), service_(service) {
    register_methods(
        this,
        {
            {"service:name", &mstch_service::name},
            {"service:functions", &mstch_service::functions},
            {"service:functions?", &mstch_service::has_functions},
            {"service:extends", &mstch_service::extends},
            {"service:extends?", &mstch_service::has_extends},
            {"service:streams?", &mstch_service::has_streams},
            {"service:sinks?", &mstch_service::has_sinks},
            {"service:annotations", &mstch_service::annotations},
            {"service:parent", &mstch_service::parent},
            {"service:interaction?", &mstch_service::is_interaction},
            {"service:interactions", &mstch_service::interactions},
            {"service:interactions?", &mstch_service::has_interactions},
            {"service:structured_annotations",
             &mstch_service::structured_annotations},
            {"interaction:serial?", &mstch_service::is_serial_interaction},
        });
  }

  virtual std::string get_service_namespace(t_program const*) { return {}; }

  mstch::node name() { return service_->get_name(); }
  mstch::node has_functions() { return !get_functions().empty(); }
  mstch::node has_extends() { return service_->get_extends() != nullptr; }
  mstch::node functions();
  mstch::node extends();
  mstch::node annotations() { return mstch_base::annotations(service_); }

  mstch::node parent() {
    return cache_->parsed_options_["parent_service_name"];
  }

  mstch::node has_streams() {
    auto& funcs = get_functions();
    return std::any_of(funcs.cbegin(), funcs.cend(), [](auto const& func) {
      return func->returns_stream();
    });
  }

  mstch::node has_sinks() {
    auto& funcs = get_functions();
    return std::any_of(funcs.cbegin(), funcs.cend(), [](auto const& func) {
      return func->returns_sink();
    });
  }

  mstch::node has_interactions() {
    auto& funcs = get_functions();
    return std::any_of(funcs.cbegin(), funcs.cend(), [](auto const& func) {
      return func->get_returntype()->is_service();
    });
  }
  mstch::node interactions() {
    if (!service_->is_interaction()) {
      cache_->parsed_options_["parent_service_name"] = service_->get_name();
    }
    std::vector<t_service const*> interactions;
    for (auto const* function : get_functions()) {
      if (function->get_returntype()->is_service()) {
        interactions.push_back(
            dynamic_cast<t_service const*>(function->get_returntype()));
      }
    }
    return generate_services(interactions);
  }
  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(service_);
  }
  mstch::node is_interaction() { return service_->is_interaction(); }
  mstch::node is_serial_interaction() {
    return service_->is_serial_interaction();
  }

  virtual ~mstch_service() = default;

 protected:
  t_service const* service_;

  mstch::node generate_cached_extended_service(const t_service* service);
  virtual const std::vector<t_function*>& get_functions() const {
    return service_->get_functions();
  }
};

class mstch_typedef : public mstch_base {
 public:
  mstch_typedef(
      t_typedef const* typedf,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), typedf_(typedf) {
    register_methods(
        this,
        {
            {"typedef:type", &mstch_typedef::type},
            {"typedef:is_same_type", &mstch_typedef::is_same_type},
            {"typedef:name", &mstch_typedef::name},
            {"typedef:structured_annotations",
             &mstch_typedef::structured_annotations},
        });
  }
  mstch::node type();
  mstch::node name() { return typedf_->name(); }
  mstch::node is_same_type() {
    return typedf_->get_name() == typedf_->get_type()->get_name();
  }
  mstch::node structured_annotations() {
    return mstch_base::structured_annotations(typedf_);
  }

 protected:
  t_typedef const* typedf_;
};

class mstch_const : public mstch_base {
 public:
  mstch_const(
      t_const const* cnst,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const std::string& field_name = std::string())
      : mstch_base(generators, cache, pos),
        cnst_(cnst),
        current_const_(current_const),
        expected_type_(expected_type),
        index_(index),
        field_name_(field_name) {
    register_methods(
        this,
        {
            {"constant:name", &mstch_const::name},
            {"constant:index", &mstch_const::index},
            {"constant:type", &mstch_const::type},
            {"constant:value", &mstch_const::value},
            {"constant:program", &mstch_const::program},
        });
  }
  mstch::node name() { return cnst_->get_name(); }
  mstch::node index() { return index_; }
  mstch::node type();
  mstch::node value();
  mstch::node program();

 protected:
  t_const const* cnst_;
  t_const const* current_const_;
  t_type const* expected_type_;
  int32_t index_;
  const std::string field_name_;
};

class mstch_program : public mstch_base {
 public:
  mstch_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), program_(program) {
    register_methods(
        this,
        {
            {"program:name", &mstch_program::name},
            {"program:path", &mstch_program::path},
            {"program:includePrefix", &mstch_program::include_prefix},
            {"program:structs", &mstch_program::structs},
            {"program:enums", &mstch_program::enums},
            {"program:services", &mstch_program::services},
            {"program:typedefs", &mstch_program::typedefs},
            {"program:constants", &mstch_program::constants},
            {"program:enums?", &mstch_program::has_enums},
            {"program:structs?", &mstch_program::has_structs},
            {"program:unions?", &mstch_program::has_unions},
            {"program:services?", &mstch_program::has_services},
            {"program:typedefs?", &mstch_program::has_typedefs},
            {"program:constants?", &mstch_program::has_constants},
            {"program:thrift_uris?", &mstch_program::has_thrift_uris},
        });
    register_has_option("program:frozen?", "frozen");
    register_has_option("program:json?", "json");
    register_has_option("program:nimble?", "nimble");
    register_has_option("program:any?", "any");
    register_has_option(
        "program:unstructured_annotations_in_metadata?",
        "deprecated_unstructured_annotations_in_metadata");
  }

  virtual std::string get_program_namespace(t_program const*) { return {}; }

  mstch::node name() { return program_->name(); }
  mstch::node path() { return program_->path(); }
  mstch::node include_prefix() { return program_->include_prefix(); }
  mstch::node has_enums() { return !program_->enums().empty(); }
  mstch::node has_structs() {
    return !program_->structs().empty() || !program_->xceptions().empty();
  }
  mstch::node has_services() { return !program_->services().empty(); }
  mstch::node has_typedefs() { return !program_->typedefs().empty(); }
  mstch::node has_constants() { return !program_->consts().empty(); }
  mstch::node has_unions() {
    auto& structs = program_->structs();
    return std::any_of(
        structs.cbegin(), structs.cend(), std::mem_fn(&t_struct::is_union));
  }

  mstch::node has_thrift_uris();
  mstch::node structs();
  mstch::node enums();
  mstch::node services();
  mstch::node typedefs();
  mstch::node constants();

 protected:
  t_program const* program_;

  virtual const std::vector<t_struct*>& get_program_objects();
  virtual const std::vector<t_enum*>& get_program_enums();
};

} // namespace compiler
} // namespace thrift
} // namespace apache
