include "thrift/lib/cpp2/frozen/test/Helper.thrift"

namespace cpp2 example2
namespace cpp example1

cpp_include "<unordered_set>"

enum Gender {
  Male = 0,
  Female = 1,
  Other = 2,
}

struct Nesting {
  1 : Helper.Ratio a,
  2 : Helper.Ratio b,
}

struct Pet1 {
  1 : string name,
  2 : optional i32 age
}

struct Person1 {
  5 : optional i32 age,
  2 : float height, // different
  1 : string name,
  4 : list<Pet1> pets,
  6 : Gender gender = Male,
}

struct Pet2 {
  3 : optional float weight,
  1 : string name,
}

struct Person2 {
  1 : string name,
  3 : float weight, // different
  4 : list<Pet2> pets,
  5 : optional i32 age,
}

struct Tiny {
  1 : required string a,
  2 : string b,
  3 : string c,
  4 : string d,
}

struct Place {
1: string name,
2: map<i32, i32> popularityByHour,
}

struct PlaceTest {
  1: map<i64, Place> places,
}

struct EveryLayout {
  1: bool aBool,
  2: i32 aInt,
  3: list<i32> aList,
  4: set<i32> aSet,
  5: set<i32> ( cpp.template = "std::unordered_set" ) aHashSet,
  6: map<i32, i32> aMap,
  7: hash_map<i32, i32> aHashMap,
  8: optional i32 optInt, // optional layout
  9: float aFloat, // trivial layout
  10: optional map<i32, i32> optMap,
}
