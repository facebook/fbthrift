exception FooEx { }

service PubSubStreamingService {
  stream i32 returnstream(1: i32 i32_from, 2: i32 i32_to);
  void takesstream(stream i32 instream, 1: i32 other_param);
  stream binary different(stream i32 foo, 1: i64 firstparam);

  void normalthrows(stream i32 foo) throws (1: FooEx e);
}
