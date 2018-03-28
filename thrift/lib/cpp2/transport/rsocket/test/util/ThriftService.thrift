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

  // As new int values arrive, add them to the previous ones and return
  // 1, 2, 3, 4 -> 1, 3, 6, 10
  // 5 -> 15
  stream i32 prefixSumIOThread(stream i32 numbers);

  // Perform prefixSum operation on the Worker Thread
  stream i32 prefixSumWorkerThread(stream i32 numbers);

  // Echo the received messages, use Worker Thread
  // Flow Control:
  //  Client will request more messages, so server can provide
  //  Then server will request more messages from the client
  stream Message channel(stream Message messages, 1: string sender);

  // As long as the client consumes, the server will send messages
  stream Message listen(1: string sender);

  // These method will not be overiden, so the default implementation will be
  // used. If client calls these methods, it should not cause any crash and it
  // should end gracefully
  stream Message nonImplementedStream(1: string sender);
  stream Message nonImplementedChannel(
      stream Message messages, 1: string sender);

  stream Message returnNullptr();
  stream Message throwException(stream Message messages);

  i32, stream Message throwError() throws (1: Error ex)
}
