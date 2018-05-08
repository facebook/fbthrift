namespace cpp2 testutil.testservice

struct Message {
  1: string message
  2: i64 timestamp
}

exception Error {
}

service StreamService {
  // Generate numbers between `from` to `to`.
  stream i32 range(1: i32 from, 2: i32 to);

  // As long as the client consumes, the server will send messages
  stream Message listen(1: string sender);

  // These method will not be overiden, so the default implementation will be
  // used. If client calls these methods, it should not cause any crash and it
  // should end gracefully
  stream Message nonImplementedStream(1: string sender);

  stream Message returnNullptr();

  i32, stream Message throwError() throws (1: Error ex)

  i32, stream i32 leakCheck(1: i32 from, 2: i32 to);
  i32 instanceCount();

  i32, stream i32 sleepWithResponse(1: i32 timeMs);
  stream i32 sleepWithoutResponse(1: i32 timeMs);

  i32, stream i32 streamNever();
}
