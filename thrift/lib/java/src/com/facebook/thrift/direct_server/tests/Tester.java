package com.facebook.thrift.direct_server.tests;


/*
 * A testing tool for TDirectServer
 *
 *  @author Wei Chen (weichen@fb.com)
 */

import com.facebook.thrift.protocol.TBinaryProtocol;
import com.facebook.thrift.transport.TSocket;
import com.facebook.thrift.transport.TFramedTransport;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.ArrayList;


public class Tester {

  static private Logger LOG =
      LoggerFactory.getLogger(Tester.class);

  static private String host_ = "localhost";

  static private int port_ = 3010;

  static boolean bigPayloadTest_ = true;


  static JavaSimpleService.Client getClient(String host, int port) {
    String msg = null;

    for (int i = 0; i < 3; ++i) {
      try {
        TSocket sock = new TSocket(host, port, 100, 500);
        sock.open();
        TFramedTransport trans = new TFramedTransport(sock);
        TBinaryProtocol proto = new TBinaryProtocol(trans);
        JavaSimpleService.Client cli = new JavaSimpleService.Client(proto);
        return cli;
      } catch (Exception e) {
        msg = e.toString();
        try {
          Thread.sleep(100);
        } catch (Exception ee) {
        }
      }
    }

    LOG.info("Fails to create client " + msg);
    return null;
  }

  static void singleTest() {
    long beg = System.nanoTime();

    for (int i = 0; i < 1000; ++i) {
      try {
        JavaSimpleService.Client cli = getClient(host_, port_);
        cli.getStatus();
        long end = System.nanoTime();
        LOG.info("getStatus takes " + Long.toString((end - beg) / 1000) + "us");
        beg = end;
      } catch (Exception e) {
        LOG.warn("Fails to do RPC " + e);
        e.printStackTrace();
      }
    }
  }


  static class MultiTestThread extends Thread {
    static public int numTests_ = 500000;

    public long beg_ = 0;
    public long end_ = 0;
    public int success_ = 0;

    private String prepareValue() {
      if (!Tester.bigPayloadTest_) {
        return "";
      }

      char[] arr = new char[15*1024];
      for (int i = 0; i < arr.length; ++i) {
        arr[i] = 'A';
      }

      return new String(arr);
    }


    public void run() {
      SimpleRequest r = new SimpleRequest();
      r.value = prepareValue();
      beg_ = System.nanoTime();

      for (int i = 0; i < numTests_; ++i) {
        JavaSimpleService.Client cli = getClient(host_, port_);
        try {
          String s = cli.simple(r);
          if (Tester.bigPayloadTest_) {
            if (s.length() > 15*1024 && s.charAt(15*1024) == 'A') {
              ++success_;
            }
          } else {
            ++success_;
          }
          cli.getInputProtocol().getTransport().close();
        } catch (Exception e) {
          cli.getInputProtocol().getTransport().close();
          cli = getClient(host_, port_);
        }
      }

      end_ = System.nanoTime();
    }
  };

  public static void multiTest() {
    List<MultiTestThread> li = new ArrayList<MultiTestThread>();
    int numThreads = 12;
    for (int i = 0; i < numThreads; ++i) {
      MultiTestThread t = new MultiTestThread();
      t.start();
      li.add(t);
    }

    long beg = 0;
    long end = 0;

    int success = 0;

    for (MultiTestThread t : li) {
      try {
        t.join();
      } catch (Exception e) {
        LOG.warn("Join err " + e);
      }

      if (beg == 0 || beg > t.beg_) {
        beg = t.beg_;
      }
      if (end < t.end_) {
        end = t.end_;
      }

      success += t.success_;
    }

    long duration = (end - beg) / 1000000; // ms
    long multiple = 1000;

    LOG.info("Total success is " + Integer.toString(success));

    if (success > 0) {
      LOG.info("QPS is " + Long.toString(success * multiple/ duration));
      LOG.info("Latency is " + Long.toString(duration * 1000 / success));
    }
  }

  public static void main(String[] args) {
    multiTest();
  }
};
