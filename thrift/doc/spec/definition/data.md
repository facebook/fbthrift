# Data Types

Types that can be (de)serialized using [data protocols](../protocol/data.md).

## Primitive Types

Type | Encoding
---|---
`bool` | True(1) or False(0)
`byte` | 8-bit signed integer[^1]
`i16` | 16-bit signed integer[^1]
`i32`, `enum` | 32-bit signed integer[^1]
`i64` | 64-bit signed integer[^1]
`float` | 32-bit floating point[^2]
`double` | 64-bit floating point[^2]
`string` | UTF-8 encoded string[^3]
`binary` | Arbitrary byte string

[^1]: [two's complement](https://en.wikipedia.org/wiki/Two%27s_complement)

[^2]: [IEEE-754](https://en.wikipedia.org/wiki/IEEE_754)

[^3]: [UTF-8 encoding](https://en.wikipedia.org/wiki/UTF-8)

### Enum

Enums are `i32` with named values. Enums *can* represent unnamed values. In ‘open’ enum languages, like C++, this is natively supported. For ‘closed’ enum languages, like Java, special care **must** be taken to allow any value to be read or written to an enum field, even if the value has no name (or the name is not known). Such languages **should** add:

- an explicit `Unrecognized` enum value, which is returned for any unnamed value; and,
- auxiliary methods for get/setting the underlying i32 value.

### Floating Point

Thrift considers all `NaN` values to be identical, so custom NaN values **should** be normalized. Additionally, like most programming languages, it is undefined behavior (UB) to have a `NaN` value in a set or map ‘key’ value (even transitively).

### UTF-8 String

While a Thrift `string` **must** always be valid UTF-8, it is treated is identically to `binary`, ignoring all unicode defined semantics, including  which code points are currently defined in the unicode standard. Any well formed unicode encoded code point **may** be present.

## Primitive Operators

### default

Each primitive type defines an intrinsic default:

- numbers → 0
- string, binary → “”

### identical

Primitive types are identical, if their logical representations are identical. This produces the same results as equal in all cases except floating point, which has two exceptions:

    identical(NaN, NaN) == true
    identical(-0.0, 0.0) == false

### equal

Same-type equality follows the semantic rules defined by that type’s specification. This produces the same results as identical in all cases except floating point, which has two exceptions:

    equal(NaN, NaN) == false
    equal(-0.0, 0.0) == true

Additionally, different numeric types can be tested for equality, in which case they are considered equal when they semantically represent equal numbers:

    equal(2.0, 2.0f) == equal(2.0f, 2) == true
    equal(-0.0, 0.0f) == equal(-0.0f, 0) == true

Equal between other primitive types is ill-defined, and not supported.

### hash

A given hash function **must** be able to hash all primitive types directly. All numeric types **must** produce the same hash for semantically equal values:

    hash(2.0) == hash(2.0f) == hash(2)
    hash(-0.0) == hash(0.0f) == hash(0)

## Container Types

Type | Definition
---|---
`list<{type}>` | Ordered container of `{type}` values.
`set<{type}>` | Unordered container of unique `{type}` values.
`map<{key}, {value}>` | Unordered container of (`{key}`, `{value}`) pairs, where all `{key}`‘s are unique.

Set and map keys can be any Thrift type; however, storing `NaN` in a key value is not supported and leads to implementation-specific UB (as `NaN` is not equal to itself, so it can never be ‘found’).

## Container Operators

All container operators are defined based on values they contain:

- **default** - An empty container.
- **identical** - Two containers are identical if they contain pairwise identical elements. For list, the elements must also be in the same order.
- **equal** - Two containers are equal if they contain pairwise equal elements. For list, the elements must also be in the same order.
- **hash** - Unordered accumulation, for set and map, and ordered accumulation for list and map key-value pairs, of hashes of all contained elements.

## Standard Data Types

Types provided by Thrift.
