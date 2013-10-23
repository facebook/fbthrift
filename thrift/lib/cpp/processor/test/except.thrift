namespace cpp apache.thrift.test

# The iii arguments are important - since they follow the required/optional
# MyArgs(2) struct, they let us test that all arguments are read from the
# input even if deserializing some of them throw an exception

struct Inner {
  1: required i32 i;
}

struct MyArgs {
  1: required string s;
  2: required Inner i;
  3: required list<i32> l;
  4: required map<string,i32> m;
  5: required list<Inner> li;
  6: required map<i32,Inner> mi;
  7: required map<Inner,i32> complex_key;
}

service SampleService {
  i32 return42(1: MyArgs unused, 2: i32 iii);
}


# These versions are identical, but the fields are optional
# See comment at the top of ProcessorExceptions.cpp for the reason
struct Inner2 {
  1: optional i32 i;
}

struct MyArgs2 {
  1: optional string s;
  2: optional Inner2 i;
  3: optional list<i32> l;
  4: optional map<string,i32> m;
  5: optional list<Inner2> li;
  6: optional map<i32,Inner2> mi;
  7: optional map<Inner2,i32> complex_key;
}

service SampleService2 {
  i32 return42(1: MyArgs2 unused, 2: i32 iii);
}
