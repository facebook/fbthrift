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

#include "thrift/lib/cpp/test/TAsyncSSLSocketTest.h"

#include <signal.h>
#include <pthread.h>

#include "thrift/lib/cpp/async/TAsyncSSLServerSocket.h"
#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/test/TimeUtil.h"
#include "thrift/lib/cpp/transport/TSSLSocket.h"
#include "thrift/lib/cpp/transport/TSocketAddress.h"

#include <boost/test/unit_test.hpp>
#include <iostream>
#include <list>
#include <set>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

using namespace boost;

using std::string;
using std::vector;
using std::min;
using std::cerr;
using std::endl;
using std::list;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::async::TAsyncSSLServerSocket;
using apache::thrift::async::TAsyncTransport;
using apache::thrift::async::TEventBase;
using apache::thrift::concurrency::Util;
using apache::thrift::transport::TSocketAddress;
using apache::thrift::transport::TTransportException;
using apache::thrift::transport::SSLContext;
using apache::thrift::transport::TSSLSocket;
using apache::thrift::transport::TSSLException;

uint32_t TestSSLAsyncCacheServer::asyncCallbacks_ = 0;
uint32_t TestSSLAsyncCacheServer::asyncLookups_ = 0;
uint32_t TestSSLAsyncCacheServer::lookupDelay_ = 0;

/**
 * Test connecting to, writing to, reading from, and closing the
 * connection to the SSL server.
 */
BOOST_AUTO_TEST_CASE(ConnectWriteReadClose) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallback acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL context.
  std::shared_ptr<SSLContext> sslContext(new SSLContext());
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");
  //sslContext->loadTrustedCertificates("./trusted-ca-certificate.pem");
  //sslContext->authenticate(true, false);

  // connect
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(sslContext,
                                               server.getAddress()));
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

  cerr << "ConnectWriteReadClose test completed" << endl;
}

/**
 * Negative test for handshakeError().
 */
BOOST_AUTO_TEST_CASE(HandshakeError) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  HandshakeErrorCallback acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL context.
  std::shared_ptr<SSLContext> sslContext(new SSLContext());
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  // connect
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(sslContext,
                                               server.getAddress()));
  socket->open();

  // read()
  bool ex = false;
  try {
    uint8_t readbuf[128];
    uint32_t bytesRead = socket->readAll(readbuf, sizeof(readbuf));
  } catch (TSSLException &e) {
    ex = true;
  }
  BOOST_CHECK(ex);

  // close()
  socket->close();
  cerr << "HandshakeError test completed" << endl;
}

/**
 * Negative test for readError().
 */
BOOST_AUTO_TEST_CASE(ReadError) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadErrorCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallback acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL context.
  std::shared_ptr<SSLContext> sslContext(new SSLContext());
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  // connect
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(sslContext,
                                               server.getAddress()));
  socket->open();

  // write something to trigger ssl handshake
  uint8_t buf[128];
  memset(buf, 'a', sizeof(buf));
  socket->write(buf, sizeof(buf));

  socket->close();
  cerr << "ReadError test completed" << endl;
}

/**
 * Negative test for writeError().
 */
BOOST_AUTO_TEST_CASE(WriteError) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  WriteErrorCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallback acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL context.
  std::shared_ptr<SSLContext> sslContext(new SSLContext());
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  // connect
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(sslContext,
                                               server.getAddress()));
  socket->open();

  // write something to trigger ssl handshake
  uint8_t buf[128];
  memset(buf, 'a', sizeof(buf));
  socket->write(buf, sizeof(buf));

  socket->close();
  cerr << "WriteError test completed" << endl;
}

/**
 * Test a socket with TCP_NODELAY unset.
 */
BOOST_AUTO_TEST_CASE(SocketWithDelay) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallbackDelay acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL context.
  std::shared_ptr<SSLContext> sslContext(new SSLContext());
  sslContext->ciphers("ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH");

  // connect
  std::shared_ptr<TSSLSocket> socket(new TSSLSocket(sslContext,
                                               server.getAddress()));
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

  cerr << "SocketWithDelay test completed" << endl;
}

