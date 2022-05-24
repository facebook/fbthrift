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

include "thrift/annotation/thrift.thrift"
include "thrift/lib/thrift/id.thrift"
include "thrift/lib/thrift/type.thrift"

cpp_include "<thrift/lib/cpp/FieldId.h>"

// The canonical representations for a Thrift schema.
//
// Note that, while these definitions may eventually also be used in an
// abstract syntax tree (AST) representation, that is not the primary goal
// of the definitions in this file.
//
// Specifically, the AST rep needs to be a ~pure representation of the source
// file, with as little 'interpretation' as possible. Semantic analysis can be
// performed on the AST representation to distill the result of interpreting the
// source file in to the representations found in this file.
//
// In contrast to an AST, the definitions in this file should include
// the absolute minimal amount of information need to work with a 'dynamic'
// thrift value (e.g., one where the type is only known at runtime).
@thrift.v1alpha
package "facebook.com/thrift/type"

// The attributes that can be associated with any Thrift definition.
struct Definition {
  // The un-scoped 'name' for this definition.
  //
  // Changing this is backward compatible for all Thrift v1+
  // supported protocols (e.g. Binary, Compact).
  //
  // Changing this is backward *incompatible* for any component
  // that converts a 'name' into another form of identity (e.g. uri,
  // field id, enum value's value). For example:
  //  - generated code,
  //  - IDL const and literal values,
  //  - YAML parsers.
  //  - Protocols deprecated in v1+, e.g. JSON and SimpleJson.
  //
  // TODO(afuller): Support aliases to help with renaming.
  1: string name;

  // The globally unique Thrift URI for this definition.
  //
  // Must match the pattern:
  // {domainLabel}(.{domainLabel})+/{pathSegment}(/{pathSegment})*/{name}
  // Where:
  //  - domainLabel: [a-z0-9-]+
  //  - pathSegment: [a-z0-9_-]+
  //  - name:        [a-zA-Z0-9_-]+
  //
  // Changing this is backward *incompatible* for any component that
  // uses URI-based lookup. For example:
  //  - URI-base AST features.
  //  - serialized Any values,
  // This means that value previously serialized in an Any with the old
  // URI can no longer be deserialized.
  //
  // TODO(afuller): Support aliases to help with renaming.
  2: string uri;

  // The annotations associated with this definition, for the current context.
  //
  // For example, only annotations explicitly marked with '@scope.Schema' are
  // present in the runtime schema, while all annotations are present in the AST.
  //
  // TODO(afuller): Consider supporting unstructured annotations for
  // backward compatibility, by documenting and populating a map<string, string>
  // pseudo-structured annotation value.
  3: id.AnnotationIds annotations;
}

// A Thrift enum value.
//
// enum ... {
//   {def.name} = {value}
// }
struct EnumValue {
  // The definition attributes.
  @thrift.Mixin
  1: Definition def;

  // The associated numeric value.
  //
  // Changing value is always backward *incompatible*.
  2: i32 value;
}

// A Thrift enum.
//
// enum {def.name} { ... values ... }
struct Enum {
  // The definition attributes.
  @thrift.Mixin
  1: Definition def;

  // The values, in the order as defined in the IDL/AST.
  //
  // Changing the order of values is always backward compatible.
  2: list<EnumValue> values;
}

// A field id is a signed 16-bit integer.
typedef i16 (
  cpp.adapter = "::apache::thrift::StaticCastAdapter<::apache::thrift::FieldId, int16_t>",
) FieldId

// The field qualifier.
enum FieldQualifier {
  Default = 0, // Terse v1+, Fill pre-v1
  Optional = 1, // Written if explicitly 'set'
  Terse = 2, // Written if not 'empty'
  Fill = 3, // Always written.
}

// A Thrift field.
//
// {id}: {qualifier} {type} {def.name} = {customDefault}
//
struct Field {
  // The static ID specified for the field.
  //
  // Changing the field ID is always backward *incompatible*.
  1: FieldId id;

  // The qualifier for the field.
  //
  // TODO(afuller): Document compatibility semantics.
  2: FieldQualifier qualifier;

  // The type of the field.
  //
  // TODO(afuller): Document compatibility semantics.
  3: type.Type type;

  // The definition attributes.
  @thrift.Mixin
  4: Definition def;

  // The custom default value for this field.
  //
  // If no value is set, the intrinsic default for the field type
  // is used.
  5: id.ValueId customDefault;
}

// A container for the fields of a structured type (e.g. struct, union, exception).
//
// Changing the order of fields is always backward compatible.
//
// TODO(afuller): Add native wrappers that provide O(1) access to fields by id,
// name, type, etc.
@thrift.Experimental // TODO: Adapt!
typedef list<Field> Fields

// A Thrift struct.
//
// struct {def.name} { ... fields ... }
struct Struct {
  // The definition attributes.
  @thrift.Mixin
  1: Definition def;

  // The fields, in the order as defined in the IDL/AST.
  //
  // Changing the order of the fields is always backward compatible.
  2: Fields fields;
}

// A Thrift union.
//
// union {def.name} { ... fields ... }
struct Union {
  // The definition attributes.
  @thrift.Mixin
  1: Definition def;

  // The fields, in the order as defined in the IDL/AST.
  //
  // Changing the order of the fields is always backward compatible.
  2: Fields fields;
}

// A Thrift exception.
//
// exception {def.name} { ... fields ... }
struct Exception {
  // The definition attributes.
  @thrift.Mixin
  1: Definition def;

  // The fields, in the order as defined in the IDL/AST.
  //
  // Changing the order of the fields is always backward compatible.
  2: Fields fields;
}
