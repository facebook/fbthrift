exception FooEx { }

service PubSubStreamingService {
  void client(stream i32 foo) client throws (1: FooEx e);
  void server(stream i32 foo) throws (1: FooEx e);
  void both(stream i32 foo) throws(1: FooEx e) client throws (1: FooEx e);
  stream i32 returnstream(1: i32 i32_from, 2: i32 i32_to);
  void takesstream(stream i32 instream, 1: i32 other_param);
  void clientthrows(stream i32 foostream) client throws (1: FooEx e);

  stream binary different(stream i32 foo, 1: i64 firstparam);
}
