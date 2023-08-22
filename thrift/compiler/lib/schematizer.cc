/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <string>
#include <utility>
#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/lib/schematizer.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {
template <typename... Args>
std::unique_ptr<t_const_value> val(Args&&... args) {
  return std::make_unique<t_const_value>(std::forward<Args>(args)...);
}
template <typename Enm, typename = std::enable_if_t<std::is_enum<Enm>::value>>
std::unique_ptr<t_const_value> val(Enm val) {
  return std::make_unique<t_const_value>(
      static_cast<std::underlying_type_t<Enm>>(val));
}
std::unique_ptr<t_const_value> mapval() {
  auto ret = val();
  ret->set_map();
  return ret;
}
std::unique_ptr<t_const_value> typeUri(
    const t_type& type, const t_program* program) {
  auto ret = mapval();
  if (!type.uri().empty()) {
    ret->add_map(val("uri"), val(type.uri()));
  } else {
    ret->add_map(val("scopedName"), val(type.get_scoped_name()));
  }
  static const std::string kTypeUriUri = "facebook.com/thrift/type/TypeUri";
  auto typeUri_ttype = t_type_ref::from_ptr(
      dynamic_cast<const t_type*>(program->scope()->find_def(kTypeUriUri)));
  ret->set_ttype(typeUri_ttype);
  return ret;
}

void add_definition(
    t_const_value& schema,
    const t_named& node,
    const t_program* program,
    schematizer::InternFunc& intern_value) {
  auto definition = val();
  definition->set_map();
  definition->add_map(val("name"), val(node.name()));
  if (!node.uri().empty()) {
    definition->add_map(val("uri"), val(node.uri()));
  }

  auto structured = node.structured_annotations();
  if (!structured.empty()) {
    auto annots = val();
    annots->set_list();

    for (const auto& item : structured) {
      auto annot = val();
      static const std::string kStructuredAnnotationSchemaUri =
          "facebook.com/thrift/type/StructuredAnnotation";
      // May be null when run from thrift2ast, which doesn't read this value.
      auto structured_annotation_ttype =
          t_type_ref::from_ptr(dynamic_cast<const t_type*>(
              program->scope()->find_def(kStructuredAnnotationSchemaUri)));
      annot->set_ttype(structured_annotation_ttype);
      annot->set_map();
      annot->add_map(val("type"), typeUri(*item.type(), program));
      if (!item.value()->is_empty()) {
        static const std::string kProtocolValueUri =
            "facebook.com/thrift/protocol/Value";
        // May be null when run from thrift2ast, which doesn't read this value.
        auto protocol_value_ttype =
            t_type_ref::from_ptr(dynamic_cast<const t_type*>(
                program->scope()->find_def(kProtocolValueUri)));
        auto fields = val();
        fields->set_map();
        for (const auto& pair : item.value()->get_map()) {
          fields->add_map(
              pair.first->clone(),
              wrap_with_protocol_value(*pair.second, protocol_value_ttype));
        }
        annot->add_map(val("fields"), std::move(fields));
      };
      auto id = intern_value(std::move(annot), const_cast<t_program*>(program));
      annots->add_list(val(id));
    }

    definition->add_map(val("annotations"), std::move(annots));
  }

  const auto& unstructured = node.annotations();
  if (!unstructured.empty()) {
    auto annots = val();
    annots->set_map();

    for (const auto& pair : unstructured) {
      annots->add_map(val(pair.first), val(pair.second.value));
    }

    definition->add_map(val("unstructuredAnnotations"), std::move(annots));
  }

  if (node.has_doc()) {
    auto docs = mapval();
    docs->add_map(val("contents"), val(node.doc()));
    definition->add_map(val("docs"), std::move(docs));
  }

  schema.add_map(val("attrs"), std::move(definition));
}

void add_as_definition(
    t_const_value& defns_schema,
    const std::string& defn_field,
    std::unique_ptr<t_const_value> schema) {
  auto defn_schema = val();
  defn_schema->set_map();
  defn_schema->add_map(val(defn_field), std::move(schema));

  defns_schema.add_list(std::move(defn_schema));
}

