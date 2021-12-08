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

#include <thrift/compiler/gen/cpp/type_resolver.h>

#include <stdexcept>

#include <thrift/compiler/ast/t_list.h>
#include <thrift/compiler/ast/t_map.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_set.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_typedef.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {
namespace {

const std::string* find_structured_adapter_annotation(const t_named* node) {
  const t_const* annotation = node->find_structured_annotation_or_null(
      "facebook.com/thrift/annotation/cpp/Adapter");
  if (!annotation) {
    annotation = node->find_structured_annotation_or_null(
        "facebook.com/thrift/annotation/cpp/ExperimentalAdapter");
  }
  if (annotation) {
    for (const auto& item : annotation->value()->get_map()) {
      if (item.first->get_string() == "name") {
        return &item.second->get_string();
      }
    }
  }
  return nullptr;
}

} // namespace

const std::string& type_resolver::get_storage_type_name(const t_field* node) {
  if (const std::string* adapter = find_structured_adapter_annotation(node)) {
    const t_type* type = &*node->type();
    return detail::get_or_gen(adapter_storage_type_cache_, node, [&]() {
      return gen_type(type, adapter, node->id());
    });
  }

  auto ref_type = find_ref_type(*node);
  if (ref_type == reference_type::none) {
    // The storage type is just the type name.
    return get_type_name(node->get_type());
  }

  auto key = std::make_pair(node->get_type(), ref_type);
  return detail::get_or_gen(
      storage_type_cache_, key, [&]() { return gen_storage_type(key); });
}

const std::string* type_resolver::find_first_adapter(const t_field* field) {
  if (const std::string* adapter = find_structured_adapter_annotation(field)) {
    return adapter;
  }
  if (const std::string* adapter =
          gen::cpp::type_resolver::find_first_adapter(&*field->type())) {
    return adapter;
  }
  return nullptr;
}

bool type_resolver::can_resolve_to_scalar(const t_type* node) {
  return node->get_true_type()->is_scalar() || find_first_adapter(node) ||
      find_first_type(node);
}

const std::string& type_resolver::default_template(t_container::type ctype) {
  switch (ctype) {
    case t_container::type::t_list: {
      static const auto& kValue = *new std::string("::std::vector");
      return kValue;
    }
    case t_container::type::t_set: {
      static const auto& kValue = *new std::string("::std::set");
      return kValue;
    }
    case t_container::type::t_map: {
      static const auto& kValue = *new std::string("::std::map");
      return kValue;
    }
  }
  throw std::runtime_error(
      "unknown container type: " + std::to_string(static_cast<int>(ctype)));
}

const std::string& type_resolver::default_type(t_base_type::type btype) {
  switch (btype) {
    case t_base_type::type::t_void: {
      static const auto& kValue = *new std::string("void");
      return kValue;
    }
    case t_base_type::type::t_bool: {
      static const auto& kValue = *new std::string("bool");
      return kValue;
    }
    case t_base_type::type::t_byte: {
      static const auto& kValue = *new std::string("::std::int8_t");
      return kValue;
    }
    case t_base_type::type::t_i16: {
      static const auto& kValue = *new std::string("::std::int16_t");
      return kValue;
    }
    case t_base_type::type::t_i32: {
      static const auto& kValue = *new std::string("::std::int32_t");
      return kValue;
    }
    case t_base_type::type::t_i64: {
      static const auto& kValue = *new std::string("::std::int64_t");
      return kValue;
    }
    case t_base_type::type::t_float: {
      static const auto& kValue = *new std::string("float");
      return kValue;
    }
    case t_base_type::type::t_double: {
      static const auto& kValue = *new std::string("double");
      return kValue;
    }
    case t_base_type::type::t_string:
    case t_base_type::type::t_binary: {
      static const auto& kValue = *new std::string("::std::string");
      return kValue;
    }
  }
  throw std::runtime_error(
      "unknown base type: " + std::to_string(static_cast<int>(btype)));
}

std::string type_resolver::gen_type(
    const t_type* node,
    const std::string* adapter,
    boost::optional<int16_t> field_id) {
  std::string name;
  // Find the base name.
  if (const auto* type = find_type(node)) {
    // Use the override.
    name = *type;
  } else {
    // Use the unmodified name.
    name = gen_raw_type_name(node, &type_resolver::get_type_name);
  }

  // Adapt the base name.
  return adapter ? gen_adapted_type(*adapter, std::move(name), field_id) : name;
}

