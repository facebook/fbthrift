namespace py.asyncio thrift_asyncio.sleep

struct OverflowResult {
  // A member that we can use to test data overflow
  byte value
}

service Sleep {
  string echo(string message, double delay);
  OverflowResult overflow(i32 value);
}