std::unique_ptr<t_const_value> gen_type(
    schematizer* generator,
    const t_program* program,
    t_const_value* defns_schema,
    const t_type& type) {
  auto schema = val();
  schema->set_map();
  auto type_name = val();
  type_name->set_map();
  std::unique_ptr<t_const_value> params;

  auto* resolved_type = &type;
  while (auto* typedf = dynamic_cast<const t_typedef*>(resolved_type)) {
    if (typedf->typedef_kind() != t_typedef::kind::defined) {
      resolved_type = &*typedf->type();
      continue;
    }

    type_name->add_map(val("typedefType"), typeUri(*resolved_type, program));
    schema->add_map(val("name"), std::move(type_name));
    return schema;
  }

  switch (resolved_type->get_type_value()) {
    case t_type::type::t_void:
      break;
    case t_type::type::t_bool:
      type_name->add_map(val("boolType"), val(0));
      break;
    case t_type::type::t_byte:
      type_name->add_map(val("byteType"), val(0));
      break;
    case t_type::type::t_i16:
      type_name->add_map(val("i16Type"), val(0));
      break;
    case t_type::type::t_i32:
      type_name->add_map(val("i32Type"), val(0));
      break;
    case t_type::type::t_i64:
      type_name->add_map(val("i64Type"), val(0));
      break;
    case t_type::type::t_double:
      type_name->add_map(val("doubleType"), val(0));
      break;
    case t_type::type::t_float:
      type_name->add_map(val("floatType"), val(0));
      break;
    case t_type::type::t_string:
      type_name->add_map(val("stringType"), val(0));
      break;
    case t_type::type::t_binary:
      type_name->add_map(val("binaryType"), val(0));
      break;
    case t_type::type::t_list:
      type_name->add_map(val("listType"), val(0));
      params = val();
      params->set_list();
      params->add_list(gen_type(
          generator,
          program,
          defns_schema,
          *static_cast<const t_list&>(*resolved_type).elem_type()));
      break;
    case t_type::type::t_set:
      type_name->add_map(val("setType"), val(0));
      params = val();
      params->set_list();
      params->add_list(gen_type(
          generator,
          program,
          defns_schema,
          *static_cast<const t_set&>(*resolved_type).elem_type()));
      break;
    case t_type::type::t_map:
      type_name->add_map(val("mapType"), val(0));
      params = val();
      params->set_list();
      {
        const auto& map = static_cast<const t_map&>(*resolved_type);
        params->add_list(
            gen_type(generator, program, defns_schema, *map.key_type()));
        params->add_list(
            gen_type(generator, program, defns_schema, *map.val_type()));
      }
      break;
    case t_type::type::t_enum: {
      if (defns_schema && generator) {
        auto enum_schema =
            generator->gen_schema(static_cast<const t_enum&>(*resolved_type));
        add_as_definition(*defns_schema, "enumDef", std::move(enum_schema));
      }
      type_name->add_map(val("enumType"), typeUri(*resolved_type, program));
      break;
    }
    case t_type::type::t_struct: {
      if (defns_schema && generator) {
        if (auto union_type = dynamic_cast<const t_union*>(resolved_type)) {
          auto union_schema = generator->gen_schema(*union_type);
          add_as_definition(*defns_schema, "unionDef", std::move(union_schema));
        } else if (
            auto exception_type =
                dynamic_cast<const t_exception*>(resolved_type)) {
          auto ex_schema = generator->gen_schema(*exception_type);
          add_as_definition(
              *defns_schema, "exceptionDef", std::move(ex_schema));
        } else {
          auto struct_type = static_cast<const t_struct*>(resolved_type);
          auto struct_schema = generator->gen_schema(*struct_type);
          add_as_definition(
              *defns_schema, "structDef", std::move(struct_schema));
        }
      }
      type_name->add_map(
          val([&] {
            if (dynamic_cast<const t_union*>(resolved_type)) {
              return "unionType";
            } else if (dynamic_cast<const t_exception*>(resolved_type)) {
              return "exceptionType";
            } else {
              return "structType";
            }
          }()),
          typeUri(*resolved_type, program));
      break;
    }
    default:
      assert(false);
  }
  schema->add_map(val("name"), std::move(type_name));
  if (params) {
    schema->add_map(val("params"), std::move(params));
  }
  return schema;
}

