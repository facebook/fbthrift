struct Foo {
  1: list<string> a;
  2: optional map<string, list<set<i32>>> b;
  3: i64 c = 7;
  4: optional bool d = 0;
}

service Bar {
  string baz(
    1: set<i32> a,
    2: list<map<i32, set<string>>> b,
    3: optional i64 c,
    4: Foo d,
    5: i64 e = 4,
  );
}

exception Baz {
  1: string message;
}
