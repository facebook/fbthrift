const list<i16> int_list = [
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
];

typedef list<i32> I32List
typedef list<list<string>> StrList2D
typedef map<string, i64> StrIntMap
typedef map<string, I32List> StrI32ListMap
typedef map<string, StrI32ListMap> StrStrIntListMapMap
typedef set<I32List> SetI32Lists
typedef set<SetI32Lists> SetSetI32Lists
typedef bool Bool
typedef i64 TimeStamp
typedef byte Byte
typedef float Float
typedef double Double

exception UnusedError {
    1: string message
} (message="message")

exception HardError {
    1: string errortext
    2: required i32 code
} (message="errortext")

enum Color {
  red = 0,
  blue = 1,
  green = 2,
}

union Integers {
    1: byte tiny,
    2: i16 small,
    3: i32 medium,
    4: i64 large,
    5: string unbounded,
}

struct easy {
    1: i32 val,
    2: I32List val_list,
    3: optional string name,
    4: Integers an_int,
}

struct hard {
    1: required i32 val,
    2: required I32List val_list,
    3: required string name,
    4: required Integers an_int,
    5: required string other = "some default",
}

struct mixed {
    1: optional string opt_field = "optional",
    2: required string req_field = "required",
    3: string unq_field = "unqualified",
    4: easy opt_easy_ref (cpp.ref="True"),
    5: required easy req_easy_ref (cpp.ref="True"),
}

service TestingService {
    bool invert(1: bool value)
    i32 complex_action(
        1: string first, 2: string second, 3: i64 third, 4: string fourth
    )
    void takes_a_list(1: I32List ints)
    void take_it_easy(1: i32 how, 2: easy what)
    void pick_a_color(1: Color color)
    void int_sizes(1: byte one, 2: i16 two, 3: i32 three, 4: i64 four)
}
