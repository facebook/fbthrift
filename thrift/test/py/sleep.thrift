namespace cpp test
namespace cpp2 test
namespace py test.sleep

service SleepBaseService {
  oneway void shutdown();
}

service SleepService extends SleepBaseService {
  void sleep(1: i32 seconds);
  string space(1: string str);
  oneway void noop();
  bool header();
}
