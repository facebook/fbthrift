---
state: draft
---

# Annotations

## Structured Annotations

Structured annotations are structural constants attached to the named IDL entities, typically representing some metadata or altering the compiler behavior. They are specified via `@Annotation{key1=value1, key2=value2, ...}` syntax before the [Definitions](../index.md). Each definition may have several annotations with no duplicates allowed. `@Annotation` can be used as a syntactic sugar for `@Annotation{}`.

### Grammar

```
StructuredAnnotations ::= NonEmptyStructuredAnnotationList | ""

NonEmptyStructuredAnnotationList ::=
    NonEmptyStructuredAnnotationList StructuredAnnotation
  | StructuredAnnotation

StructuredAnnotation ::= "@" ConstStruct | "@" ConstStructType
```

### Examples

Here's a comprehensive list of various annotated entities.

```
struct FirstAnnotation {
  1: string name;
  2: i64 count = 1;
}

struct SecondAnnotation {
  2: i64 total = 0;
  3: SecondAnnotation recurse (cpp.ref = "True");
  4: bool is_cool;
}

@FirstAnnotation{name="my_type"}
typedef string annotated_string

@FirstAnnotation{name="my_struct", count=3}
@SecondAnnotationstruct
MyStruct {
  @SecondAnnotation{}
  5: annotated_string tag;
}

@FirstAnnotationexception
MyException {
  1: string message;
}

@SecondAnnotation{total=1, is_cool=true}
union MyUnion {
  1: i64 int_value;
  2: string string_value;
}

@SecondAnnotation{total=4, recurse=SecondAnnotation{total=5}}
service MyService {
  @SecondAnnotation
  i64 my_function(2: annotated_string param);
}

@FirstAnnotation{name="shiny"}
enum MyEnum {
  UNKNOWN = 0;
  @SecondAnnotation
  FIRST = 1;
}

@FirstAnnotation{name="my_hack_enum"}
const map<string, string> MyConst = {
  "ENUMERATOR": "value",
}
```

## Unstructured Annotations (Deprecated)

Unstructured annotations are structured as key-value pairs where the key is a string and the value is either a string or a const structure. They may be applied to [definitions](../index.md) in the Thrift language, following that construct.

```
Annotations ::=
  "(" AnnotationList ["," | ";"] ")" |
  "(" ")"

AnnotationList ::=
  AnnotationList ("," | ";") Annotation |
  Annotation

Annotation ::=
  Name [ "=" ( Name | StringLiteral ) ]
```

If a value is not present, then the default value of `"1"` (a string) is assumed.

## Scope Annotations

How to specify what types of definitions an annotation can be applied to. <!--- TODO --->

## Transitive Annotations

How to bundle annotations together. <!--- TODO --->


## Program Annotations

How to apply an annotation to every definition in a [program](program.md). <!--- TODO --->


## Standard Annotations

Annotations provided by Thrift. <!--- TODO --->