BOOST_AUTO_TEST_CASE(NpnTestOverlap) {
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> serverCtx(new SSLContext);;
  int fds[2];
  getfds(fds);
  getctx(clientCtx, serverCtx);

  clientCtx->setAdvertisedNextProtocols({"blub","baz"});
  serverCtx->setAdvertisedNextProtocols({"foo","bar","baz"});

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(serverCtx, &eventBase, fds[1], true));
  NpnClient client(std::move(clientSock));
  NpnServer server(std::move(serverSock));

  eventBase.loop();

  BOOST_CHECK(client.nextProtoLength != 0);
  BOOST_CHECK_EQUAL(client.nextProtoLength, server.nextProtoLength);
  BOOST_CHECK_EQUAL(memcmp(client.nextProto, server.nextProto,
                           server.nextProtoLength), 0);
  string selected((const char*)client.nextProto, client.nextProtoLength);
  BOOST_CHECK_EQUAL(selected.compare("baz"), 0);
}

BOOST_AUTO_TEST_CASE(NpnTestUnset) {
  // Identical to above test, except that we want unset NPN before
  // looping.
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> serverCtx(new SSLContext);;
  int fds[2];
  getfds(fds);
  getctx(clientCtx, serverCtx);

  clientCtx->setAdvertisedNextProtocols({"blub","baz"});
  serverCtx->setAdvertisedNextProtocols({"foo","bar","baz"});

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(serverCtx, &eventBase, fds[1], true));

  // unsetting NPN for any of [client, server] is enought to make NPN not
  // work
  clientCtx->unsetNextProtocols();

  NpnClient client(std::move(clientSock));
  NpnServer server(std::move(serverSock));

  eventBase.loop();

  BOOST_CHECK(client.nextProtoLength == 0);
  BOOST_CHECK(server.nextProtoLength == 0);
  BOOST_CHECK(client.nextProto == nullptr);
  BOOST_CHECK(server.nextProto == nullptr);
}

BOOST_AUTO_TEST_CASE(NpnTestNoOverlap) {
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> serverCtx(new SSLContext);;
  int fds[2];
  getfds(fds);
  getctx(clientCtx, serverCtx);

  clientCtx->setAdvertisedNextProtocols({"blub"});
  serverCtx->setAdvertisedNextProtocols({"foo","bar","baz"});

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(serverCtx, &eventBase, fds[1], true));
  NpnClient client(std::move(clientSock));
  NpnServer server(std::move(serverSock));

  eventBase.loop();

  BOOST_CHECK(client.nextProtoLength != 0);
  BOOST_CHECK_EQUAL(client.nextProtoLength, server.nextProtoLength);
  BOOST_CHECK_EQUAL(memcmp(client.nextProto, server.nextProto,
                           server.nextProtoLength), 0);
  string selected((const char*)client.nextProto, client.nextProtoLength);
  BOOST_CHECK_EQUAL(selected.compare("blub"), 0);
}

BOOST_AUTO_TEST_CASE(RandomizedNpnTest) {
  // Probability that this test will fail is 2^-64, which could be considered
  // as negligible.
  const int kTries = 64;

  std::set<string> selectedProtocols;
  for (int i = 0; i < kTries; ++i) {
    TEventBase eventBase;
    std::shared_ptr<SSLContext> clientCtx = std::make_shared<SSLContext>();
    std::shared_ptr<SSLContext> serverCtx = std::make_shared<SSLContext>();
    int fds[2];
    getfds(fds);
    getctx(clientCtx, serverCtx);

    clientCtx->setAdvertisedNextProtocols({"foo", "bar", "baz"});
    serverCtx->setRandomizedAdvertisedNextProtocols({{1, {"foo"}},
        {1, {"bar"}}});


    TAsyncSSLSocket::UniquePtr clientSock(
      new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
    TAsyncSSLSocket::UniquePtr serverSock(
      new TAsyncSSLSocket(serverCtx, &eventBase, fds[1], true));
    NpnClient client(std::move(clientSock));
    NpnServer server(std::move(serverSock));

    eventBase.loop();

    BOOST_CHECK(client.nextProtoLength != 0);
    BOOST_CHECK_EQUAL(client.nextProtoLength, server.nextProtoLength);
    BOOST_CHECK_EQUAL(memcmp(client.nextProto, server.nextProto,
                             server.nextProtoLength), 0);
    string selected((const char*)client.nextProto, client.nextProtoLength);
    selectedProtocols.insert(selected);
  }
  BOOST_CHECK_EQUAL(selectedProtocols.size(), 2);
}


#ifndef OPENSSL_NO_TLSEXT
/**
 * 1. Client sends TLSEXT_HOSTNAME in client hello.
 * 2. Server found a match SSL_CTX and use this SSL_CTX to
 *    continue the SSL handshake.
 * 3. Server sends back TLSEXT_HOSTNAME in server hello.
 */
BOOST_AUTO_TEST_CASE(SNITestMatch) {
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> dfServerCtx(new SSLContext);
  // Use the same SSLContext to continue the handshake after
  // tlsext_hostname match.
  std::shared_ptr<SSLContext> hskServerCtx(dfServerCtx);
  const std::string serverName("xyz.newdev.facebook.com");
  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], serverName));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));
  SNIClient client(std::move(clientSock));
  SNIServer server(std::move(serverSock),
                   dfServerCtx,
                   hskServerCtx,
                   serverName);

  eventBase.loop();

  BOOST_CHECK(client.serverNameMatch);
  BOOST_CHECK(server.serverNameMatch);
}

