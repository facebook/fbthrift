#
# exp = the expected new value of dst after merging
# nil = the expected new value of src after merging with rvalue-ref
#

cpp_include "thrift/test/fatal_merge_types.h"

namespace cpp apache.thrift.test

enum FooBar {
  kFoo,
  kBar,
}

struct EnumExample {
  1: FooBar src,
  2: FooBar dst,
  3: FooBar exp,
  4: FooBar nil,
}

const EnumExample kEnumExample = {
  "src": FooBar.kBar,
  "dst": FooBar.kFoo,
  "exp": FooBar.kBar,
  "nil": FooBar.kBar,
}

struct Basic {
  1: string b,
}

struct BasicExample {
  1: Basic src,
  2: Basic dst,
  3: Basic exp,
  4: Basic nil,
}

const BasicExample kBasicExample = {
  "src": {
    "b": "hello",
  },
  "dst": {
  },
  "exp": {
    "b": "hello",
  },
  "nil": {
  },
}

struct BasicListExample {
  1: list<Basic> src,
  2: list<Basic> dst,
  3: list<Basic> exp,
  4: list<Basic> nil,
}

const BasicListExample kBasicListExample = {
  "src": [
    {
      "b": "hello",
    },
  ],
  "dst": [
  ],
  "exp": [
    {
      "b": "hello",
    },
  ],
  "nil": [
    {
    },
  ],
}

struct BasicSetExample {
  1: set<Basic> src,
  2: set<Basic> dst,
  3: set<Basic> exp,
  4: set<Basic> nil,
}

const BasicSetExample kBasicSetExample = {
  "src": [
    {
      "b": "hello",
    },
  ],
  "dst": [
  ],
  "exp": [
    {
      "b": "hello",
    },
  ],
  "nil": [
    {
      "b": "hello", # can't actually move elements
    },
  ],
}

struct BasicMapExample {
  1: map<string, Basic> src,
  2: map<string, Basic> dst,
  3: map<string, Basic> exp,
  4: map<string, Basic> nil,
}

const BasicMapExample kBasicMapExample = {
  "src": {
    "foo": {
      "b": "hello",
    },
  },
  "dst": {
  },
  "exp": {
    "foo": {
      "b": "hello",
    },
  },
  "nil": {
    "foo": {
    },
  },
}

struct Nested {
  1: Basic a,
  2: Basic b,
  3: string c,
  4: string d,
}

struct NestedExample {
  1: Nested src,
  2: Nested dst,
  3: Nested exp,
  4: Nested nil,
}

const NestedExample kNestedExample = {
  "src": {
    "b": {
      "b": "hello",
    },
    "d": "bar",
  },
  "dst": {
    "a": {
      "b": "world",
    },
    "c": "foo",
  },
  "exp": {
    "a": {
      "b": "", # shouldn't this be "world"?
    },
    "b": {
      "b": "hello",
    },
    "c": "", # shouldn't this be "foo"?
    "d": "bar",
  },
  "nil": {
  },
}

struct NestedRefUnique {
  1: Basic a (cpp.ref_type = "unique"),
  2: Basic b (cpp.ref_type = "unique"),
  3: string c,
  4: string d,
}

struct NestedRefUniqueExample {
  1: NestedRefUnique src,
  2: NestedRefUnique dst,
  3: NestedRefUnique exp,
  4: NestedRefUnique nil,
}

const NestedRefUniqueExample kNestedRefUniqueExample = {
  "src": {
    "b": {
      "b": "hello",
    },
    "d": "bar",
  },
  "dst": {
    "a": {
      "b": "world",
    },
    "c": "foo",
  },
  "exp": {
    # "a": {"b": ""}, # why not this?
    "b": {
      "b": "hello",
    },
    "c": "", # shouldn't this be "foo"?
    "d": "bar",
  },
  "nil": {
  },
}

struct NestedRefShared {
  1: Basic a (cpp.ref_type = "shared"),
  2: Basic b (cpp.ref_type = "shared"),
  3: string c,
  4: string d,
}

struct NestedRefSharedExample {
  1: NestedRefShared src,
  2: NestedRefShared dst,
  3: NestedRefShared exp,
  4: NestedRefShared nil,
}

const NestedRefSharedExample kNestedRefSharedExample = {
  "src": {
    "b": {
      "b": "hello",
    },
    "d": "bar",
  },
  "dst": {
    "a": {
      "b": "world",
    },
    "c": "foo",
  },
  "exp": {
    # should "a" be {"b": ""}?
    "b": {
      "b": "hello",
    },
    "c": "", # shouldn't this be "foo"?
    "d": "bar",
  },
  "nil": {
  },
}

struct NestedRefSharedConst {
  1: Basic a (cpp.ref_type = "shared_const"),
  2: Basic b (cpp.ref_type = "shared_const"),
  3: string c,
  4: string d,
}

struct NestedRefSharedConstExample {
  1: NestedRefSharedConst src,
  2: NestedRefSharedConst dst,
  3: NestedRefSharedConst exp,
  4: NestedRefSharedConst nil,
}

const NestedRefSharedConstExample kNestedRefSharedConstExample = {
  "src": {
    "b": {
      "b": "hello",
    },
    "d": "bar",
  },
  "dst": {
    "a": {
      "b": "world",
    },
    "c": "foo",
  },
  "exp": {
    # should "a" be {"b": ""}?
    "b": {
      "b": "hello",
    },
    "c": "", # shouldn't this be "foo"?
    "d": "bar",
  },
  "nil": {
  },
}

typedef i32 (cpp.type = 'CppHasANumber', cpp.indirection = '.number') HasANumber

struct Indirection {
  1: i32 real,
  2: HasANumber fake,
}

struct IndirectionExample {
  1: Indirection src,
  2: Indirection dst,
  3: Indirection exp,
  4: Indirection nil,
}

const IndirectionExample kIndirectionExample = {
  "src": {
    "real": 45,
    "fake": 33,
  },
  "dst": {},
  "exp": {
    "real": 45,
    "fake": 33,
  },
  "nil": {
    "real": 45,
    "fake": 33,
  }
}
