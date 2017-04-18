// Definitions intentionally spans mutliple lines to test for corner cases
const
i64
fortyTwo
= 42

typedef
map
<i64,
string>
MyMapTypedef

struct
MyStruct
{
  1:
  i64
  MyIntField,
  2:
  string
  MyStringField,
}

enum
MyEnum
{
  VALUE1
  =1,
  VALUE2
  =
  2,
}

service
MyService
{
  void
  ping
  ()
}