/**
 * 1. Client sends TLSEXT_HOSTNAME in client hello.
 * 2. Server cannot find a matching SSL_CTX and continue to use
 *    the current SSL_CTX to do the handshake.
 * 3. Server does not send back TLSEXT_HOSTNAME in server hello.
 */
BOOST_AUTO_TEST_CASE(SNITestNotMatch) {
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> dfServerCtx(new SSLContext);
  // Use the same SSLContext to continue the handshake after
  // tlsext_hostname match.
  std::shared_ptr<SSLContext> hskServerCtx(dfServerCtx);
  const std::string clientRequestingServerName("foo.com");
  const std::string serverExpectedServerName("xyz.newdev.facebook.com");

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx,
                        &eventBase,
                        fds[0],
                        clientRequestingServerName));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));
  SNIClient client(std::move(clientSock));
  SNIServer server(std::move(serverSock),
                   dfServerCtx,
                   hskServerCtx,
                   serverExpectedServerName);

  eventBase.loop();

  BOOST_CHECK(!client.serverNameMatch);
  BOOST_CHECK(!server.serverNameMatch);
}

/**
 * 1. Client does not send TLSEXT_HOSTNAME in client hello.
 * 2. Server does not send back TLSEXT_HOSTNAME in server hello.
 */
BOOST_AUTO_TEST_CASE(SNITestClientHelloNoHostname) {
  TEventBase eventBase;
  std::shared_ptr<SSLContext> clientCtx(new SSLContext);
  std::shared_ptr<SSLContext> dfServerCtx(new SSLContext);
  // Use the same SSLContext to continue the handshake after
  // tlsext_hostname match.
  std::shared_ptr<SSLContext> hskServerCtx(dfServerCtx);
  const std::string serverExpectedServerName("xyz.newdev.facebook.com");

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));
  SNIClient client(std::move(clientSock));
  SNIServer server(std::move(serverSock),
                   dfServerCtx,
                   hskServerCtx,
                   serverExpectedServerName);

  eventBase.loop();

  BOOST_CHECK(!client.serverNameMatch);
  BOOST_CHECK(!server.serverNameMatch);
}

#endif
/**
 * Test SSL client socket
 */
BOOST_AUTO_TEST_CASE(SSLClientTest) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallbackDelay acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             1));

  client->connect();
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  BOOST_CHECK_EQUAL(client->getMiss(), 1);
  BOOST_CHECK_EQUAL(client->getHit(), 0);

  cerr << "SSLClientTest test completed" << endl;
}


/**
 * Test SSL client socket session re-use
 */
BOOST_AUTO_TEST_CASE(SSLClientTestReuse) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallbackDelay acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             10));

  client->connect();
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  BOOST_CHECK_EQUAL(client->getMiss(), 1);
  BOOST_CHECK_EQUAL(client->getHit(), 9);

  cerr << "SSLClientTestReuse test completed" << endl;
}

/**
 * Test SSL client socket timeout
 */
