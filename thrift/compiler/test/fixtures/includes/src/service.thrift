include "module.thrift"
include "includes.thrift"
namespace java.swift test.fixtures.includes

service MyService {
  void query(1: module.MyStruct s, 2: includes.Included i)
}
