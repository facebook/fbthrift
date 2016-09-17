package com.facebook.thrift.direct_server;

import static org.junit.Assert.*;

import com.facebook.thrift.direct_server.TDirectServer;
import com.facebook.thrift.protocol.TBinaryProtocol;
import com.facebook.thrift.transport.TFramedTransport;
import com.facebook.thrift.transport.TSocket;
import com.facebook.thrift.transport.TTransportException;
import java.util.concurrent.CountDownLatch;
import org.junit.BeforeClass;
import org.junit.Test;

import com.facebook.fbcode.fb303.fb_status;

public class JUnitTestHsHaServer {

  private static final String HOST = "localhost";
  private static int PORT;
  private static final int NUM_THREADS = 1;
  private static final int MAX_PENDING = 1;

  private static JavaSimpleService.Client newClient() throws Exception {
    TFramedTransport trans = new TFramedTransport(new TSocket(HOST, PORT));
    trans.open();
    return new JavaSimpleService.Client(new TBinaryProtocol(trans));
  }

  private static class PortGetter implements Runnable {
    private CountDownLatch latch_;
    private TDirectServer server_;
    private int port_;

    public PortGetter(CountDownLatch latch) {
      latch_ = latch;
    }

    public void setServer(TDirectServer server) {
      server_ = server;
    }

    public int getPort() {
      return port_;
    }

    @Override
    public void run() {
      port_ = server_.directServer().getLocalPort();
      latch_.countDown();
    }
  }

  @BeforeClass
  public static void setUpBeforeClass() throws Exception {
    JavaSimpleService.Processor proc =
        new JavaSimpleService.Processor(new SimpleServiceHandler());
    CountDownLatch latch = new CountDownLatch(1);
    PortGetter portGetter = new PortGetter(latch);
    final TDirectServer server =
        TDirectServer.asHsHaServer(0, NUM_THREADS, MAX_PENDING, proc, portGetter);
    portGetter.setServer(server);
    new Thread() {
      @Override
      public void run() {
        server.serve();
      }
    }.start();
    latch.await();
    PORT = portGetter.getPort();
    final int NUM_ITERS = 20, MS_PER_ITER = 100;
    for (int i = 0; i < NUM_ITERS; i++) {
      try {
        if (newClient().getStatus() == fb_status.ALIVE) {
          return;
        }
      } catch (TTransportException e) {
        if (i == NUM_ITERS - 1) {
          throw e;
        }
      }
      Thread.sleep(MS_PER_ITER);
    }
  }

  @Test
  public void simple() throws Exception {
    assertEquals(fb_status.ALIVE, newClient().getStatus());
    assertEquals("", newClient().getString(0));
    assertEquals("xxxxx", newClient().getString(5));
  }

  @Test
  public void largeResponse() throws Exception {
    final int LENGTH = 1024 * 1024;
    assertEquals(LENGTH, newClient().getString(LENGTH).length());
  }
}
