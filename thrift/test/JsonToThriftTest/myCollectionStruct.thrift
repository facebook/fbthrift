namespace cpp2 apache.thrift.test
namespace java thrift.test

struct myTestStruct {
  1: double d,
}

struct myCollectionStruct {
  1: list<double> l,
  2: set<double> s,
  3: map<double, double> m,
  4: list<myTestStruct> ll,
}
