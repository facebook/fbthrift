cpp_include "<unordered_map>"
cpp_include "<boost/interprocess/containers/flat_map.hpp>"

namespace cpp apache.thrift.reflection

// A type id is a 64-bit value.  The least significant 5 bits of the type id
// are the actual type (one of the values of the Type enum, below).  The
// remaining 59 bits are a unique identifier for container or user-defined
// types, or 0 for base types (void, string, bool, byte, i16, i32, i64,
// and double)

// IMPORTANT!
// These values must be exactly the same as those defined in
// thrift/compiler/parse/t_type.h
enum Type {
  TYPE_VOID,
  TYPE_STRING,
  TYPE_BOOL,
  TYPE_BYTE,
  TYPE_I16,
  TYPE_I32,
  TYPE_I64,
  TYPE_DOUBLE,
  TYPE_ENUM,
  TYPE_LIST,
  TYPE_SET,
  TYPE_MAP,
  TYPE_STRUCT,
  TYPE_SERVICE,
  TYPE_PROGRAM,
  TYPE_STREAM,
  TYPE_FLOAT,
}

struct StructField {
  1: bool isRequired,
  2: i64 type,
  3: string name,
  4: optional map<string, string>
    ( cpp.template = "boost::container::flat_map" )
    annotations,
  5: i16 order, // lexical order of this field, 0-based
}

struct DataType {
  1: string name,
  2: optional map<i16, StructField>
    ( cpp.template = "boost::container::flat_map" )
    fields,
  3: optional i64 mapKeyType,
  4: optional i64 valueType,
  5: optional map<string, i32> ( cpp.template = "boost::container::flat_map" )
    enumValues,
}

struct Schema {
  1: map<i64, DataType> ( cpp.template = "std::unordered_map" ) dataTypes,
  // name to type
  2: map<string, i64> ( cpp.template = "std::unordered_map" ) names,
}
