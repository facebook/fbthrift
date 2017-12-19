include "module.thrift"

struct MyStruct {
  1: string field;
}

struct Combo {
  1: list<list<MyStruct>> listOfOurMyStructLists;
  2: list<module.MyStruct> theirMyStructList;
  3: list<MyStruct> ourMyStructList;
  4: list<list<module.MyStruct>> listOfTheirMyStructList;
}
