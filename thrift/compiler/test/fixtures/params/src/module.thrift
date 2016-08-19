namespace java.swift test.fixtures.params

service NestedContainers {
  void mapList(1: map<i32, list<i32>> foo)
  void mapSet(1: map<i32, set<i32>> foo)
  void listMap(1: list<map<i32, i32>> foo)
  void listSet(1: list<set<i32>> foo)

 void turtles(1: list<list<map<i32, map<i32, set<i32>>>>> foo)
}
