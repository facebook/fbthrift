/*
 * Copyright 2016-present Facebook, Inc.
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

#include <thrift/compiler/mutator.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

class mutator_list {
 public:
  mutator_list() = default;

  std::vector<visitor*> get_pointers() const {
    auto pointers = std::vector<visitor*>{};
    for (auto const& v : mutators_) {
      pointers.push_back(v.get());
    }
    return pointers;
  }

  template <typename T, typename... Args>
  void add(Args&&... args) {
    auto ptr = make_mutator<T>(std::forward<Args>(args)...);
    mutators_.push_back(std::move(ptr));
  }

 private:
  std::vector<std::unique_ptr<mutator>> mutators_;
};
}

static void fill_mutators(mutator_list& ms);

void mutator::mutate(t_program* const program) {
  auto mutators = mutator_list();
  fill_mutators(mutators);
  interleaved_visitor(mutators.get_pointers()).traverse(program);
}

/**
 * fill_mutators - the validator registry
 *
 * This is where all concrete validator types must be registered.
 */

static void fill_mutators(mutator_list& ms) {

  ms.add<field_type_to_const_value>();
  ms.add<const_type_to_const_value>();

  // add more mutators here ...

}

bool mutator::visit(t_program* const /* program */) {
  return true;
}

t_type* mutator::resolve_type(t_type* type) {
  if (!type->is_typedef()) {
    return type;
  }
  return resolve_type(dynamic_cast<t_typedef*>(type)->get_type());
}

void mutator::traverse_field(
    t_type* long_type,
    t_const_value* value) {
  t_type* type = resolve_type(long_type);
  value->set_ttype(type);
  if (type->is_list()) {
    auto* elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
    for (auto list_val : value->get_list()) {
      traverse_field(elem_type, list_val);
    }
  }
  if (type->is_set()) {
    auto* elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
    for (auto set_val : value->get_list()) {
      traverse_field(elem_type, set_val);
    }
  }
  if (type->is_map()) {
    auto* key_type = dynamic_cast<const t_map*>(type)->get_key_type();
    auto* val_type = dynamic_cast<const t_map*>(type)->get_val_type();
    for (auto map_val : value->get_map()) {
      traverse_field(key_type, map_val.first);
      traverse_field(val_type, map_val.second);
    }
  }
  if (type->is_struct()) {
    auto* struct_type = dynamic_cast<const t_struct*>(type);
    for (auto map_val : value->get_map()) {
      auto tfield = struct_type->get_member(map_val.first->get_string());
      traverse_field(tfield->get_type(), map_val.second);
    }
  }
  // Set constant value types as enums when they are declared with integers
  if (type->is_enum() && !value->is_enum()) {
    value->set_is_enum();
    auto enm = dynamic_cast<t_enum const*>(type);
    value->set_enum(enm);
    if (enm->find_value(value->get_integer())) {
      value->set_enum_value(enm->find_value(value->get_integer()));
    }
  }
}

/**
 * field_type_to_const_value
 */
bool field_type_to_const_value::visit(t_field* const tfield) {
  match_type_with_const_value(tfield);
  return true;
}

void field_type_to_const_value::match_type_with_const_value(t_field* tfield) {
  if (!tfield->get_type() || !tfield->get_value()) {
    return;
  }
  traverse_field(tfield->get_type(), tfield->get_value());
}

/**
 * const_type_to_const_value
 */
bool const_type_to_const_value::visit(t_const* const tconst) {
  match_type_with_const_value(tconst);
  return true;
}

void const_type_to_const_value::match_type_with_const_value(t_const* tconst) {
  if (!tconst->get_type() || !tconst->get_value()) {
    return;
  }

  traverse_field(tconst->get_type(), tconst->get_value());
}

}
}
}
