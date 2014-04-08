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

#include "thrift/lib/cpp/transport/TSSLSocket.h"

#include <errno.h>
#include <string>
#include <vector>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>
#include <boost/scoped_array.hpp>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include "thrift/lib/cpp/concurrency/Mutex.h"
#ifdef __x86_64__
#include "folly/SmallLocks.h"
#include "folly/String.h"
#else
#include "thrift/lib/cpp/concurrency/SpinLock.h"
#endif
#include <glog/logging.h>

using std::list;
using std::string;
using std::exception;
using std::vector;
using boost::lexical_cast;
using std::shared_ptr;
using boost::scoped_array;
using namespace apache::thrift::concurrency;

struct CRYPTO_dynlock_value {
  Mutex mutex;
};

namespace apache { namespace thrift { namespace transport {


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
                       const TSocketAddress& address) :
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
  int rc = SSL_get_verify_result(ssl_);
  if (rc != X509_V_OK) {
    throw TSSLException(string("SSL_get_verify_result(), ") +
                        X509_verify_cert_error_string(rc));
  }

  // nothing left to do if we're not checking the certificate name of the peer
  if (!ctx_->validatePeerName(this, ssl_)) {
    throw TSSLException("verifyCertificate: name verification failed");
  }
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

// ---------------------------------------------------------------------
// TSSLContext implementation
// ---------------------------------------------------------------------

uint64_t SSLContext::count_ = 0;
Mutex    SSLContext::mutex_;
#ifdef OPENSSL_NPN_NEGOTIATED
int SSLContext::sNextProtocolsExDataIndex_ = -1;

#endif
// SSLContext implementation
SSLContext::SSLContext(SSLVersion version) {
  {
    Guard g(mutex_);
    if (!count_++) {
      initializeOpenSSL();
      randomize();
#ifdef OPENSSL_NPN_NEGOTIATED
      sNextProtocolsExDataIndex_ = SSL_get_ex_new_index(0,
          (void*)"Advertised next protocol index", nullptr, nullptr, nullptr);
#endif
    }
  }

  ctx_ = SSL_CTX_new(SSLv23_method());
  if (ctx_ == nullptr) {
    throw TSSLException("SSL_CTX_new: " + getErrors());
  }

  int opt = 0;
  switch (version) {
    case TLSv1:
      opt = SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3;
      break;
    case SSLv3:
      opt = SSL_OP_NO_SSLv2;
      break;
    default:
      // do nothing
      break;
  }
  int newOpt = SSL_CTX_set_options(ctx_, opt);
  assert((newOpt & opt) == opt);

  SSL_CTX_set_mode(ctx_, SSL_MODE_AUTO_RETRY);

  checkPeerName_ = false;

#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)
  SSL_CTX_set_tlsext_servername_callback(ctx_, baseServerNameOpenSSLCallback);
  SSL_CTX_set_tlsext_servername_arg(ctx_, this);
#endif
}

SSLContext::~SSLContext() {
  if (ctx_ != nullptr) {
    SSL_CTX_free(ctx_);
    ctx_ = nullptr;
  }

#ifdef OPENSSL_NPN_NEGOTIATED
  deleteNextProtocolsStrings();
#endif

  Guard g(mutex_);
  if (!--count_) {
    cleanupOpenSSL();
  }
}

void SSLContext::ciphers(const string& ciphers) {
  providedCiphersString_ = ciphers;
  setCiphersOrThrow(ciphers);
}

void SSLContext::setCiphersOrThrow(const string& ciphers) {
  int rc = SSL_CTX_set_cipher_list(ctx_, ciphers.c_str());
  if (ERR_peek_error() != 0) {
    throw TSSLException("SSL_CTX_set_cipher_list: " + getErrors());
  }
  if (rc == 0) {
    throw TSSLException("None of specified ciphers are supported");
  }
}

void SSLContext::authenticate(bool checkPeerCert, bool checkPeerName,
                              const string& peerName) {
  int mode;
  if (checkPeerCert) {
    mode  = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE;
    checkPeerName_ = checkPeerName;
    peerFixedName_ = peerName;
  } else {
    mode = SSL_VERIFY_NONE;
    checkPeerName_ = false; // can't check name without cert!
    peerFixedName_.clear();
  }
  SSL_CTX_set_verify(ctx_, mode, nullptr);
}

void SSLContext::loadCertificate(const char* path, const char* format) {
  if (path == nullptr || format == nullptr) {
    throw TTransportException(TTransportException::BAD_ARGS,
         "loadCertificateChain: either <path> or <format> is nullptr");
  }
  if (strcmp(format, "PEM") == 0) {
    if (SSL_CTX_use_certificate_chain_file(ctx_, path) == 0) {
      int errnoCopy = errno;
      string reason("SSL_CTX_use_certificate_chain_file: ");
      reason.append(path);
      reason.append(": ");
      reason.append(getErrors(errnoCopy));
      throw TSSLException(reason);
    }
  } else {
    throw TSSLException("Unsupported certificate format: " + string(format));
  }
}

void SSLContext::loadPrivateKey(const char* path, const char* format) {
  if (path == nullptr || format == nullptr) {
    throw TTransportException(TTransportException::BAD_ARGS,
         "loadPrivateKey: either <path> or <format> is nullptr");
  }
  if (strcmp(format, "PEM") == 0) {
    if (SSL_CTX_use_PrivateKey_file(ctx_, path, SSL_FILETYPE_PEM) == 0) {
      throw TSSLException("SSL_CTX_use_PrivateKey_file: " + getErrors());
    }
  } else {
    throw TSSLException("Unsupported private key format: " + string(format));
  }
}

void SSLContext::loadTrustedCertificates(const char* path) {
  if (path == nullptr) {
    throw TTransportException(TTransportException::BAD_ARGS,
         "loadTrustedCertificates: <path> is nullptr");
  }
  if (SSL_CTX_load_verify_locations(ctx_, path, nullptr) == 0) {
    throw TSSLException("SSL_CTX_load_verify_locations: " + getErrors());
  }
}

void SSLContext::loadTrustedCertificates(X509_STORE* store) {
  SSL_CTX_set_cert_store(ctx_, store);
}

void SSLContext::loadClientCAList(const char* path) {
  auto clientCAs = SSL_load_client_CA_file(path);
  if (clientCAs == nullptr) {
    LOG(CRITICAL) << "Unable to load ca file: " << path;
    return;
  }
  SSL_CTX_set_client_CA_list(ctx_, clientCAs);
}

void SSLContext::randomize() {
  RAND_poll();
}

void SSLContext::passwordCollector(shared_ptr<PasswordCollector> collector) {
  if (collector == nullptr) {
    GlobalOutput("passwordCollector: ignore invalid password collector");
    return;
  }
  collector_ = collector;
  SSL_CTX_set_default_passwd_cb(ctx_, passwordCallback);
  SSL_CTX_set_default_passwd_cb_userdata(ctx_, this);
}

#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)

void SSLContext::setServerNameCallback(const ServerNameCallback& cb) {
  serverNameCb_ = cb;
}

void SSLContext::addClientHelloCallback(const ClientHelloCallback& cb) {
  clientHelloCbs_.push_back(cb);
}

int SSLContext::baseServerNameOpenSSLCallback(SSL* ssl, int* al, void* data) {
  SSLContext* context = (SSLContext*)data;

  if (context == nullptr) {
    return SSL_TLSEXT_ERR_NOACK;
  }

  for (auto& cb : context->clientHelloCbs_) {
    // Generic callbacks to happen after we receive the Client Hello.
    // For example, we use one to switch which cipher we use depending
    // on the user's TLS version.  Because the primary purpose of
    // baseServerNameOpenSSLCallback is for SNI support, and these callbacks
    // are side-uses, we ignore any possible failures other than just logging
    // them.
    cb(ssl);
  }

  if (!context->serverNameCb_) {
    return SSL_TLSEXT_ERR_NOACK;
  }

  ServerNameCallbackResult ret = context->serverNameCb_(ssl);
  switch (ret) {
    case SERVER_NAME_FOUND:
      return SSL_TLSEXT_ERR_OK;
    case SERVER_NAME_NOT_FOUND:
      return SSL_TLSEXT_ERR_NOACK;
    case SERVER_NAME_NOT_FOUND_ALERT_FATAL:
      *al = TLS1_AD_UNRECOGNIZED_NAME;
      return SSL_TLSEXT_ERR_ALERT_FATAL;
    default:
      CHECK(false);
  }

  return SSL_TLSEXT_ERR_NOACK;
}

void SSLContext::switchCiphersIfTLS11(
    SSL* ssl,
    const std::string& tls11CipherString) {

  CHECK(!tls11CipherString.empty()) << "Shouldn't call if empty alt ciphers";

  if (TLS1_get_client_version(ssl) <= TLS1_VERSION) {
    // We only do this for TLS v 1.1 and later
    return;
  }

  // Prefer AES for TLS versions 1.1 and later since these are not
  // vulnerable to BEAST attacks on AES.  Note that we're setting the
  // cipher list on the SSL object, not the SSL_CTX object, so it will
  // only last for this request.
  int rc = SSL_set_cipher_list(ssl, tls11CipherString.c_str());
  if ((rc == 0) || ERR_peek_error() != 0) {
    // This shouldn't happen since we checked for this when proxygen
    // started up.
    LOG(WARNING) << "ssl_cipher: No specified ciphers supported for switch";
    SSL_set_cipher_list(ssl, providedCiphersString_.c_str());
  }
}
#endif

#ifdef OPENSSL_NPN_NEGOTIATED
bool SSLContext::setAdvertisedNextProtocols(const list<string>& protocols) {
  return setRandomizedAdvertisedNextProtocols({{1, protocols}});
}

bool SSLContext::setRandomizedAdvertisedNextProtocols(
    const list<NextProtocolsItem>& items) {
  unsetNextProtocols();
  if (items.size() == 0) {
    return false;
  }
  int total_weight = 0;
  for (const auto &item : items) {
    if (item.protocols.size() == 0) {
      continue;
    }
    AdvertisedNextProtocolsItem advertised_item;
    advertised_item.length = 0;
    for (const auto& proto : item.protocols) {
      ++advertised_item.length;
      unsigned protoLength = proto.length();
      if (protoLength >= 256) {
        deleteNextProtocolsStrings();
        return false;
      }
      advertised_item.length += protoLength;
    }
    advertised_item.protocols = new unsigned char[advertised_item.length];
    if (!advertised_item.protocols) {
      throw TSSLException("alloc failure");
    }
    unsigned char* dst = advertised_item.protocols;
    for (auto& proto : item.protocols) {
      unsigned protoLength = proto.length();
      *dst++ = (unsigned char)protoLength;
      memcpy(dst, proto.data(), protoLength);
      dst += protoLength;
    }
    total_weight += item.weight;
    advertised_item.probability = item.weight;
    advertisedNextProtocols_.push_back(advertised_item);
  }
  if (total_weight == 0) {
    deleteNextProtocolsStrings();
    return false;
  }
  for (auto &advertised_item : advertisedNextProtocols_) {
    advertised_item.probability /= total_weight;
  }
  SSL_CTX_set_next_protos_advertised_cb(
    ctx_, advertisedNextProtocolCallback, this);
  SSL_CTX_set_next_proto_select_cb(
    ctx_, selectNextProtocolCallback, this);
  return true;
}

void SSLContext::deleteNextProtocolsStrings() {
  for (auto protocols : advertisedNextProtocols_) {
    delete[] protocols.protocols;
  }
  advertisedNextProtocols_.clear();
}

void SSLContext::unsetNextProtocols() {
  deleteNextProtocolsStrings();
  SSL_CTX_set_next_protos_advertised_cb(ctx_, nullptr, nullptr);
  SSL_CTX_set_next_proto_select_cb(ctx_, nullptr, nullptr);
}

int SSLContext::advertisedNextProtocolCallback(SSL* ssl,
      const unsigned char** out, unsigned int* outlen, void* data) {
  SSLContext* context = (SSLContext*)data;
  if (context == nullptr || context->advertisedNextProtocols_.empty()) {
    *out = nullptr;
    *outlen = 0;
  } else if (context->advertisedNextProtocols_.size() == 1) {
    *out = context->advertisedNextProtocols_[0].protocols;
    *outlen = context->advertisedNextProtocols_[0].length;
  } else {
    uintptr_t selected_index = reinterpret_cast<uintptr_t>(SSL_get_ex_data(ssl,
          sNextProtocolsExDataIndex_));
    if (selected_index) {
      --selected_index;
      *out = context->advertisedNextProtocols_[selected_index].protocols;
      *outlen = context->advertisedNextProtocols_[selected_index].length;
    } else {
      unsigned char random_byte;
      RAND_bytes(&random_byte, 1);
      double random_value = random_byte / 255.0;
      double sum = 0;
      for (size_t i = 0; i < context->advertisedNextProtocols_.size(); ++i) {
        sum += context->advertisedNextProtocols_[i].probability;
        if (sum < random_value &&
            i + 1 < context->advertisedNextProtocols_.size()) {
          continue;
        }
        uintptr_t selected = i + 1;
        SSL_set_ex_data(ssl, sNextProtocolsExDataIndex_, (void *)selected);
        *out = context->advertisedNextProtocols_[i].protocols;
        *outlen = context->advertisedNextProtocols_[i].length;
        break;
      }
    }
  }
  return SSL_TLSEXT_ERR_OK;
}

int SSLContext::selectNextProtocolCallback(
  SSL* ssl, unsigned char **out, unsigned char *outlen,
  const unsigned char *server, unsigned int server_len, void *data) {

  SSLContext* ctx = (SSLContext*)data;
  if (ctx->advertisedNextProtocols_.size() > 1) {
    VLOG(3) << "SSLContext::selectNextProcolCallback() "
            << "client should be deterministic in selecting protocols.";
  }

  unsigned char *client;
  int client_len;
  if (ctx->advertisedNextProtocols_.empty()) {
    client = (unsigned char *) "";
    client_len = 0;
  } else {
    client = ctx->advertisedNextProtocols_[0].protocols;
    client_len = ctx->advertisedNextProtocols_[0].length;
  }

  int retval = SSL_select_next_proto(out, outlen, server, server_len,
                                     client, client_len);
  if (retval != OPENSSL_NPN_NEGOTIATED) {
    VLOG(3) << "SSLContext::selectNextProcolCallback() "
            << "unable to pick a next protocol.";
  }
  return SSL_TLSEXT_ERR_OK;
}
#endif // OPENSSL_NPN_NEGOTIATED

SSL* SSLContext::createSSL() const {
  SSL* ssl = SSL_new(ctx_);
  if (ssl == nullptr) {
    throw TSSLException("SSL_new: " + getErrors());
  }
  return ssl;
}

bool SSLContext::validatePeerName(TSSLSocket* sock, SSL* ssl) const {
  if (!checkPeerName_) {
    return true;
  }

  X509* cert = SSL_get_peer_certificate(ssl);
  if (cert == nullptr) {
    throw TSSLException("verifyCertificate: certificate not present");
  }
  STACK_OF(GENERAL_NAME)* alternatives = nullptr;
  bool verified = false;
  string host;
  if (!peerFixedName_.empty()) {
    host = peerFixedName_;
  }

  try {
    // only consider alternatives if we're not matching against a fixed name
    if (peerFixedName_.empty()) {
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
        char* data = (char*)ASN1_STRING_data(name->d.ia5);
        int length = ASN1_STRING_length(name->d.ia5);
        switch (name->type) {
          case GEN_DNS:
            if (host.empty()) {
              host = (sock->server() ? sock->getPeerHost() : sock->getHost());
            }
            if (matchName(host.c_str(), data, length)) {
              verified = true;
            }
            break;
          case GEN_IPADD: {
            const TSocketAddress* remaddr = sock->getPeerAddress();
            if (remaddr->getFamily() == AF_INET &&
                length == sizeof(in_addr)) {
              if (!memcmp(&reinterpret_cast<const sockaddr_in*>(
                            remaddr->getAddress())->sin_addr,
                          data, length)) {
                verified = true;
              }
            } else if (remaddr->getFamily() == AF_INET6 &&
                       length == sizeof(in6_addr)) {
              if (!memcmp(&reinterpret_cast<const sockaddr_in6*>(
                            remaddr->getAddress())->sin6_addr,
                          data, length)) {
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
          host = (sock->server() ? sock->getPeerHost() : sock->getHost());
        }
        if (matchName(host.c_str(), (char*)utf8, size)) {
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

/**
 * Match a name with a pattern. The pattern may include wildcard. A single
 * wildcard "*" can match up to one component in the domain name.
 *
 * @param  host    Host name, typically the name of the remote host
 * @param  pattern Name retrieved from certificate
 * @param  size    Size of "pattern"
 * @return True, if "host" matches "pattern". False otherwise.
 */
bool SSLContext::matchName(const char* host, const char* pattern, int size) {
  bool match = false;
  int i = 0, j = 0;
  while (i < size && host[j] != '\0') {
    if (toupper(pattern[i]) == toupper(host[j])) {
      i++;
      j++;
      continue;
    }
    if (pattern[i] == '*') {
      while (host[j] != '.' && host[j] != '\0') {
        j++;
      }
      i++;
      continue;
    }
    break;
  }
  if (i == size && host[j] == '\0') {
    match = true;
  }
  return match;
}

int SSLContext::passwordCallback(char* password,
                                 int size,
                                 int,
                                 void* data) {
  SSLContext* context = (SSLContext*)data;
  if (context == nullptr || context->passwordCollector() == nullptr) {
    return 0;
  }
  string userPassword;
  // call user defined password collector to get password
  context->passwordCollector()->getPassword(userPassword, size);
  int length = userPassword.size();
  if (length > size) {
    length = size;
  }
  strncpy(password, userPassword.c_str(), length);
  return length;
}

struct SSLLock {
  explicit SSLLock(
    SSLContext::SSLLockType inLockType = SSLContext::LOCK_MUTEX) :
      lockType(inLockType) {
#if __x86_64__
    memset(&spinLock, 0, sizeof(folly::MicroSpinLock));
#endif
  }

  void lock() {
    if (lockType == SSLContext::LOCK_MUTEX) {
      mutex.lock();
    } else if (lockType == SSLContext::LOCK_SPINLOCK) {
      spinLock.lock();
    }
    // lockType == LOCK_NONE, no-op
  }

  void unlock() {
    if (lockType == SSLContext::LOCK_MUTEX) {
      mutex.unlock();
    } else if (lockType == SSLContext::LOCK_SPINLOCK) {
      spinLock.unlock();
    }
    // lockType == LOCK_NONE, no-op
  }

  SSLContext::SSLLockType lockType;
#if __x86_64__
  folly::MicroSpinLock spinLock;
#else
  SpinLock spinLock;
#endif
  Mutex mutex;
};

static std::map<int, SSLContext::SSLLockType> lockTypes;
static scoped_array<SSLLock> locks;

static void callbackLocking(int mode, int n, const char*, int) {
  if (mode & CRYPTO_LOCK) {
    locks[n].lock();
  } else {
    locks[n].unlock();
  }
}

static ulong callbackThreadID() {
  return static_cast<ulong>(pthread_self());
}

static CRYPTO_dynlock_value* dyn_create(const char*, int) {
  return new CRYPTO_dynlock_value;
}

static void dyn_lock(int mode,
                     struct CRYPTO_dynlock_value* lock,
                     const char*, int) {
  if (lock != nullptr) {
    if (mode & CRYPTO_LOCK) {
      lock->mutex.lock();
    } else {
      lock->mutex.unlock();
    }
  }
}

static void dyn_destroy(struct CRYPTO_dynlock_value* lock, const char*, int) {
  delete lock;
}

void SSLContext::setSSLLockTypes(std::map<int, SSLLockType> inLockTypes) {
  lockTypes = inLockTypes;
}

void SSLContext::initializeOpenSSL() {
  SSL_library_init();
  SSL_load_error_strings();
  ERR_load_crypto_strings();
  // static locking
  locks.reset(new SSLLock[::CRYPTO_num_locks()]);
  for (auto it: lockTypes) {
    locks[it.first].lockType = it.second;
  }
  CRYPTO_set_id_callback(callbackThreadID);
  CRYPTO_set_locking_callback(callbackLocking);
  // dynamic locking
  CRYPTO_set_dynlock_create_callback(dyn_create);
  CRYPTO_set_dynlock_lock_callback(dyn_lock);
  CRYPTO_set_dynlock_destroy_callback(dyn_destroy);
}

void SSLContext::cleanupOpenSSL() {
  CRYPTO_set_id_callback(nullptr);
  CRYPTO_set_locking_callback(nullptr);
  CRYPTO_set_dynlock_create_callback(nullptr);
  CRYPTO_set_dynlock_lock_callback(nullptr);
  CRYPTO_set_dynlock_destroy_callback(nullptr);
  CRYPTO_cleanup_all_ex_data();
  ERR_free_strings();
  EVP_cleanup();
  ERR_remove_state(0);
  locks.reset();
}

void SSLContext::setOptions(long options) {
  long newOpt = SSL_CTX_set_options(ctx_, options);
  if ((newOpt & options) != options) {
    throw TSSLException("SSL_CTX_set_options failed");
  }
}

string SSLContext::getErrors(int errnoCopy) {
  string errors;
  ulong  errorCode;
  char   message[256];

  errors.reserve(512);
  while ((errorCode = ERR_get_error()) != 0) {
    if (!errors.empty()) {
      errors += "; ";
    }
    const char* reason = ERR_reason_error_string(errorCode);
    if (reason == nullptr) {
      snprintf(message, sizeof(message) - 1, "SSL error # %lu", errorCode);
      reason = message;
    }
    errors += reason;
  }
  if (errors.empty()) {
    if (errnoCopy != 0) {
      errors += TOutput::strerror_s(errnoCopy);
    }
  }
  if (errors.empty()) {
    errors = "error code: " + lexical_cast<string>(errnoCopy);
  }
  return errors;
}

}}}