std::unique_ptr<t_const_value> gen_type(
    const t_type& type, const t_program* program) {
  return gen_type(nullptr, program, nullptr, type);
}

void schematize_recursively(
    schematizer* generator,
    const t_program* program,
    t_const_value* defns_schema,
    const t_type& type) {
  auto schema = mapval();
  auto type_name = mapval();
  std::unique_ptr<t_const_value> params;

  auto* resolved_type = &type;
  while (auto* typedf = dynamic_cast<const t_typedef*>(resolved_type)) {
    if (typedf->typedef_kind() != t_typedef::kind::defined) {
      resolved_type = &*typedf->type();
      continue;
    }
    return;
  }

  switch (resolved_type->get_type_value()) {
    case t_type::type::t_void:
    case t_type::type::t_bool:
    case t_type::type::t_byte:
    case t_type::type::t_i16:
    case t_type::type::t_i32:
    case t_type::type::t_i64:
    case t_type::type::t_double:
    case t_type::type::t_float:
    case t_type::type::t_string:
    case t_type::type::t_binary:
      break;
    case t_type::type::t_list:
      schematize_recursively(
          generator,
          program,
          defns_schema,
          *static_cast<const t_list&>(*resolved_type).elem_type());
      break;
    case t_type::type::t_set:
      schematize_recursively(
          generator,
          program,
          defns_schema,
          *static_cast<const t_set&>(*resolved_type).elem_type());
      break;
    case t_type::type::t_map: {
      const auto& map = static_cast<const t_map&>(*resolved_type);
      schematize_recursively(generator, program, defns_schema, *map.key_type());
      schematize_recursively(generator, program, defns_schema, *map.val_type());
      break;
    }
    case t_type::type::t_enum: {
      auto enum_schema =
          generator->gen_schema(static_cast<const t_enum&>(*resolved_type));
      add_as_definition(*defns_schema, "enumDef", std::move(enum_schema));
      break;
    }
    case t_type::type::t_struct: {
      auto schema = generator->gen_schema(
          static_cast<const t_structured&>(*resolved_type));
      std::string def_type = [&] {
        if (resolved_type->is_union()) {
          return "unionDef";
        } else if (resolved_type->is_exception()) {
          return "exceptionDef";
        } else {
          return "structDef";
        }
      }();
      add_as_definition(*defns_schema, def_type, std::move(schema));
      break;
    }
    default:
      assert(false);
  }
}

const t_enum* find_enum(const t_program* program, const std::string& enum_uri) {
  // May be null in unit tests.
  return program
      ? dynamic_cast<const t_enum*>(program->scope()->find_def(enum_uri))
      : nullptr;
}

void add_qualifier(const t_enum* t_enum, t_const_value& schema, int enum_val) {
  auto qualifier_schema = val(enum_val);
  if (t_enum) {
    qualifier_schema->set_is_enum();
    qualifier_schema->set_enum(t_enum);
    qualifier_schema->set_enum_value(t_enum->find_value(enum_val));
  }
  schema.add_map(val("qualifier"), std::move(qualifier_schema));
}

void add_fields(
    schematizer* generator,
    const t_program* program,
    t_const_value* defns_schema,
    t_const_value& schema,
    const std::string& fields_name,
    node_list_view<const t_field> fields,
    schematizer::InternFunc& intern_value) {
  auto fields_schema = val();
  fields_schema->set_list();

  const auto* field_qualifier_enum =
      find_enum(program, "facebook.com/thrift/type/FieldQualifier");

  for (const auto& field : fields) {
    auto field_schema = val();
    field_schema->set_map();
    add_definition(*field_schema, field, program, intern_value);
    field_schema->add_map(val("id"), val(field.id()));

    add_qualifier(field_qualifier_enum, *field_schema, [&] {
      switch (field.qualifier()) {
        case t_field_qualifier::none:
        case t_field_qualifier::required:
          return 3; // Fill
        case t_field_qualifier::optional:
          return 1; // Optional
        case t_field_qualifier::terse:
          return 2; // Terse
      }
      assert(false);
      return 0; // Default
    }());

    field_schema->add_map(
        val("type"), gen_type(generator, program, defns_schema, *field.type()));
    if (auto deflt = field.default_value()) {
      assert(program);
      auto clone = deflt->clone();
      clone->set_ttype(field.type());
      auto id = intern_value(std::move(clone), const_cast<t_program*>(program));
      field_schema->add_map(val("customDefault"), val(id));
    }
    fields_schema->add_list(std::move(field_schema));
  }

  schema.add_map(val(fields_name), std::move(fields_schema));
}
} // namespace

