/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * Implement automatic fb303 counters for java thrift service.
 */

package com.facebook.thrift.direct_server;

import java.util.HashMap;
import com.facebook.thrift.TBase;
import com.facebook.thrift.TProcessorEventHandler;
import com.facebook.thrift.TException;
import com.facebook.thrift.server.TConnectionContext;
import com.facebook.fbcode.fb303.FacebookBase;


public class DirectTProcessorEventHandler
  extends TProcessorEventHandler {

  protected FacebookBase handler_;


  static private class TStat {
    MovingAvgStat bytesRead_;
    MovingAvgStat bytesWritten_;
    MovingAvgStat numCalls_;
    MovingAvgStat timeProcess_;

    TStat(FacebookBase h, String fn_name) {
      numCalls_ = new MovingAvgStat(h, fn_name + ".num_calls", null);
      bytesRead_ = new MovingAvgStat(h, fn_name + ".bytes_read", numCalls_);

      bytesWritten_ =
          new MovingAvgStat(h, fn_name + ".bytes_written", numCalls_);

      timeProcess_ =
          new MovingAvgStat(h, fn_name + ".time_process_us", numCalls_);
    }
  };


  static HashMap<String, TStat> theMap_ = new HashMap<String, TStat>();


  static private class Stats {
    long begin_;
    long bytesRead_;
    TStat stats_;
    TConnectionContext cxt_;

    Stats(TStat s, TConnectionContext c) {
      begin_ = 0;
      bytesRead_ = 0;
      stats_ = s;
      cxt_ = c;
    }
  };


  public DirectTProcessorEventHandler(FacebookBase h) {
    handler_ = h;
  }

  @Override public Object getContext(String fn_name, TConnectionContext context) {
    synchronized(theMap_) {
      TStat ss = theMap_.get(fn_name);
      if (ss == null) {
        ss = new TStat(handler_, fn_name);
        theMap_.put(fn_name, ss);
      }

      return (Object)new Stats(ss, context);
    }
  }

  @Override public void preRead(Object handler_context, String fn_name)
    throws TException {

    Stats stats = (Stats)handler_context;
    if (stats.cxt_ != null) {
      TDirectBufferProtocol p =
          (TDirectBufferProtocol)stats.cxt_.getInputProtocol();
      stats.bytesRead_ = p.inputSize();
    }

    stats.begin_ = System.nanoTime();
  }

  @Override public void postWrite(Object handler_context, String fn_name,
      TBase result) throws TException {

    Stats stats = (Stats)handler_context;
    long bytesWritten = 0;
    long end = System.nanoTime();
    long latency = (end - stats.begin_) / 1000;

    if (stats.cxt_ != null) {
      TDirectBufferProtocol p =
          (TDirectBufferProtocol)stats.cxt_.getInputProtocol();
      bytesWritten = p.outputSize();
    }

    TStat s = stats.stats_;

    s.bytesRead_.addValue(stats.bytesRead_);
    s.bytesWritten_.addValue(bytesWritten);
    s.numCalls_.addValue(1);
    s.timeProcess_.addValue(latency);

  }

  @Override public void handlerError(Object handler_context, String fn_name,
      Throwable ex) throws TException {
    handler_.incrementCounter(fn_name + ".num_exceptions.sum", 1);
  }
};
