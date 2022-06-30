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

include "thrift/annotation/api.thrift"
include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"
include "thrift/lib/thrift/any.thrift"
include "thrift/lib/thrift/id.thrift"
include "thrift/lib/thrift/standard.thrift"
include "thrift/lib/thrift/type.thrift"

cpp_include "<thrift/lib/cpp/Field.h>"

/**
 * The canonical representations for a Thrift schema.
 *
 * The definitions in this file contain the absolute minimal amount of
 * information needed to work with a 'dynamic' thrift value (e.g., one where the
 * type is only known at runtime).
 *
 *
 * Note that, while an AST representation may reuse some of the definitions from
 * this file, it has significantly different design goals. For example, an ideal
 * AST representation is a ~pure representation of the source file, with as
 * little 'interpretation' as possible. While a 'schema' is the result of
 * performing semantic analysis on an AST, to distill the result of interpreting
 * the source file in to the representations found in this file.
 */
@thrift.v1alpha
package "facebook.com/thrift/type"

/** A list of parsed packages, accessible via a `PackageId`. */
typedef list<standard.UriStruct> PackageList

/** The attributes that can be associated with any Thrift definition. */
struct Definition {
  /**
   * The un-scoped 'name' for this definition.
   *
   * Changing this is backward compatible for all Thrift v1+ supported protocols
   * (e.g. Binary, Compact).
   *
   * Changing this is backward *incompatible* for any component that converts a
   * 'name' into another form of identity (e.g. uri, field id, enum value's
   * value). For example:
   *  - generated code,
   *  - IDL const and literal values,
   *  - YAML parsers.
   *  - Protocols deprecated in v1+, e.g. JSON and SimpleJson.
   */
  // TODO(afuller): Support aliases to help with renaming.
  @api.Immutable
  1: string name;

  /**
   * The globally unique Thrift URI for this definition.
   *
   * Must match the pattern:
   *     {domainLabel}(.{domainLabel})+(/{pathSegment})+/{name}
   * Where:
   *  - domainLabel: [a-z0-9-]+
   *  - pathSegment: [a-z0-9_-]+
   *  - name:        [a-zA-Z0-9_-]+
   *
   * Changing this is backward *incompatible* for any component that
   * uses URI-based lookup. For example:
   *  - URI-base AST features.
   *  - serialized Any values,
   * This means that value previously serialized in an Any with the old
   * URI can no longer be deserialized.
   */
  // TODO(afuller): Support aliases to help with renaming.
  2: standard.Uri uri;

  /**
   * The annotations associated with this definition, for the current context.
   *
   * For example, only annotations explicitly marked with '@scope.Schema' are
   * present in the runtime schema, while all annotations are present in the AST.
   */
  // TODO(afuller): Consider supporting unstructured annotations for backward
  // compatibility, by documenting and populating a map<string, string>
  // pseudo-structured annotation value.
  3: id.AnnotationIds annotations;
}

/**
 * A Thrift enum value.
 *
 *     enum ... {
 *       {definition.name} = {value}
 *     }
 */
struct EnumValue {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The associated numeric value.
   *
   * Changing value is always backward *incompatible*.
   */
  2: i32 value;
}

/**
 * A Thrift enum.
 *
 *     enum {definition.name} { ... values ... }
 */
struct Enum {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The values, in the order as defined in the IDL/AST.
   *
   * Changing the order of values is always backward compatible.
   */
  2: list<EnumValue> values;
}

/** A field id is a signed 16-bit integer. */
@cpp.Adapter{
  name = "::apache::thrift::StaticCastAdapter<::apache::thrift::FieldId, int16_t>",
}
typedef i16 FieldId

/** The field qualifier. */
enum FieldQualifier {
  /** `Terse` v1+, `Fill` pre-v1. */
  Default = 0,
  /** Written if explicitly 'set'. */
  Optional = 1,
  /** Written if not 'empty'. */
  Terse = 2,
  /** Always written. */
  Fill = 3,
}

/**
 * A Thrift field.
 *
 *     {id}: {qualifier} {type} {definition.name} = {customDefault}
 */
