// Copyright 2011-present Facebook. All Rights Reserved.

package com.facebook.swift.scalaexample.server

import com.facebook.swift.service._
import com.facebook.swift.codec._

@ThriftService
class PingService {
  @ThriftMethod
  def ping = "pong"
}

object server {
  def main(args: Array[String]) {
    val config = new ThriftServerConfig().setWorkerThreads(200).setPort(4567)
    val handler = new PingService()
    val codecManager = new ThriftCodecManager()
    val processor = new ThriftServiceProcessor(codecManager, handler)
    val server = new ThriftServer(processor, config)

    server.start()
  }
}
