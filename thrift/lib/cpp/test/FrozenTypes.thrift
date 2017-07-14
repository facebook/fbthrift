
namespace cpp "FrozenTypes"

struct Pod {
  3: i32 a;
  2: i16 b;
  1: byte c;
}

struct Person {
  1: required string name;
  2: required i64 id;
  4: set<i32> nums;
  3: optional double dob;
}

struct Team {
  2: optional map<i64, Person> peopleById;
  4: optional map<i64, i64> (cpp.template = 'std::unordered_map') ssnLookup;
  3: optional map<string, Person> peopleByName;
  1: optional set<string> projects;
}
