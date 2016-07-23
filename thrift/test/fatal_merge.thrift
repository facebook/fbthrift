#
# exp = the expected new value of dst after merging
# nil = the expected new value of src after merging with rvalue-ref
#

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
