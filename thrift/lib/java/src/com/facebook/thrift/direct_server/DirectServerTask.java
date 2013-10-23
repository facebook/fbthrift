

package com.facebook.thrift.direct_server;

import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ThreadPoolExecutor;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class DirectServerTask implements Runnable {

  private static final Logger LOG =
      LoggerFactory.getLogger(DirectServerTask.class);

  private final SelectorThread selectorThread_;
  private final ChannelHandler handler_;


  DirectServerTask(SelectorThread s, ChannelHandler h) {
    selectorThread_ = s;
    handler_ = h;
    handler_.wouldUseThreadPool(true);
  }

  ChannelHandler handler() {
    return handler_;
  }

  // Submit the task to a thread pool, return false if the task can not
  // be submitted.
  void executingBy(ThreadPoolExecutor executorService) {
    try {
      executorService.execute(this);
    } catch (RejectedExecutionException e) {
      drop();
    } catch (Exception e) {
      LOG.warn("Fails to submit task", e);
    }
  }

  @Override public void run() {
    handler_.transition(selectorThread_);
    handler_.wouldUseThreadPool(false);
    selectorThread_.notifyExecutorServiceTaskComplete(this);
  }

  /**
   * If the queue is too long, we have to remove older tasks from it.
   * The task we selected may not belong to the selector thread.
   * We use this to wakeup the corresponding selector thread to do cleanup.
   */
  public void drop() {
    handler_.markError();
    handler_.wouldUseThreadPool(false);
    selectorThread_.getLoggingCallback().onAsyncDrop(LOG);
    selectorThread_.notifyExecutorServiceTaskComplete(this);
  }
};
