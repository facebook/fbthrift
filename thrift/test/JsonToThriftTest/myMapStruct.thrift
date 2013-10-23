namespace java thrift.test

enum Gender {
  MALE = 1,
  FEMALE = 2,
}

struct myMapStruct {
  1: map<string, string> stringMap;
  2: map<bool, string> boolMap;
  3: map<byte, string> byteMap;
  4: map<double, string> doubleMap;
  5: map<Gender, string> enumMap;
}
