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

const std::string& type_resolver::get_storage_type_name(const t_field* node) {
  auto ref_type = find_ref_type(*node);
  if (ref_type == reference_type::none) {
    // The storage type is just the type name.
    return get_type_name(node->get_type());
  }

  auto key = std::make_pair(node->get_type(), ref_type);
  return detail::get_or_gen(
      storage_type_cache_, key, [&]() { return gen_storage_type(key); });
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

std::string type_resolver::gen_type(const t_type* node) {
  std::string type = gen_type_impl(node, &type_resolver::get_type_name);
  if (const auto* adapter = find_adapter(node)) {
    return gen_adapted_type(*adapter, std::move(type));
  }
  return type;
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

std::string type_resolver::gen_type_impl(
    const t_type* node, type_resolve_fn resolve_fn) {
  if (const auto* type = find_type(node)) {
    // Return the override.
    return *type;
  }

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
    const std::string& adapter, const std::string& standard_type) {
  return detail::gen_template_type(
      "::apache::thrift::adapt_detail::adapted_t", {adapter, standard_type});
}

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