std::string type_resolver::gen_standard_type(const t_type* node) {
  if (const auto* type = find_type(node)) {
    // Return the override.
    return *type;
  }

  if (const auto* ttypedef = dynamic_cast<const t_typedef*>(node)) {
    // Traverse the typedef.
    // TODO(afuller): Always traverse the adapter. There are some cpp.type and
    // cpp.template annotations that rely on the namespacing of the typedef to
    // avoid namespacing issues with the annotation itself. To avoid breaking
    // these cases we are only traversing the typedef when the presences of an
    // adapter requires we do so. However, we should update all annotations to
    // using fully qualified names, then always traverse here.
    if (find_first_adapter(node) != nullptr) {
      return get_standard_type_name(ttypedef->get_type());
    }
  }

  return gen_raw_type_name(node, &type_resolver::get_standard_type_name);
}

std::string type_resolver::gen_storage_type(
    const std::pair<const t_type*, reference_type>& ref_type) {
  const std::string& type_name = get_type_name(ref_type.first);
  switch (ref_type.second) {
    case reference_type::unique:
      return detail::gen_template_type("::std::unique_ptr", {type_name});
    case reference_type::shared_mutable:
      return detail::gen_template_type("::std::shared_ptr", {type_name});
    case reference_type::shared_const:
      return detail::gen_template_type(
          "::std::shared_ptr", {"const " + type_name});
    case reference_type::boxed:
      return detail::gen_template_type(
          "::apache::thrift::detail::boxed_value_ptr", {type_name});
    default:
      throw std::runtime_error("unknown cpp ref_type");
  }
}

std::string type_resolver::gen_raw_type_name(
    const t_type* node, type_resolve_fn resolve_fn) {
  // Base types have fixed type mappings.
  if (const auto* tbase_type = dynamic_cast<const t_base_type*>(node)) {
    return default_type(tbase_type->base_type());
  }

  // Containers have fixed template mappings.
  if (const auto* tcontainer = dynamic_cast<const t_container*>(node)) {
    return gen_container_type(tcontainer, resolve_fn);
  }

  // Streaming types have special handling.
  if (const auto* tstream_res = dynamic_cast<const t_stream_response*>(node)) {
    return gen_stream_resp_type(tstream_res, resolve_fn);
  }
  if (const auto* tsink = dynamic_cast<const t_sink*>((node))) {
    return gen_sink_type(tsink, resolve_fn);
  }

  // For everything else, just use namespaced name.
  return namespaces_.gen_namespaced_name(node);
}

std::string type_resolver::gen_container_type(
    const t_container* node, type_resolve_fn resolve_fn) {
  const auto* val = find_template(node);
  const auto& template_name =
      val ? *val : default_template(node->container_type());

  switch (node->container_type()) {
    case t_container::type::t_list:
      return detail::gen_template_type(
          template_name,
          {resolve(
              resolve_fn, static_cast<const t_list*>(node)->get_elem_type())});
    case t_container::type::t_set:
      return detail::gen_template_type(
          template_name,
          {resolve(
              resolve_fn, static_cast<const t_set*>(node)->get_elem_type())});
    case t_container::type::t_map: {
      const auto* tmap = static_cast<const t_map*>(node);
      return detail::gen_template_type(
          template_name,
          {resolve(resolve_fn, tmap->get_key_type()),
           resolve(resolve_fn, tmap->get_val_type())});
    }
  }
  throw std::runtime_error(
      "unknown container type: " +
      std::to_string(static_cast<int>(node->container_type())));
}

std::string type_resolver::gen_stream_resp_type(
    const t_stream_response* node, type_resolve_fn resolve_fn) {
  if (node->first_response_type() != boost::none) {
    return detail::gen_template_type(
        "::apache::thrift::ResponseAndServerStream",
        {resolve(resolve_fn, node->get_first_response_type()),
         resolve(resolve_fn, node->get_elem_type())});
  }
  return detail::gen_template_type(
      "::apache::thrift::ServerStream",
      {resolve(resolve_fn, node->get_elem_type())});
}

std::string type_resolver::gen_sink_type(
    const t_sink* node, type_resolve_fn resolve_fn) {
  if (node->first_response_type() != boost::none) {
    return detail::gen_template_type(
        "::apache::thrift::ResponseAndSinkConsumer",
        {resolve(resolve_fn, node->get_first_response_type()),
         resolve(resolve_fn, node->get_sink_type()),
         resolve(resolve_fn, node->get_final_response_type())});
  }
  return detail::gen_template_type(
      "::apache::thrift::SinkConsumer",
      {resolve(resolve_fn, node->get_sink_type()),
       resolve(resolve_fn, node->get_final_response_type())});
}

std::string type_resolver::gen_adapted_type(
    const std::string& adapter,
    const std::string& standard_type,
    boost::optional<int16_t> field_id) {
  if (field_id == boost::none) {
    return detail::gen_template_type(
        "::apache::thrift::adapt_detail::adapted_t", {adapter, standard_type});
  }
  return detail::gen_template_type(
      "::apache::thrift::adapt_detail::adapted_field_t",
      {adapter,
       std::to_string(*field_id),
       standard_type,
       "__fbthrift_cpp2_type"});
}

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
