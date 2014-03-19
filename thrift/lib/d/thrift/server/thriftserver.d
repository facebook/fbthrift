import thrift.codegen.processor;
import thrift.protocol.processor;
import thrift.protocol.binary;
import thrift.transport.memory;
import thrift.server.base;
import std.stdio;
import core.thread;
import thrift.util.cancellation;

// Public D interface

class CppServer {
  this(TProcessor processor, ushort port) {
    server_ = thriftserver_new();
    thriftserver_setInterface(server_, new ThriftServerProcessor(processor));
    thriftserver_setPort(server_, port);
  }

  ~this() {
    thriftserver_free(server_);
  }

  void serve() {
    thriftserver_serve(server_);
  }
 private:
  ThriftServer* server_ = null;
}

// C++ Glue logic follows

// Opaque C++ pointers
struct ThriftServer;
struct TEventBase;
struct ThriftServerRequest;

// C++ interface.
extern (C) {

  void thriftserver_setPort(ThriftServer*, ushort);
  void thriftserver_serve(ThriftServer*);
  void thriftserver_setInterface(ThriftServer*, ThriftServerInterface);
  void thriftserver_sendReply(ThriftServerRequest* req,
                              TEventBase* eb,
                              const ubyte* data, size_t len);
  void thriftserver_freeRequest(ThriftServerRequest* req);

  ThriftServer* thriftserver_new();
  void thriftserver_free(ThriftServer*);
}

// Interface exposed to C++
extern (C++) interface ThriftServerInterface {
  void process(ThriftServerRequest* req, TEventBase* eb,
               ubyte* data, size_t len, ubyte protType);
}

// Globals to avoid new class every request
TMemoryBuffer buf = null;
TBinaryProtocol!TMemoryBuffer protocol = null;
TMemoryBuffer outbuf = null;
TBinaryProtocol!TMemoryBuffer outprot = null;

// See note below
bool attached = false;

class ThriftServerProcessor : ThriftServerInterface {
  this(TProcessor iface) {
    iface_ = iface;
  }

  // TODO: check protType
  extern (C++) void process(ThriftServerRequest* req,
                            TEventBase* eb,
                            ubyte* data, size_t len, ubyte protType) {
    try {
      // Needed to initialize D's GC on this thread, since it is
      // a C++ started thread.
      //
      // thread_attachThis() documentation says it is a noop
      // if called more than once, but perf says it actually
      // grabs a lock.  Checking if we've already called it ourselves
      // saves ~10% CPU
      if (!attached) {
        thread_attachThis();
        attached = true;
      }

      if (!outbuf) {
        // Initial alloc
        buf = new TMemoryBuffer(data, len);
        protocol = new TBinaryProtocol!TMemoryBuffer(buf);
        outbuf = new TMemoryBuffer();
        outprot = new TBinaryProtocol!TMemoryBuffer(outbuf);
      } else {
        buf.reset(data, len);
        outbuf.reset();
      }
      iface_.process(protocol, outprot);

      thriftserver_sendReply(
        req, eb, outbuf.getContents().ptr, outbuf.getContents().length);
    } catch (Exception ex) {
      writeln(ex);
      thriftserver_freeRequest(req);
    }
  }

  private:
  TProcessor iface_;
}
