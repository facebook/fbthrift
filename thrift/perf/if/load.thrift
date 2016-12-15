include "common/fb303/if/fb303.thrift"

namespace cpp apache.thrift.test
namespace cpp2 apache.thrift
namespace py apache.thrift.test.load
namespace py3 py3.apache.thrift.test.load
namespace py.asyncio apache.thrift.test.asyncio.load
namespace py.twisted apache.thrift.test.twisted.load
namespace java com.facebook.thrift.perf
namespace java.swift org.apache.swift.thrift.perf

exception LoadError {
  1: i32 code
}

service LoadTest extends fb303.FacebookService {
  // Methods for testing server performance
  // Fairly simple requests, to minimize serialization overhead

  /**
   * noop() returns immediately, to test behavior of fast, cheap operations
   */
  void noop()
  oneway void onewayNoop()

  /**
   * asyncNoop() is like noop() except for one minor difference in the async
   * implementation.
   *
   * In the async handler, noop() invokes the callback immediately, while
   * asyncNoop() uses runInLoop() to invoke the callback.
   */
  void asyncNoop()

  /**
   * sleep() waits for the specified time period before returning,
   * to test slow, but not CPU-intensive operations
   */
  void sleep(1: i64 microseconds)
  oneway void onewaySleep(1: i64 microseconds)

  /**
   * burn() uses as much CPU as possible for the desired time period,
   * to test CPU-intensive operations
   */
  void burn(1: i64 microseconds)
  oneway void onewayBurn(1: i64 microseconds)

  /**
   * badSleep() is like sleep(), except that it exhibits bad behavior in the
   * async implementation.
   *
   * Instead of returning control to the event loop, it actually sleeps in the
   * worker thread for the specified duration.  This tests how well the thrift
   * infrastructure responds to badly written handler code.
   */
  void badSleep(1: i64 microseconds) (thread="eb")

  /**
   * badBurn() is like burn(), except that it exhibits bad behavior in the
   * async implementation.
   *
   * The normal burn() call periodically yields to the event loop, while
   * badBurn() does not.  This tests how well the thrift infrastructure
   * responds to badly written handler code.
   */
  void badBurn(1: i64 microseconds) (thread="eb")

  /**
   * throw an error
   */
  void throwError(1: i32 code) throws (1: LoadError error)

  /**
   * throw an unexpected error (not declared in the .thrift file)
   */
  void throwUnexpected(1: i32 code)

  /**
   * throw an error in a oneway call,
   * just to make sure the internal thrift code handles it properly
   */
  oneway void onewayThrow(1: i32 code)

  /**
   * Send some data to the server.
   *
   * The data is ignored.  This is primarily to test the server I/O
   * performance.
   */
  void send(1: binary data)

  /**
   * Send some data to the server.
   *
   * The data is ignored.  This is primarily to test the server I/O
   * performance.
   */
  oneway void onewaySend(1: binary data)

  /**
   * Receive some data from the server.
   *
   * The contents of the data are undefined.  This is primarily to test the
   * server I/O performance.
   */
  binary recv(1: i64 bytes)

  /**
   * Send and receive data
   */
  binary sendrecv(1: binary data, 2: i64 recvBytes)

  /**
   * Echo data back to the client.
   */
  binary echo(1: binary data)

  /**
   * Add the two integers
   */
  i64 add(1: i64 a, 2: i64 b)
}
