namespace py typedef_test.included

enum Foo {
    bar = 1
    baz = 3
}

struct Pod {
     1: i32 potato
     2: string carrot
}

typedef list<Pod> Pods

exception Beep {
}

typedef i64 Int64

typedef Beep BeepAlso
