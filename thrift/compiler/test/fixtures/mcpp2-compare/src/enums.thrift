namespace cpp2 facebook.ns.qwerty

enum AnEnumA {
  FIELDA = 0;
}

enum AnEnumB {
  FIELDA = 0;
  FIELDB = 2;
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


const map<AnEnumA, set<AnEnumB>> ConstantMap1 = {
  AnEnumA.FIELDA: [
    AnEnumB.FIELDA,
    AnEnumB.FIELDB,
  ]
}

const map<AnEnumC, map<i16, set<i16>>> ConstantMap2 = {
  AnEnumC.FIELDC : ConstantMap1,
}