std::unique_ptr<t_const_value> schematizer::gen_schema(
    const t_structured& node) {
  auto schema = val();
  schema->set_map();
  add_definition(*schema, node, node.program(), intern_value_);
  add_fields(
      this,
      node.program(),
      nullptr,
      *schema,
      "fields",
      node.fields(),
      intern_value_);

  if (node.is_exception()) {
    const auto& ex = static_cast<const t_exception&>(node);
    schema->add_map(val("safety"), val(ex.safety()));
    schema->add_map(val("kind"), val(ex.kind()));
    schema->add_map(val("blame"), val(ex.blame()));
  }

  return schema;
}

std::unique_ptr<t_const_value> schematizer::gen_full_schema(
    const t_service& node) {
  auto schema = val();
  schema->set_map();

  auto dfns_schema = val();
  dfns_schema->set_list();

  auto svc_schema = gen_schema(node);
  add_as_definition(*dfns_schema, "serviceDef", std::move(svc_schema));

  for (const auto& func : node.functions()) {
    for (const auto& ret : func.return_types()) {
      // TODO: Handle sink, stream, interactions
      if (!func.sink_or_stream() && !ret->is_service()) {
        schematize_recursively(
            this, node.program(), dfns_schema.get(), *ret->get_true_type());
      }
    }

    for (const auto& field : func.params().fields()) {
      schematize_recursively(
          this, node.program(), dfns_schema.get(), *field.type());
    }

    if (func.exceptions()) {
      for (const auto& field : func.exceptions()->fields()) {
        schematize_recursively(
            this, node.program(), dfns_schema.get(), *field.type());
      }
    }
  }

  if (auto parent = node.extends()) {
    add_as_definition(*dfns_schema, "serviceDef", gen_schema(*parent));
  }

  schema->add_map(val("definitions"), std::move(dfns_schema));
  return schema;
}

std::unique_ptr<t_const_value> schematizer::gen_schema(const t_service& node) {
  auto svc_schema = val();
  svc_schema->set_map();
  add_definition(*svc_schema, node, node.program(), intern_value_);

  auto functions_schema = val();
  functions_schema->set_list();

  const auto* func_qualifier_enum =
      find_enum(node.program(), "facebook.com/thrift/type/FunctionQualifier");

  for (const auto& func : node.functions()) {
    auto func_schema = val();
    func_schema->set_map();
    add_definition(*func_schema, func, node.program(), intern_value_);

    add_qualifier(func_qualifier_enum, *func_schema, [&] {
      switch (func.qualifier()) {
        case t_function_qualifier::unspecified:
          return 0;
        case t_function_qualifier::one_way:
          return 1;
        case t_function_qualifier::idempotent:
          return 2;
        case t_function_qualifier::read_only:
          return 3;
      }
      assert(false);
      return 0; // Default
    }());

    auto return_types_schema = val();
    return_types_schema->set_list();
    for (const auto& ret : func.return_types()) {
      // TODO: Handle sink, stream, interactions
      if (!func.sink_or_stream() && !ret->is_service()) {
        auto return_type_schema = val();
        return_type_schema->set_map();
        return_type_schema->add_map(
            val("thriftType"), gen_type(*ret->get_true_type(), node.program()));
        return_types_schema->add_list(std::move(return_type_schema));
      }
    }
    func_schema->add_map(val("returnTypes"), std::move(return_types_schema));

    auto param_list_schema = val();
    param_list_schema->set_map();
    add_fields(
        this,
        node.program(),
        nullptr,
        *param_list_schema,
        "fields",
        func.params().fields(),
        intern_value_);
    func_schema->add_map(val("paramlist"), std::move(param_list_schema));

    if (func.exceptions()) {
      add_fields(
          this,
          node.program(),
          nullptr,
          *func_schema,
          "exceptions",
          func.exceptions()->fields(),
          intern_value_);
    }

    functions_schema->add_list(std::move(func_schema));
  }
  svc_schema->add_map(val("functions"), std::move(functions_schema));

  if (auto parent = node.extends()) {
    auto ref = mapval();
    ref->add_map(val("uri"), typeUri(*parent, node.program()));
    svc_schema->add_map(val("baseService"), std::move(ref));
  }

  return svc_schema;
}

