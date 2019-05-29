namespace java.swift test.fixtures.annotation

typedef map<i32, i64> (java.swift.type = "com.foo.FastIntLongMap") FMap

struct MyStruct {
  1: i64 intField,
  2: string stringField,
  3: string detailField (java.swift.annotation = "com.foo.Ignored"),
  4: FMap detailMap (java.swift.annotation = "com.foo.Ignored"),
} (java.swift.annotation = "com.foo.Enabled")

struct MyMapping {
  1: map<i64, string> (java.swift.type = "com.foo.FastLongStringMap") lsMap,
  2: map<i32, FMap> (
    java.swift.type = "com.foo.FastIntObjectMap<com.foo.FastIntLongMap>"
  ) ioMap,
}
