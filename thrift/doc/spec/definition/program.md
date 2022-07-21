---
state: draft
---

# Programs

## Universal Names

A Universal Name must be a valid [URI](https://tools.ietf.org/html/rfc3986) that meets the following criteria:

* Must not have a scheme; "fbthrift://" is implied.
* Include a valid case-insensitive domain name as the 'authority'.
* Are case sensitive
* Must have 2 or more path segments
    * One segment must include the package name
    * One segment must include the type
* Must not have a query or a fragment.
* Must be in the canonical form:
    * Domain name labels must be lowercase ASCII letters, numbers, -, or _.
    * Path segments must be ASCII letters, numbers, -, or _.

## Packages

A package declaration contains a Universal Name as package name and optionally program annotations

```
PackageDeclaration:
    {Annotations} package "Domain/Identifier{/Identifier}";
```

## Universal Name for [definitions](../index.md)

[Definitions](../index.md) may have an unstructured annotation `thrift.uri` as an Universal Name. It must be globally unique. If a Universal Name is not specified, one will be generated using a package `{package}/{identifier}`. For example,

```
package "example.com/path/to/file"

struct Foo {
  1: i32 field;
}
```

This is equivalent to

```
package "example.com/path/to/file"

struct Foo {
  1: i32 field (thrift.uri = "example.com/path/to/file/Foo/field");
} (thrift.uri = "example.com/path/to/file/Foo")
```

### Namespace

A namespace is a set of identifiers (names) that are used to identify and refer to objects of various kinds.

```
NamespaceDeclaration:
    namespace Language Identifier{.Identifier};
```

If there is no explicitly defined namespace for a given language, the following namespace will be implied for Thrift officially-supported languages.

* C++: `{reverse(domain)[1:]}.{paths}`
* py3: `{reverse(domain)[1:]}.{paths[:-2] if paths[-1] == filename else paths}`
* hack: `{paths}`
* java: `{reverse(domain)}.{paths}`

For example,

```
package "domain.com/path/to/file"
```

This package name generates the following namespaces

```
namespace cpp2 domain.path.to.file
namespace py3 domain.path.to
namespace hack path.to.file
namespace php path.to.file
namespace java2 com.domain.path.to.file
namespace java.swift com.domain.path.to.file
```

## Imports and Identifiers

Thrift allows including other Thrift files to use any of its consts and types. These can be referenced from another file by prepending the filename to the struct or service name (i.e., `Filename.Identifier`). This means you cannot include multiple files with the same file name, only differing by path.

```
IncludeDeclaration:
    include "{Pathname}/{Filename}.thrift"
```

* [Pathname](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_271) and [Filename](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_170) must be valid POSIX portable pathname and filename.

## Abstract Syntax Tree (AST)

How a program is represented, abstractly. <!--- TODO --->
