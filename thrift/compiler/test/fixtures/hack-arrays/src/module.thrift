struct Foo {
 1: list<string> a;
 2: map<string, list<set<i32>>> b;
}

service Bar {
  string baz(
    1: set<i32> a,
    2: list<map<i32, set<string>>> b,
  );
}
