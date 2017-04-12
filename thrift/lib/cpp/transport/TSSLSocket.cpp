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

#include <thrift/lib/cpp/transport/TSSLSocket.h>

#include <folly/portability/OpenSSL.h>
#include <folly/portability/Sockets.h>

#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <errno.h>
#include <folly/String.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <string>
#include <vector>

#include <thrift/lib/cpp/concurrency/ProfiledMutex.h>

using apache::thrift::concurrency::ProfiledMutex;
using boost::lexical_cast;
using boost::scoped_array;
using std::exception;
using std::list;
using std::shared_ptr;
using std::string;
using std::vector;

struct CRYPTO_dynlock_value {
  ProfiledMutex<std::mutex> mutex;
};

namespace apache { namespace thrift { namespace transport {

using folly::SSLContext;
using folly::PasswordCollector;

// TSSLSocket implementation
TSSLSocket::TSSLSocket(const shared_ptr<SSLContext>& ctx):
  TVirtualTransport<TSSLSocket, TSocket>(),
  server_(false), ssl_(nullptr), ctx_(ctx) {
}

TSSLSocket::TSSLSocket(const shared_ptr<SSLContext>& ctx, int socket):
  TVirtualTransport<TSSLSocket, TSocket>(socket),
  server_(false), ssl_(nullptr), ctx_(ctx) {
}

TSSLSocket::TSSLSocket(const shared_ptr<SSLContext>& ctx,
                       const string& host, int port) :
  TVirtualTransport<TSSLSocket, TSocket>(host, port),
  server_(false), ssl_(nullptr), ctx_(ctx) {
}

TSSLSocket::TSSLSocket(const shared_ptr<SSLContext>& ctx,
                       const folly::SocketAddress& address) :
  TVirtualTransport<TSSLSocket, TSocket>(address),
  server_(false), ssl_(nullptr), ctx_(ctx) {
}

TSSLSocket::~TSSLSocket() {
  close();
}

bool TSSLSocket::isOpen() {
  if (ssl_ == nullptr || !TSocket::isOpen()) {
    return false;
  }
  int shutdown = SSL_get_shutdown(ssl_);
  bool shutdownReceived = (shutdown & SSL_RECEIVED_SHUTDOWN);
  bool shutdownSent     = (shutdown & SSL_SENT_SHUTDOWN);
  if (shutdownReceived && shutdownSent) {
    return false;
  }
  return true;
}

bool TSSLSocket::peek() {
  if (!isOpen()) {
    return false;
  }
  checkHandshake();
  int rc;
  uint8_t byte;
  rc = SSL_peek(ssl_, &byte, 1);
  if (rc < 0) {
    throw TSSLException("SSL_peek: " + ctx_->getErrors());
  }
  if (rc == 0) {
    ERR_clear_error();
  }
  return (rc > 0);
}

void TSSLSocket::open() {
  if (isOpen() || server()) {
    throw TTransportException(TTransportException::ALREADY_OPEN);
  }
  TSocket::open();
}

void TSSLSocket::close() {
  if (ssl_ != nullptr) {
    int rc = SSL_shutdown(ssl_);
    // "According to the TLS standard, it is acceptable for an application to
    // only send its shutdown alert and then close the underlying connection
    // without waiting for the peer's response", so we don't need to call
    // SSL_shutdown a second time.
    if (rc < 0) {
      string errMsg = "SSL_shutdown: " + ctx_->getErrors();
      GlobalOutput(errMsg.c_str());
    }
    SSL_free(ssl_);
    ssl_ = nullptr;
    ERR_remove_state(0);
  }
  TSocket::close();
}

uint32_t TSSLSocket::read(uint8_t* buf, uint32_t len) {
  checkHandshake();
  int32_t bytes = 0;
  for (int32_t retries = 0; retries < maxRecvRetries_; retries++){
    bytes = SSL_read(ssl_, buf, len);
    if (bytes >= 0)
      break;
    int errnoCopy = errno;
    if (SSL_get_error(ssl_, bytes) == SSL_ERROR_SYSCALL) {
      if (ERR_get_error() == 0 && errnoCopy == EINTR) {
        continue;
      }
    }
    throw TSSLException("SSL_read: " + ctx_->getErrors(errnoCopy));
  }
  return bytes;
}

void TSSLSocket::write(const uint8_t* buf, uint32_t len) {
  checkHandshake();
  // loop in case SSL_MODE_ENABLE_PARTIAL_WRITE is set in SSL_CTX.
  uint32_t written = 0;
  while (written < len) {
    int32_t bytes = SSL_write(ssl_, &buf[written], len - written);
    if (bytes <= 0) {
      throw TSSLException("SSL_write: " + ctx_->getErrors());
    }
    written += bytes;
  }
}

void TSSLSocket::flush() {
  // Don't throw exception if not open. Thrift servers close socket twice.
  if (ssl_ == nullptr) {
    return;
  }
  checkHandshake();
  BIO* bio = SSL_get_wbio(ssl_);
  if (bio == nullptr) {
    throw TSSLException("SSL_get_wbio returns nullptr");
  }
  if (BIO_flush(bio) != 1) {
    throw TSSLException("BIO_flush: " + ctx_->getErrors());
  }
}

void TSSLSocket::checkHandshake() {
  if (!TSocket::isOpen()) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "underlying socket not open in checkHandshake");
  }
  if (ssl_ != nullptr) {
    return;
  }
  ssl_ = ctx_->createSSL();
  SSL_set_fd(ssl_, socket_);
  int rc;
  if (server()) {
    rc = SSL_accept(ssl_);
  } else {
    rc = SSL_connect(ssl_);
  }
  if (rc <= 0) {
    string errors = ctx_->getErrors();
    const char* prefix = server() ? "SSL_accept: " : "SSL_connect: ";
    throw TSSLException(prefix + errors);
  }
  if (SSL_get_verify_mode(ssl_) & SSL_VERIFY_PEER) {
    verifyCertificate();
  }
}

void TSSLSocket::verifyCertificate() {

  // verify authentication result
  long rc = SSL_get_verify_result(ssl_);
  if (rc != X509_V_OK) {
    throw TSSLException(string("SSL_get_verify_result(), ") +
                        X509_verify_cert_error_string(rc));
  }

  // nothing left to do if we're not checking the certificate name of the peer
  if (!validatePeerName(ssl_)) {
    throw TSSLException("verifyCertificate: name verification failed");
  }
}

bool TSSLSocket::validatePeerName(SSL* ssl) {
  if (!ctx_->checkPeerName()) {
    return true;
  }

  X509* cert = SSL_get_peer_certificate(ssl);
  if (cert == nullptr) {
    throw TSSLException("verifyCertificate: certificate not present");
  }
  STACK_OF(GENERAL_NAME)* alternatives = nullptr;
  bool verified = false;
  string host;
  if (!ctx_->peerFixedName().empty()) {
    host = ctx_->peerFixedName();
  }

  try {
    // only consider alternatives if we're not matching against a fixed name
    if (ctx_->peerFixedName().empty()) {
      alternatives = (STACK_OF(GENERAL_NAME)*)
        X509_get_ext_d2i(cert, NID_subject_alt_name, nullptr, nullptr);
    }
    if (alternatives != nullptr) {
      // verify subjectAlternativeName
      const int count = sk_GENERAL_NAME_num(alternatives);
      for (int i = 0; !verified && i < count; i++) {
        const GENERAL_NAME* name = sk_GENERAL_NAME_value(alternatives, i);
        if (name == nullptr) {
          continue;
        }
        char* data = (char*)ASN1_STRING_get0_data(name->d.ia5);
        int length = ASN1_STRING_length(name->d.ia5);
        switch (name->type) {
          case GEN_DNS:
            if (host.empty()) {
              host = (server() ? getPeerHost() : getHost());
            }
            if (SSLContext::matchName(host.c_str(), data, length)) {
              verified = true;
            }
            break;
          case GEN_IPADD: {
            const folly::SocketAddress* remaddr = getPeerAddress();
            if (remaddr->getFamily() == AF_INET &&
                length == sizeof(in_addr)) {
              in_addr addr = remaddr->getIPAddress().asV4().toAddr();
              if (!memcmp(&addr, data, length)) {
                verified = true;
              }
            } else if (remaddr->getFamily() == AF_INET6 &&
                       length == sizeof(in6_addr)) {
              in6_addr addr = remaddr->getIPAddress().asV6().toAddr();
              if (!memcmp(&addr, data, length)) {
                verified = true;
              }
            }
            break;
          }
        }
      }
      sk_GENERAL_NAME_pop_free(alternatives, GENERAL_NAME_free);
      alternatives = nullptr;
    }

    if (verified) {
      X509_free(cert);
      return true;
    }

    // verify commonName
    X509_NAME* name = X509_get_subject_name(cert);
    if (name != nullptr) {
      X509_NAME_ENTRY* entry;
      unsigned char* utf8;
      int last = -1;
      while (!verified) {
        last = X509_NAME_get_index_by_NID(name, NID_commonName, last);
        if (last == -1)
          break;
        entry = X509_NAME_get_entry(name, last);
        if (entry == nullptr)
          continue;
        ASN1_STRING* common = X509_NAME_ENTRY_get_data(entry);
        int size = ASN1_STRING_to_UTF8(&utf8, common);
        string fart((char*)utf8, size);
        if (host.empty()) {
          host = (server() ? getPeerHost() : getHost());
        }
        if (SSLContext::matchName(host.c_str(), (char*)utf8, size)) {
          verified = true;
        }
        OPENSSL_free(utf8);
      }
    }

    X509_free(cert);
    cert = nullptr;
  } catch (const exception& e) {
    if (alternatives) {
      sk_GENERAL_NAME_pop_free(alternatives, GENERAL_NAME_free);
    }
    if (cert) {
      X509_free(cert);
    }
    throw;
  }

  return verified;
}

// ---------------------------------------------------------------------
// TSSLSocketFactory implementation
// ---------------------------------------------------------------------

TSSLSocketFactory::TSSLSocketFactory(const shared_ptr<SSLContext>& context)
    : ctx_(context),
      server_(false) {
}

TSSLSocketFactory::~TSSLSocketFactory() {
}

shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket() {
  shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_));
  ssl->server(server());
  return ssl;
}

shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket(int socket) {
  shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, socket));
  ssl->server(server());
  return ssl;
}

shared_ptr<TSSLSocket> TSSLSocketFactory::createSocket(const string& host,
                                                       int port) {
  shared_ptr<TSSLSocket> ssl(new TSSLSocket(ctx_, host, port));
  ssl->server(server());
  return ssl;
}

}}}
