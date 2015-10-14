include "includes.thrift"

struct StructUsingOtherNamespace {
  1: optional includes.Included other (cpp.ref = "true", cpp2.ref = "true")
}