BOOST_AUTO_TEST_CASE(SSLClientTimeoutTest) {
  // Start listening on a local port
  EmptyReadCallback readCallback;
  HandshakeCallback handshakeCallback(&readCallback,
                                      HandshakeCallback::EXPECT_ERROR);
  HandshakeTimeoutCallback acceptCallback(&handshakeCallback);
  TestSSLServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             1, 10));
  client->connect(true /* write before connect completes */);
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  usleep(100000);
  // This is checking that the connectError callback precedes any queued
  // writeError callbacks.  This matches TAsyncSocket's behavior
  BOOST_CHECK_EQUAL(client->getWriteAfterConnectErrors(), 1);
  BOOST_CHECK_EQUAL(client->getErrors(), 1);
  BOOST_CHECK_EQUAL(client->getMiss(), 0);
  BOOST_CHECK_EQUAL(client->getHit(), 0);

  cerr << "SSLClientTimeoutTest test completed" << endl;
}


/**
 * Test SSL server async cache
 */
BOOST_AUTO_TEST_CASE(SSLServerAsyncCacheTest) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAsyncCacheAcceptCallback acceptCallback(&handshakeCallback);
  TestSSLAsyncCacheServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             10, 500));

  client->connect();
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  BOOST_CHECK_EQUAL(server.getAsyncCallbacks(), 18);
  BOOST_CHECK_EQUAL(server.getAsyncLookups(), 9);
  BOOST_CHECK_EQUAL(client->getMiss(), 10);
  BOOST_CHECK_EQUAL(client->getHit(), 0);

  cerr << "SSLServerAsyncCacheTest test completed" << endl;
}


/**
 * Test SSL server accept timeout with cache path
 */
BOOST_AUTO_TEST_CASE(SSLServerTimeoutTest) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  EmptyReadCallback clientReadCallback;
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAcceptCallback acceptCallback(&handshakeCallback, 50);
  TestSSLAsyncCacheServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  // only do a TCP connect
  std::shared_ptr<TAsyncSocket> sock = TAsyncSocket::newSocket(&eventBase);
  sock->connect(nullptr, server.getAddress());
  clientReadCallback.tcpSocket_ = sock;
  sock->setReadCallback(&clientReadCallback);

  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  BOOST_CHECK_EQUAL(readCallback.state, STATE_WAITING);

  cerr << "SSLServerTimeoutTest test completed" << endl;
}

/**
 * Test SSL server accept timeout with cache path
 */
BOOST_AUTO_TEST_CASE(SSLServerAsyncCacheTimeoutTest) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback);
  SSLServerAsyncCacheAcceptCallback acceptCallback(&handshakeCallback, 50);
  TestSSLAsyncCacheServer server(&acceptCallback);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             2));

  client->connect();
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  BOOST_CHECK_EQUAL(server.getAsyncCallbacks(), 1);
  BOOST_CHECK_EQUAL(server.getAsyncLookups(), 1);
  BOOST_CHECK_EQUAL(client->getErrors(), 1);
  BOOST_CHECK_EQUAL(client->getMiss(), 1);
  BOOST_CHECK_EQUAL(client->getHit(), 0);

  cerr << "SSLServerAsyncCacheTimeoutTest test completed" << endl;
}

/**
 * Test SSL server accept timeout with cache path
 */
BOOST_AUTO_TEST_CASE(SSLServerCacheCloseTest) {
  // Start listening on a local port
  WriteCallbackBase writeCallback;
  ReadCallback readCallback(&writeCallback);
  HandshakeCallback handshakeCallback(&readCallback,
                                      HandshakeCallback::EXPECT_ERROR);
  SSLServerAsyncCacheAcceptCallback acceptCallback(&handshakeCallback);
  TestSSLAsyncCacheServer server(&acceptCallback, 500);

  // Set up SSL client
  TEventBase eventBase;
  std::shared_ptr<SSLClient> client(new SSLClient(&eventBase, server.getAddress(),
                                             2, 100));

  client->connect();
  EventBaseAborter eba(&eventBase, 3000);
  eventBase.loop();

  server.getEventBase().runInEventBaseThread([&handshakeCallback]{
      handshakeCallback.closeSocket();});
  // give time for the cache lookup to come back and find it closed
  usleep(500000);

  BOOST_CHECK_EQUAL(server.getAsyncCallbacks(), 1);
  BOOST_CHECK_EQUAL(server.getAsyncLookups(), 1);
  BOOST_CHECK_EQUAL(client->getErrors(), 1);
  BOOST_CHECK_EQUAL(client->getMiss(), 1);
  BOOST_CHECK_EQUAL(client->getHit(), 0);

  cerr << "SSLServerCacheCloseTest test completed" << endl;
}

