import std.conv, std.getopt, std.stdio, core.time;
import thrift.server.nonblocking;
import thrift.protocol.binary;
import thrift.transport.buffered;
import thrift.codegen.processor;
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
      "Usage: Nonblocking --name=N --port=P --num_threads=N --num_io_threads=N");
    return;
  }

  auto protocolFactory = new TBinaryProtocolFactory!();
  auto processor = new TServiceProcessor!LoadTest(new LoadHandler());
  auto transportFactory = new TBufferedTransportFactory;
  TaskPool taskPool;
  if (to!ushort(num_threads) > 0) {
    taskPool = new TaskPool(to!ushort(num_threads));
  }

  auto server = new TNonblockingServer(
    processor, to!ushort(port), transportFactory, protocolFactory, taskPool);
  server.numIOThreads(to!ushort(num_io_threads));
  server.serve();

}
