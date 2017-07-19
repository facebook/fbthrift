namespace cpp2 facebook.ns.qwerty

enum AnEnumA {
  FIELDA = 0;
}

enum AnEnumB {
  FIELDB = 0;
}

enum AnEnumC {
  FIELDC = 0;
}

const map<string, AnEnumB> MapStringEnum = {
  "0": FIELDB,
};

const map<AnEnumC, string> MapEnumString = {
  FIELDC: "unknown",
};


enum AnEnumD {
  FIELDD = 0,
}

enum AnEnumE {
  FIELDA = 0,
}

struct SomeStruct {
  1: i32 fieldA
}