/**
 * Verify sucessful behavior of SSL certificate validation.
 */
BOOST_AUTO_TEST_CASE(SSLHandshakeValidationSuccess) {
  TEventBase eventBase;
  auto clientCtx = std::make_shared<SSLContext>();
  auto dfServerCtx = std::make_shared<SSLContext>();

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));

  SSLHandshakeClient client(std::move(clientSock), true, true, true);
  clientCtx->loadTrustedCertificates("thrift/lib/cpp/test/ssl/ca-cert.pem");

  SSLHandshakeServer server(std::move(serverSock), true, true, true);

  eventBase.loop();

  BOOST_CHECK(client.handshakeVerify_);
  BOOST_CHECK(client.handshakeSuccess_);
  BOOST_CHECK(!client.handshakeError_);
  BOOST_CHECK(!server.handshakeVerify_);
  BOOST_CHECK(server.handshakeSuccess_);
  BOOST_CHECK(!server.handshakeError_);
}

/**
 * Verify that the client's verification callback is able to fail SSL
 * connection establishment.
 */
BOOST_AUTO_TEST_CASE(SSLHandshakeValidationFailure) {
  TEventBase eventBase;
  auto clientCtx = std::make_shared<SSLContext>();
  auto dfServerCtx = std::make_shared<SSLContext>();

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));

  SSLHandshakeClient client(std::move(clientSock), true, true, false);
  clientCtx->loadTrustedCertificates("thrift/lib/cpp/test/ssl/ca-cert.pem");

  SSLHandshakeServer server(std::move(serverSock), true, true, true);

  eventBase.loop();

  BOOST_CHECK(client.handshakeVerify_);
  BOOST_CHECK(!client.handshakeSuccess_);
  BOOST_CHECK(client.handshakeError_);
  BOOST_CHECK(!server.handshakeVerify_);
  BOOST_CHECK(!server.handshakeSuccess_);
  BOOST_CHECK(server.handshakeError_);
}

/**
 * Verify that the client's verification callback is able to override
 * the preverification failure and allow a successful connection.
 */
BOOST_AUTO_TEST_CASE(SSLHandshakeValidationOverride) {
  TEventBase eventBase;
  auto clientCtx = std::make_shared<SSLContext>();
  auto dfServerCtx = std::make_shared<SSLContext>();

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));

  SSLHandshakeClient client(std::move(clientSock), true, false, true);
  SSLHandshakeServer server(std::move(serverSock), true, true, true);

  eventBase.loop();

  BOOST_CHECK(client.handshakeVerify_);
  BOOST_CHECK(client.handshakeSuccess_);
  BOOST_CHECK(!client.handshakeError_);
  BOOST_CHECK(!server.handshakeVerify_);
  BOOST_CHECK(server.handshakeSuccess_);
  BOOST_CHECK(!server.handshakeError_);
}

/**
 * Verify that specifying that no validation should be performed allows an
 * otherwise-invalid certificate to be accepted and doesn't fire the validation
 * callback.
 */
BOOST_AUTO_TEST_CASE(SSLHandshakeValidationSkip) {
  TEventBase eventBase;
  auto clientCtx = std::make_shared<SSLContext>();
  auto dfServerCtx = std::make_shared<SSLContext>();

  int fds[2];
  getfds(fds);
  getctx(clientCtx, dfServerCtx);

  TAsyncSSLSocket::UniquePtr clientSock(
    new TAsyncSSLSocket(clientCtx, &eventBase, fds[0], false));
  TAsyncSSLSocket::UniquePtr serverSock(
    new TAsyncSSLSocket(dfServerCtx, &eventBase, fds[1], true));

  SSLHandshakeClient client(std::move(clientSock), false, false, false);
  SSLHandshakeServer server(std::move(serverSock), false, false, false);

  eventBase.loop();

  BOOST_CHECK(!client.handshakeVerify_);
  BOOST_CHECK(client.handshakeSuccess_);
  BOOST_CHECK(!client.handshakeError_);
  BOOST_CHECK(!server.handshakeVerify_);
  BOOST_CHECK(server.handshakeSuccess_);
  BOOST_CHECK(!server.handshakeError_);
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value =
    "TAsyncSSLSocketTest";
  signal(SIGPIPE, SIG_IGN);

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
