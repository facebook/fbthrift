include "thrift/lib/py/util/tests/parent.thrift"

namespace py thrift.test.child

exception ChildError {
  1: required string message
  2: optional i32 errorCode
} (message = 'message')

struct SomeStruct {
  1: string data
}

enum AnEnum {
  FOO = 1,
  BAR = 2,
}

service ChildService extends parent.ParentService {
  oneway void shoutIntoTheWind(1: string message)

  i32 mightFail(1: string message) throws (
    1: parent.ParentError parent_ex
    2: ChildError child_ex
  )

  SomeStruct doSomething(
    19: string message
    2: SomeStruct input1,
    5: SomeStruct input2,
    3: AnEnum e,
    4: binary data,
  )
}
