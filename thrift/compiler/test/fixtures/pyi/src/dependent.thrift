namespace cpp2 simple.dependent
namespace py simple.dependent
namespace py.asyncio simple.dependent_asyncio
namespace py3 simple.dependent


struct Item {
    1: string key
    2: optional binary value
    3: ItemEnum enum_value
}


enum ItemEnum {
  OPTION_ONE = 1
  OPTION_TWO = 2
}
