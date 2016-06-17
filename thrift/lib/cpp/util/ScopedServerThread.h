/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef THRIFT_UTIL_SCOPEDSERVERTHREAD_H_
#define THRIFT_UTIL_SCOPEDSERVERTHREAD_H_ 1

#include <boost/noncopyable.hpp>
#include <memory>

namespace folly {
class SocketAddress;
}

namespace apache { namespace thrift {

namespace concurrency {
class Thread;
}
namespace server {
class TServer;
}
namespace util {

class ServerCreator;

/**
 * ScopedServerThread spawns a thrift server in a new thread.
 *
 * The server is stopped automatically when the ScopedServerThread is
 * destroyed.
 */
class ScopedServerThread : public boost::noncopyable {
 public:
  /**
   * Create a new, unstarted ScopedServerThread object.
   */
  ScopedServerThread();

  /**
   * Create a ScopedServerThread object and automatically start it.
   */
  explicit ScopedServerThread(ServerCreator* serverCreator);

  /**
   * Create a ScopedServerThread object and automatically start it.
   */
  explicit ScopedServerThread(const std::shared_ptr<server::TServer>& server);

  virtual ~ScopedServerThread();

  /**
   * Start the server thread.
   *
   * This method does not return until the server has successfully started.
   *
   * @param serverCreator The ServerCreator object to use to create the server.
   */
  void start(ServerCreator* serverCreator);

  /**
   * Start the server thread.
   *
   * This method does not return until the server has successfully started.
   *
   * @param server The server to run in the new thread.
   */
  void start(const std::shared_ptr<server::TServer>& server);

  /**
   * Stop the server thread.
   */
  void stop();

  /**
   * Waits for the server thread to finish.
   * Note that this doesn't stop the thread.
   */
  void join();

  /**
   * Get the address on which the server is listening.
   */
  const folly::SocketAddress* getAddress() const;

  /**
   * Get the server.
   */
  std::weak_ptr<server::TServer> getServer() const;

 private:
  class Helper;
  friend class ServerStartHelper;

  std::shared_ptr<Helper> helper_;
  std::shared_ptr<concurrency::Thread> thread_;
};

/*
 * Encapsulates the synchronous start functionality of ScopedServerThread.
 * Used to wait for server to get up in those case when server is started
 * by a method other than ScopedServerThread.
 */
class ServerStartHelper {
 public:
  /*
   * Constructor. Should be called prior to starting the server
   */
  explicit ServerStartHelper(const std::shared_ptr<server::TServer>& server);

  /**
   * Wait until the server has started.
   *
   * Raises TException if the server failed to start.
   */
  void waitUntilStarted();

 private:
  std::shared_ptr<ScopedServerThread::Helper> helper_;
};

}}} // apache::thrift::util

#endif // THRIFT_UTIL_SCOPEDSERVERTHREAD_H_
