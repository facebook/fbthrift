
struct SimpleData {
  1: i32 a,
  2: double b,
  3: string c,
}

struct SecondLevel {
  1: map<string, SimpleData> a,
  2: map<i32, string> b,
  3: list<SimpleData> c,
  4: list<string> d,
  5: list<i32> e,
}

struct FirstLevel {
  1: map<string, SecondLevel> a,
  2: list<string> b,
  3: SimpleData c,
}

