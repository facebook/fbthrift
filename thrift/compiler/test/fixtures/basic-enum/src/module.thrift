namespace java.swift test.fixtures.swift.enumstrict
namespace java test.fixtures.enumstrict
namespace cpp2 test.fixtures.enumstrict
namespace py3 test.fixtures.enumstrict

enum EmptyEnum {}

enum MyEnum {
  ONE = 1,
  TWO = 2,
}

enum MyBigEnum {
  UNKNOWN = 0,
  ONE = 1,
  TWO = 2,
  THREE = 3,
  FOUR = 4,
  FIVE = 5,
  SIX = 6,
  SEVEN = 7,
  EIGHT = 8,
  NINE = 9,
  TEN = 10,
  ELEVEN = 11,
  TWELVE = 12,
  THIRTEEN = 13,
  FOURTEEN = 14,
  FIFTEEN = 15,
  SIXTEEN = 16,
  SEVENTEEN = 17,
  EIGHTEEN = 18,
  NINETEEN = 19,
}

const MyEnum kOne = MyEnum.ONE

struct MyStruct {
  1: MyEnum myEnum,
  2: MyBigEnum myBigEnum = MyBigEnum.ONE
}
