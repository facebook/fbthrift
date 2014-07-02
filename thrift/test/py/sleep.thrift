namespace cpp test
namespace cpp2 test
namespace py test.sleep

service SleepService {
  void sleep(1: i32 seconds);
  string space(1: string str);
}
