import std.conv, std.getopt, std.stdio;
import thrift.codegen.processor;
import thrift.server.thriftserver;
import LoadHandler;

void main(string[] args)
{
  string serviceName = "";
  string port = "1234";
  string num_threads = "0";
  string num_io_threads = "1";

  try {
    getopt(
      args,
      "name", &serviceName,
      "port", &port,
      "num_threads", &num_threads,
      "num_io_threads", &num_io_threads);
  } catch(Exception e) {
    writeln(
      "Usage: ThriftServer --name=N --port=P --num_threads=N --num_io_threads=N");
    return;
  }

  auto processor = new TServiceProcessor!LoadTest(new LoadHandler());
  auto server = new CppServer(processor, to!ushort(port));

  server.serve();
}
