/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <signal.h>
#include <pthread.h>

#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/concurrency/Util.h>
#include <thrift/lib/cpp/test/TimeUtil.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>
#include <thrift/lib/cpp/transport/TSSLServerSocket.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/transport/TRpcTransport.h>
#include <thrift/lib/cpp/TProcessor.h>

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <list>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace boost;

using std::string;
using std::vector;
using std::min;
using std::cerr;
using std::endl;
using std::list;
using apache::thrift::concurrency::Util;
using folly::SocketAddress;
using folly::SSLContext;
using apache::thrift::transport::TTransportException;
using apache::thrift::transport::TSSLSocket;
using apache::thrift::transport::TSSLServerSocket;
using apache::thrift::transport::TSSLException;
using apache::thrift::transport::TSSLSocketFactory;
using apache::thrift::transport::TRpcTransport;
using apache::thrift::server::TServer;
using apache::thrift::util::ScopedServerThread;
using apache::thrift::TProcessor;

class TestSSLServer: public TServer {
 private:
  bool stopped_;
  std::shared_ptr<SSLContext> ctx_;
  std::shared_ptr<TSSLServerSocket> socket_;

 public:
  // Create a TestSSLServer.
  // This immediately starts listening on the port 0.
  explicit TestSSLServer(SSLContext::SSLVersion version);

  void serve();

  void stop() {
    stopped_ = true;
    socket_->interrupt();
    cerr << "Waiting for server thread to exit" << endl;
  }
};

TestSSLServer::TestSSLServer(SSLContext::SSLVersion version) :
    TServer(std::shared_ptr<TProcessor>()),
    stopped_(false), ctx_(new SSLContext(version)) {
  // Set up the SSL context
  ctx_->loadCertificate("thrift/lib/cpp/test/ssl/tests-cert.pem");
  ctx_->loadPrivateKey("thrift/lib/cpp/test/ssl/tests-key.pem");
  ctx_->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  //set up the listening socket
  std::shared_ptr<TSSLSocketFactory> factory(new TSSLSocketFactory(ctx_));
  socket_.reset(new TSSLServerSocket(0, factory));
  socket_->listen();

  cerr << "Accepting connections on port 0" << endl;
}

void TestSSLServer::serve() {
  folly::SocketAddress addr;
  socket_->getAddress(&addr);
  getEventHandler()->preServe(&addr);

  while (!stopped_) {
    std::shared_ptr<TRpcTransport> sock = socket_->accept();
    TSSLSocket *sslSock = dynamic_cast<TSSLSocket*>(sock.get());
    BOOST_CHECK(sslSock);

    // read()
    uint8_t buf[128];
    uint32_t bytesRead = sslSock->readAll(buf, sizeof(buf));
    BOOST_CHECK_EQUAL(bytesRead, 128);

    // write()
    sslSock->write(buf, sizeof(buf));
  }

  cerr << "TestSSLServer::serve() terminates" << endl;
}

void testServerClient(SSLContext::SSLVersion serverVersion,
                      SSLContext::SSLVersion clientVersion) {
  // Set up the server thread.
  std::shared_ptr<TestSSLServer> server(new TestSSLServer(serverVersion));
  ScopedServerThread thread(server);

  // Set up SSL context for the client.
  std::shared_ptr<SSLContext> sslContext(new SSLContext(clientVersion));
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  // connect
  const folly::SocketAddress *serverAddr = thread.getAddress();
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(
                                  sslContext,
                                  serverAddr->getHostStr().c_str(),
                                  serverAddr->getPort()));
  socket->open();

  // write()
  uint8_t buf[128];
  memset(buf, 'a', sizeof(buf));
  socket->write(buf, sizeof(buf));

  // read()
  uint8_t readbuf[128];
  uint32_t bytesRead = socket->readAll(readbuf, sizeof(readbuf));
  BOOST_CHECK_EQUAL(bytesRead, 128);
  BOOST_CHECK_EQUAL(memcmp(buf, readbuf, bytesRead), 0);

  // close()
  socket->close();
}

/**
 * Test connecting to, writing to, reading from, and closing the
 * connection to the SSL server.
 */
BOOST_AUTO_TEST_CASE(ConnectWriteReadClose) {
  for (int serverVersion = SSLContext::SSLv2;
       serverVersion <= SSLContext::TLSv1; serverVersion++) {
    // Client version must be the same or higher than the server version.
    for (int clientVersion = serverVersion;
         clientVersion <= SSLContext::TLSv1; clientVersion++) {
      testServerClient((SSLContext::SSLVersion)serverVersion,
                       (SSLContext::SSLVersion)clientVersion);
    }
  }
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
    "TSSLContextTest";

  if (argc != 1) {
    cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      cerr << " " << argv[n];
    }
    cerr << endl;
    exit(1);
  }

  return nullptr;
}
