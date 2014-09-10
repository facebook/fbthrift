namespace cpp apache.thrift.frozen.schema

struct Field {
  // layout id, indexes into layouts
  1: i16 layoutId,
  // field offset:
  //  < 0: -(bit offset)
  //  >= 0: byte offset
  2: i16 offset = 0,
}

struct Layout {
  1: i32 size = 0,
  2: i16 bits = 0,
  3: map<i16, Field> fields;
  4: string typeName;
}

const i32 kCurrentFrozenFileVersion = 1;

struct Schema {
  // File format version, incremented on breaking changes to Frozen2
  // implementation.  Only backwards-compatibility is guaranteed.
  4: i32 fileVersion = 0;
  // Field type names may not change unless relaxTypeChecks is set.
  1: bool relaxTypeChecks = 0;
  2: map<i16, Layout> layouts;
  3: i16 rootLayout = 0;
}
