namespace cpp cpp1
namespace cpp2 cpp2

struct RecTree {
  1: list<RecTree> children
  2: i16 item
}

struct RecList {
  1: optional RecList next (cpp.ref = "true", cpp2.ref = "true")
  3: i16 item
}

struct CoRec {
  1: optional CoRec2 other (cpp.ref = "true", cpp2.ref = "true")
}

struct CoRec2 {
  1: CoRec other
}

struct VectorTest {
  1: list<RecList> lister;
}

struct MyField {
  1: optional i64 some_val
}

struct MyStruct {
  1: optional MyField field (cpp.ref = "true", cpp2.ref = "true")
}

service TestService
{
  RecTree echoTree(1:RecTree tree)
  RecList echoList(1:RecList lst)
  CoRec echoCoRec(1:CoRec item)
}
