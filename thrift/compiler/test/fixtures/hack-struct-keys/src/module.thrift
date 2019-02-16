struct Foo {
  1: i32 fiels;
}

struct Bar {
 1: set<Foo> a;
 2: map<Foo, i32> b;
}

service Baz {
  string qux(
    1: set<Foo> a,
    2: list<Bar> b,
    3: map<Foo, string> c,
  );
}