std::unique_ptr<t_const_value> schematizer::gen_schema(const t_const& node) {
  const auto* program = node.program();
  assert(program);

  auto schema = val();
  schema->set_map();
  add_definition(*schema, node, program, intern_value_);

  schema->add_map(val("type"), gen_type(*node.type(), program));

  auto clone = node.value()->clone();
  clone->set_ttype(node.type());
  auto id = intern_value_(std::move(clone), const_cast<t_program*>(program));
  schema->add_map(val("value"), val(id));

  return schema;
}

std::unique_ptr<t_const_value> schematizer::gen_schema(const t_enum& node) {
  auto schema = val();
  schema->set_map();
  add_definition(*schema, node, node.program(), intern_value_);

  auto values = val();
  values->set_list();

  for (const auto& value : node.values()) {
    auto value_schema = val();
    value_schema->set_map();
    add_definition(*value_schema, value, node.program(), intern_value_);
    value_schema->add_map(val("value"), val(value.get_value()));
    values->add_list(std::move(value_schema));
  }

  schema->add_map(val("values"), std::move(values));

  return schema;
}

std::unique_ptr<t_const_value> schematizer::gen_schema(const t_program& node) {
  auto schema = val();
  schema->set_map();
  add_definition(*schema, node, &node, intern_value_);

  // The remaining fields are intern IDs and have to be stiched in by the
  // caller.

  return schema;
}

std::unique_ptr<t_const_value> schematizer::gen_schema(const t_typedef& node) {
  auto schema = mapval();
  add_definition(*schema, node, node.program(), intern_value_);
  schema->add_map(val("type"), gen_type(*node.type(), node.program()));
  return schema;
}

t_program::value_id schematizer::default_intern_value(
    std::unique_ptr<t_const_value> val, t_program* program) {
  return program->intern_value(std::move(val));
}

std::unique_ptr<t_const_value> wrap_with_protocol_value(
    const t_const_value& value, t_type_ref ttype) {
  auto ret = val();
  ret->set_map();
  ret->set_ttype(ttype);
  switch (value.get_type()) {
    case t_const_value::CV_BOOL:
      ret->add_map(val("boolValue"), val(value.get_bool()));
      break;
    case t_const_value::CV_INTEGER:
      ret->add_map(val("i64Value"), val(value.get_integer()));
      break;
    case t_const_value::CV_DOUBLE:
      ret->add_map(val("doubleValue"), val(value.get_double()));
      break;
    case t_const_value::CV_STRING:
      ret->add_map(val("stringValue"), val(value.get_string()));
      break;
    case t_const_value::CV_MAP: {
      auto map = val();
      map->set_map();
      for (const auto& map_elem : value.get_map()) {
        map->add_map(
            wrap_with_protocol_value(*map_elem.first, ttype),
            wrap_with_protocol_value(*map_elem.second, ttype));
      }
      ret->add_map(val("mapValue"), std::move(map));
      break;
    }
    case t_const_value::CV_LIST: {
      auto list = val();
      list->set_list();
      for (const auto& list_elem : value.get_list()) {
        list->add_list(wrap_with_protocol_value(*list_elem, ttype));
      }
      ret->add_map(val("listValue"), std::move(list));
      break;
    }
  }
  return ret;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
