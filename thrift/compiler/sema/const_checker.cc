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

#include <thrift/compiler/sema/const_checker.h>

#include <utility>
#include <vector>

#include <thrift/compiler/ast/t_union.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {
class const_checker {
 public:
  const_checker(diagnostic_context& ctx, const t_named& node, std::string name)
      : ctx_{ctx}, node_{node}, name_{std::move(name)} {}

  void check(const t_type* type, const t_const_value* value) {
    type = type->get_true_type();

    if (const auto concrete = dynamic_cast<const t_base_type*>(type)) {
      check_base_type(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_enum*>(type)) {
      check_enum(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_union*>(type)) {
      check_union(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_exception*>(type)) {
      check_exception(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_struct*>(type)) {
      check_struct(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_map*>(type)) {
      check_map(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_list*>(type)) {
      check_list(concrete, value);
    } else if (const auto concrete = dynamic_cast<const t_set*>(type)) {
      check_set(concrete, value);
    } else {
      assert(false); // should be unreachable.
    }
  }

 private:
  diagnostic_context& ctx_;
  const t_named& node_;
  std::string name_;

  template <typename... Args>
  void failure(Args&&... args) {
    ctx_.failure(node_, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning(Args&&... args) {
    ctx_.warning(node_, std::forward<Args>(args)...);
  }

  void report_type_mismatch(const char* expected) {
    failure([&](auto& o) {
      o << "type error: const `" << name_ << "` was declared as " << expected
        << ".";
    });
  }

  void report_type_mismatch_warning(const char* expected) {
    warning([&](auto& o) {
      o << "type error: const `" << name_ << "` was declared as " << expected
        << ". This will become an error in future versions of thrift.";
    });
  }

  void check_base_type(const t_base_type* type, const t_const_value* value) {
    switch (type->base_type()) {
      case t_base_type::type::t_void:
        failure([&](auto& o) {
          o << "type error: cannot declare a void const: " << name_;
        });
        break;
      case t_base_type::type::t_string:
      case t_base_type::type::t_binary:
        if (value->get_type() != t_const_value::CV_STRING) {
          report_type_mismatch("string");
        }
        break;
      case t_base_type::type::t_bool:
        if (value->get_type() != t_const_value::CV_BOOL &&
            value->get_type() != t_const_value::CV_INTEGER) {
          report_type_mismatch("bool");
        }
        break;
      case t_base_type::type::t_byte:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          report_type_mismatch("byte");
        }
        break;
      case t_base_type::type::t_i16:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          report_type_mismatch("i16");
        }
        break;
      case t_base_type::type::t_i32:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          report_type_mismatch("i32");
        }
        break;
      case t_base_type::type::t_i64:
        if (value->get_type() != t_const_value::CV_INTEGER) {
          report_type_mismatch("i64");
        }
        break;
      case t_base_type::type::t_double:
      case t_base_type::type::t_float:
        if (value->get_type() != t_const_value::CV_INTEGER &&
            value->get_type() != t_const_value::CV_DOUBLE) {
          report_type_mismatch("double");
        }
        break;
      default:
        failure([&](auto& o) {
          o << "compiler error: no const of base type "
            << t_base_type::t_base_name(type->base_type()) << name_;
        });
    }
  }

  void check_enum(const t_enum* type, const t_const_value* value) {
    if (value->get_type() != t_const_value::CV_INTEGER) {
      report_type_mismatch("enum");
    }

    if (type->find_value(value->get_integer()) == nullptr) {
      warning([&](auto& o) {
        o << "type error: const `" << name_ << "` was declared as enum `"
          << type->name()
          << "` with a value"
             " not of that enum.";
      });
    }
  }

  void check_union(const t_union* type, const t_const_value* value) {
    if (value->get_type() != t_const_value::CV_MAP) {
      report_type_mismatch("union");
    }
    auto const& map = value->get_map();
    if (map.size() > 1) {
      failure([&](auto& o) {
        o << "type error: const `" << name_
          << "` is a union and can't have more than one field set.";
      });
    }
    check_fields(type, map);
  }

  void check_struct(const t_struct* type, const t_const_value* value) {
    if (value->get_type() != t_const_value::CV_MAP) {
      report_type_mismatch_warning("struct");
    }
    check_fields(type, value->get_map());
  }

  void check_exception(const t_exception* type, const t_const_value* value) {
    if (value->get_type() != t_const_value::CV_MAP) {
      report_type_mismatch_warning("exception");
    }
    check_fields(type, value->get_map());
  }

  void check_fields(
      const t_structured* type,
      const std::vector<std::pair<t_const_value*, t_const_value*>>& map) {
    for (const auto& entry : map) {
      if (entry.first->get_type() != t_const_value::CV_STRING) {
        failure([&](auto& o) {
          o << "type error: `" << name_ << "` field name must be string.";
        });
      }
      const auto* field = type->get_field_by_name(entry.first->get_string());
      if (field == nullptr) {
        failure([&](auto& o) {
          o << "type error: `" << type->name() << "` has no field `"
            << entry.first->get_string() << "`.";
        });
        continue;
      }
      const t_type* field_type = &field->type().deref();

      const_checker(ctx_, node_, name_ + "." + entry.first->get_string())
          .check(field_type, entry.second);
    }
  }

  void check_map(const t_map* type, const t_const_value* value) {
    if (value->get_type() == t_const_value::CV_LIST &&
        value->get_list().empty()) {
      warning([&](auto& o) {
        o << "type error: map `" << name_ << "` initialized with empty list.";
      });
      return;
    }
    if (value->get_type() != t_const_value::CV_MAP) {
      report_type_mismatch_warning("map");
      return;
    }
    const t_type* k_type = &type->key_type().deref();
    const t_type* v_type = &type->val_type().deref();
    for (const auto& entry : value->get_map()) {
      const_checker(ctx_, node_, name_ + "<key>").check(k_type, entry.first);
      const_checker(ctx_, node_, name_ + "<val>").check(v_type, entry.second);
    }
  }

  void check_list(const t_list* type, const t_const_value* value) {
    if (value->get_type() == t_const_value::CV_MAP &&
        value->get_map().empty()) {
      warning([&](auto& o) {
        o << "type error: list `" << name_ << "` initialized with empty map.";
      });
      return;
    }
    if (value->get_type() != t_const_value::CV_LIST) {
      report_type_mismatch_warning("list");
      return;
    }
    check_elements(&type->elem_type().deref(), value->get_list());
  }

  void check_set(const t_set* type, const t_const_value* value) {
    if (value->get_type() == t_const_value::CV_MAP &&
        value->get_map().empty()) {
      warning([&](auto& o) {
        o << "type error: set `" << name_ << "` initialized with empty map.";
      });
      return;
    }
    if (value->get_type() != t_const_value::CV_LIST) {
      report_type_mismatch_warning("set");
      return;
    }
    check_elements(&type->elem_type().deref(), value->get_list());
  }

  void check_elements(
      const t_type* elem_type, const std::vector<t_const_value*>& list) {
    for (const auto& elem : list) {
      const_checker(ctx_, node_, name_ + "<elem>").check(elem_type, elem);
    }
  }
};
} // namespace

void check_const_rec(
    diagnostic_context& ctx,
    const t_named& node,
    const t_type* type,
    const t_const_value* value) {
  const_checker(ctx, node, node.name()).check(type, value);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
