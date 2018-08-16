enum FooEnum {
  BAR = 0,
  BAZ = 1,
}

struct BarStruct {
  1: map<FooEnum, FooEnum> e
  2: set<FooEnum> s
}
