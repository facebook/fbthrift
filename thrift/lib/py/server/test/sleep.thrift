namespace py.asyncio thrift_asyncio.sleep

service Sleep {
  string echo(string message, double delay);
}
