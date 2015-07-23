enum Color {
  RED = 0xFF0000,
  ORANGE = 0xFF6600,
  YELLOW = 0xFFFF00,
  GREEN = 0x00FF00,
  BLUE = 0x0000FF,
  PURPLE = 0x663399,
  WHITE = 0xFFFFFF,
  BLACK = 0x000000,
  GRAY = 0x808080,
}

struct ListStruct {
  1: list<bool> a;
  2: list<i16> b;
  3: list<double> c;
  4: list<string> d;
  5: list<list<i32>> e;
  6: list<map<i32, i32>> f;
  7: list<set<string>> g;
}

union IntUnion {
  1: i32 a;
  2: i32 b;
}

struct Rainbow {
  1: list<Color> colors;
  2: double brightness;
}

struct NestedStructs {
  1: ListStruct ls;
  2: Rainbow rainbow;
  3: IntUnion ints;
}

struct BTreeBranch {
  1: required BTree child;
  2: i16 cost;
}

struct BTree {
  1: i32 min;
  2: i32 max;
  3: string data;
  4: required list<BTreeBranch> children;
}

exception KeyNotFound {
  1: i32 key;
  2: i32 nearest_above;
  3: i32 nearest_below;
}

exception EmptyData {
  1: string message;
}

service TestService {
  string lookup (
    1: BTree root;
    2: i32 key;
  ) throws (
    1: KeyNotFound e;
    2: EmptyData f;
  ),

  void nested (
    1: NestedStructs ns;
  ),

  void listStruct (
    1: ListStruct ls;
  )
}