struct Field {
  /**
   * The static ID specified for the field.
   *
   * Changing the field ID is always backward *incompatible*.
   */
  @api.Immutable
  1: FieldId id;

  /** The qualifier for the field. */
  // TODO(afuller): Document compatibility semantics, and add conformance tests.
  2: FieldQualifier qualifier;

  /** The type of the field. */
  // TODO(afuller): Document compatibility semantics, and add conformance tests.
  3: type.Type type;

  /** The definition attributes. */
  @thrift.Mixin
  4: Definition definition;

  /**
   * The custom default value for this field.
   *
   * If no value is set, the intrinsic default for the field type is used.
   */
  // TODO(afuller): Document compatibility semantics, and add conformance tests.
  5: id.ValueId customDefault;
}

/**
 * A container for the fields of a structured type (e.g. struct, union, exception).
 *
 * Changing the order of fields is always backward compatible.
 */
// TODO(afuller): Add native wrappers that provide O(1) access to fields by id,
// name, type, etc.
@thrift.Experimental // TODO: Adapt!
typedef list<Field> Fields

/**
 * A Thrift struct.
 *
 *     struct {definition.name} { ... fields ... }
 */
struct Struct {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The fields, in the order as defined in the IDL/AST.
   *
   * Changing the order of the fields is always backward compatible.
   */
  2: Fields fields;
}

/**
 * A Thrift union.
 *
 *   union {definition.name} { ... fields ... }
 */
struct Union {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The fields, in the order as defined in the IDL/AST.
   *
   * Changing the order of the fields is always backward compatible.
   */
  2: Fields fields;
}

/**
 * A Thrift exception.
 *
 *   exception {definition.name} { ... fields ... }
 */
struct Exception {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The fields, in the order as defined in the IDL/AST.
   *
   * Changing the order of the fields is always backward compatible.
   */
  2: Fields fields;
}

/** A list of definitions (Structs, Enums, etc), accessible by `DefinitionId`. */
// TODO(afuller): As this can only be one of a fixed set of types, consider
// adding 'union types' to Thrift and use that instead of Any.
typedef any.AnyValueList DefinitionList

/**
 * A Thrift program.
 *
 *     {definition.name}.thrift:
 *       ... {definition.annotations} ...
 *       package {package/definition.uri}
 *
 *       ... {includes} ...
 *
 *       ... {definitions} ...
 */
struct Program {
  /** The definition attributes. */
  @thrift.Mixin
  1: Definition definition;

  /**
   * The parsed package for the program, if available.
   *
   * The unparsed package is available as {definition.uri}.
   */
  // TODO(afuller): Allow 'package' as an ident in Thrift and remove trailing
  // '_' (or change the name slightly in some other way).
  2: id.PackageId package_ (cpp.name = "package");

  /**
   * The included programs, in the order included in the IDL/AST.
   *
   * Changing the order of includes is always backward compatible.
   */
  3: id.IncludeIds includes;

  /**
   * The definitions included in this program, in the order declared in the
   * IDL/AST.
   *
   * Changing the order of definitions is always backward compatible.
   */
  // TODO(afuller): Fix type resolution order bugs in the parser to make this
  // comment true in all cases.
  // TODO(afuller): Add support for all definitions, e.g. service, interface,
  // etc. which currently show up as `void`, to preserve the ordering of the
  // original definitions.
  4: id.DefinitionIds definitions;
}

/** A list of programs, accessible by `ProgramId`. */
typedef list<Program> ProgramList

/**
 * A Thrift schema represented as a collection of Thrift programs and associated
 * schema values.
 */
// TODO(afuller): Add native wrappers/adapters that 'index' all the stored
// information and maybe even converts stored `ExternId`s into pointers to the
// values owned by the schema.
@thrift.Experimental // TODO(afuller): Adapt!
struct Schema {
  /** The programs included in the schema, accessible by `ProgramId`. */
  1: ProgramList programs;

  /** The instantiated types, accessible by `TypeId`. */
  2: type.TypeList types;

  /** The values, accessible by `ValueId`. */
  3: any.AnyValueList values;

  /** The packages, accessible by `PackageId`. */
  4: PackageList packages;

  /** The definitions, accessible by `DefinitionId`. */
  5: DefinitionList definitions;
}
