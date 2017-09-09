include "module.thrift"

namespace cpp2 extra.svc

service ExtraService extends module.ParamService {
  bool simple_function()
  void throws_function()
      throws (1: module.AnException ex,
              2: module.AnotherException aex) (thread="eb")
  bool throws_function2(1: bool param1)
      throws (1: module.AnException ex,
              2: module.AnotherException aex) (priority = "HIGH")
  map<i32, string> throws_function3(1: bool param1, 3: string param2)
      throws (2: module.AnException ex, 5: module.AnotherException aex)
  oneway void oneway_void_ret()
  oneway void oneway_void_ret_i32_i32_i32_i32_i32_param(
      1: i32 param1,
      2: i32 param2,
      3: i32 param3,
      4: i32 param4,
      5: i32 param5)
  oneway void oneway_void_ret_map_setlist_param(
      1: map<string, i64> param1,
      3: set<list<string>> param2) (thread="eb")
  oneway void oneway_void_ret_struct_param(1: module.MyStruct param1)
  oneway void oneway_void_ret_listunion_param(
      1: list<module.ComplexUnion> param1)
}
