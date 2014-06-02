import core.thread;
import thrift.codegen.client : tClient, TClientBase;
import thrift.codegen.processor;
import thrift.protocol.binary : tBinaryProtocol;
import thrift.transport.base;
import thrift.transport.socket : TSocket;
import thrift.transport.tsocketaddress;
import thrift.transport.framed : TFramedTransport;
import thrift.protocol.base;
import thrift.server.base;
import thrift.server.transport.socket;
import thrift.server.thriftserver;
import thrift.async.teventbase;
import thrift.async.teventbasemanager;
import TestServiceIf.TestService;

class TestServiceHandler : TestService {
  string getName() {
    return "foo";
  }
}

class TestServiceThriftTestFixture {
  public:
    TClientBase!TestService client;
    this() {
      auto processor =
        new TServiceProcessor!TestService(new TestServiceHandler);
      server_ = new CppServer(processor, 0);
      server_.setup();
      auto ebm = server_.getEventBaseManager();
      auto server_eb = ebm.getEventBase();
      serverThread_ = new Thread({
        server_eb.loop();
      });
      auto tsockAddr = server_.getAddress();
      auto port = tsockAddr.getPort();
      socket_ = new TSocket("localhost", port);
      socket_.recvTimeout(dur!"msecs"(500));
      socket_.sendTimeout(dur!"msecs"(500));
      socket_.open();
      serverThread_.isDaemon = true;
      serverThread_.start();
      auto trans = new TFramedTransport(socket_);
      auto protocol = tBinaryProtocol(trans);
      client = tClient!TestService(protocol);
    }

    void shutdown() {
      socket_.close();      // close the client
      server_.stop();       // stop the server's event loop
      serverThread_.join(); // ensure the thread completes
    }

    ~this() {
      server_.cleanUp();
    }

  private:
    CppServer server_;
    Thread serverThread_;
    TSocket socket_;
}

unittest {
  auto x = new TestServiceThriftTestFixture();
  scope(exit) x.shutdown();
  assert(x.client.getName() == "foo");
}

void main() {
}
