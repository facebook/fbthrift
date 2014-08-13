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

struct Schema {
  // Field type names may not change unless relaxTypeChecks is set.
  1: bool relaxTypeChecks = 0;
  2: map<i16, Layout> layouts;
  3: i16 rootLayout = 0;
}
