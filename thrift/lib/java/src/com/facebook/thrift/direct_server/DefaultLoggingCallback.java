package com.facebook.thrift.direct_server;

import org.slf4j.Logger;


class DefaultLoggingCallback implements DirectServer.LoggingCallback {

  public void onAsyncDrop(Logger logger) {
    logger.info("Dropping a task");
  }

}
