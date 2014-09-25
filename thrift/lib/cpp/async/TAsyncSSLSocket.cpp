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

#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>

#include <thrift/lib/cpp/TLogging.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <thrift/lib/cpp/concurrency/SpinLock.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <openssl/err.h>
#include <openssl/asn1.h>
#include <openssl/ssl.h>

#include <folly/Bits.h>
#include <folly/io/IOBuf.h>
#include <folly/io/Cursor.h>

using folly::SocketAddress;
using apache::thrift::transport::TTransportException;
using apache::thrift::transport::SSLContext;
using std::string;
using std::shared_ptr;

using folly::Endian;
using folly::IOBuf;
using folly::io::Cursor;
using std::unique_ptr;
using std::bind;

namespace {
using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncSSLSocket;
using apache::thrift::concurrency::Util;
using apache::thrift::concurrency::SpinLock;
using apache::thrift::concurrency::SpinLockGuard;
using folly::Optional;

/** Try to avoid calling SSL_write() for buffers smaller than this: */
size_t MIN_WRITE_SIZE = 1500;

// We have one single dummy SSL context so that we can implement attach
// and detach methods in a thread safe fashion without modifying opnessl.
static SSLContext *dummyCtx = nullptr;
static SpinLock dummyCtxLock;

// Numbers chosen as to not collide with functions in ssl.h
const uint8_t TASYNCSSLSOCKET_F_PERFORM_READ = 90;
const uint8_t TASYNCSSLSOCKET_F_PERFORM_WRITE = 91;

// This converts "illegal" shutdowns into ZERO_RETURN
inline bool zero_return(int error, int rc) {
  return (error == SSL_ERROR_ZERO_RETURN || (rc == 0 && errno == 0));
}

class TAsyncSSLSocketConnector: public TAsyncSocket::ConnectCallback,
                                public TAsyncSSLSocket::HandshakeCallback {

 private:
  TAsyncSSLSocket *sslSocket_;
  TAsyncSSLSocket::ConnectCallback *callback_;
  int timeout_;
  int64_t startTime_;

 protected:
  virtual ~TAsyncSSLSocketConnector() {
  }

 public:
  TAsyncSSLSocketConnector(TAsyncSSLSocket *sslSocket,
                           TAsyncSocket::ConnectCallback *callback,
                           int timeout) :
      sslSocket_(sslSocket),
      callback_(callback),
      timeout_(timeout),
      startTime_(Util::currentTime()) {
  }

  virtual void connectSuccess() noexcept {
    T_DEBUG("client socket connected");

    int64_t timeoutLeft = 0;
    if (timeout_ > 0) {
      timeoutLeft = timeout_ - (Util::currentTime() - startTime_);
      if (timeoutLeft <= 0) {
        TTransportException ex(TTransportException::TIMED_OUT,
                               "SSL connect timed out");
        fail(ex);
        delete this;
        return;
      }
    }
    sslSocket_->sslConnect(this, timeoutLeft);
  }

  virtual void connectError(const TTransportException& ex) noexcept {
    T_ERROR("TCP connect failed: %s", ex.what());
    fail(ex);
    delete this;
  }

  virtual void handshakeSuccess(TAsyncSSLSocket *sock) noexcept {
    T_DEBUG("client handshake success");
    if (callback_) {
      callback_->connectSuccess();
    }
    delete this;
  }

  virtual void handshakeError(TAsyncSSLSocket *socket,
                              const TTransportException& ex) noexcept {
    T_ERROR("client handshakeError: %s", ex.what());
    fail(ex);
    delete this;
  }

  void fail(const TTransportException &ex) {
    // fail is a noop if called twice
    if (callback_) {
      TAsyncSSLSocket::ConnectCallback *cb = callback_;
      callback_ = nullptr;
      cb->connectError(ex);
      sslSocket_->closeNow();
      // closeNow can call handshakeError if it hasn't been called already.
      // So this may have been deleted, no member variable access beyond this
      // point
      // Note that closeNow may invoke writeError callbacks if the socket had
      // write data pending connection completion.
    }
  }
};

/**
 * Utility class that corks a TCP socket upon construction or uncorks
 * the socket upon destruction
 */
class CorkGuard {
 public:
  CorkGuard(int fd, bool multipleWrites, bool haveMore, bool* corked):
    fd_(fd), haveMore_(haveMore), corked_(corked) {
    if (*corked_) {
      // socket is already corked; nothing to do
      return;
    }
    if (multipleWrites || haveMore) {
      // We are performing multiple writes in this performWrite() call,
      // and/or there are more calls to performWrite() that will be invoked
      // later, so enable corking
      int flag = 1;
      setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag));
      *corked_ = true;
    }
  }

  ~CorkGuard() {
    if (haveMore_) {
      // more data to come; don't uncork yet
      return;
    }
    if (!*corked_) {
      // socket isn't corked; nothing to do
      return;
    }

    int flag = 0;
    setsockopt(fd_, IPPROTO_TCP, TCP_CORK, &flag, sizeof(flag));
    *corked_ = false;
  }

 private:
  CorkGuard(const CorkGuard&) = delete;
  CorkGuard& operator=(const CorkGuard&) = delete;

  int fd_;
  bool haveMore_;
  bool* corked_;
};

void setup_SSL_CTX(SSL_CTX *ctx) {
#ifdef SSL_MODE_RELEASE_BUFFERS
  SSL_CTX_set_mode(ctx,
                   SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                   SSL_MODE_ENABLE_PARTIAL_WRITE
                   | SSL_MODE_RELEASE_BUFFERS
                   );
#else
  SSL_CTX_set_mode(ctx,
                   SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER |
                   SSL_MODE_ENABLE_PARTIAL_WRITE
                   );
#endif
}

BIO_METHOD eorAwareBioMethod;

__attribute__((__constructor__))
void initEorBioMethod(void) {
  memcpy(&eorAwareBioMethod, BIO_s_socket(), sizeof(eorAwareBioMethod));
  // override the bwrite method for MSG_EOR support
  eorAwareBioMethod.bwrite = TAsyncSSLSocket::eorAwareBioWrite;

  // Note that the eorAwareBioMethod.type and eorAwareBioMethod.name are not
  // set here. openssl code seems to be checking ".type == BIO_TYPE_SOCKET" and
  // then have specific handlings. The eorAwareBioWrite should be compatible
  // with the one in openssl.
}

} // anonymous namespace

namespace apache { namespace thrift { namespace async {

TSSLException::TSSLException(int sslError, int errno_copy):
    TTransportException(
      TTransportException::SSL_ERROR,
      ERR_error_string(sslError, msg_),
      sslError == SSL_ERROR_SYSCALL ? errno_copy : 0), error_(sslError) {}

/**
 * Create a client TAsyncSSLSocket
 */
TAsyncSSLSocket::TAsyncSSLSocket(const shared_ptr<transport::SSLContext> &ctx,
                                 TEventBase* evb) :
    TAsyncSocket(evb),
    ctx_(ctx),
    handshakeTimeout_(this, evb) {
  setup_SSL_CTX(ctx_->getSSLCtx());
}

/**
 * Create a server/client TAsyncSSLSocket
 */
TAsyncSSLSocket::TAsyncSSLSocket(const shared_ptr<SSLContext>& ctx,
                                 TEventBase* evb, int fd, bool server) :
    TAsyncSocket(evb, fd),
    server_(server),
    ctx_(ctx),
    handshakeTimeout_(this, evb) {
  setup_SSL_CTX(ctx_->getSSLCtx());
  if (server) {
    SSL_CTX_set_info_callback(ctx_->getSSLCtx(),
                              TAsyncSSLSocket::sslInfoCallback);
  }
}

#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)
/**
 * Create a client TAsyncSSLSocket and allow tlsext_hostname
 * to be sent in Client Hello.
 */
TAsyncSSLSocket::TAsyncSSLSocket(const shared_ptr<transport::SSLContext> &ctx,
                                 TEventBase* evb,
                                 const std::string& serverName) :
    TAsyncSocket(evb),
    ctx_(ctx),
    handshakeTimeout_(this, evb),
    tlsextHostname_(serverName) {
  setup_SSL_CTX(ctx_->getSSLCtx());
}

/**
 * Create a client TAsyncSSLSocket from an already connected fd
 * and allow tlsext_hostname to be sent in Client Hello.
 */
TAsyncSSLSocket::TAsyncSSLSocket(const shared_ptr<SSLContext>& ctx,
                                 TEventBase* evb, int fd,
                                 const std::string& serverName) :
    TAsyncSocket(evb, fd),
    ctx_(ctx),
    handshakeTimeout_(this, evb),
    tlsextHostname_(serverName) {
  setup_SSL_CTX(ctx_->getSSLCtx());
}
#endif

TAsyncSSLSocket::~TAsyncSSLSocket() {
  T_DEBUG_L(3, "actual destruction of TAsyncSSLSocket(this=%p, evb=%p, fd=%d, "
            "state=%d, sslState=%d, events=%#x)",
            this, eventBase_, fd_, int(state_), sslState_, eventFlags_);
}

void TAsyncSSLSocket::closeNow() {
  // Close the SSL connection.
  if (ssl_ != nullptr && fd_ != -1) {
    int rc = SSL_shutdown(ssl_);
    if (rc == 0) {
      rc = SSL_shutdown(ssl_);
    }
    if (rc < 0) {
      ERR_clear_error();
    }
  }

  if (sslSession_ != nullptr) {
    SSL_SESSION_free(sslSession_);
    sslSession_ = nullptr;
  }

  sslState_ = STATE_CLOSED;

  if (handshakeTimeout_.isScheduled()) {
    handshakeTimeout_.cancelTimeout();
  }

  DestructorGuard dg(this);

  if (handshakeCallback_) {
    TTransportException ex(TTransportException::END_OF_FILE,
                           "SSL connection closed locally");
    HandshakeCallback* callback = handshakeCallback_;
    handshakeCallback_ = nullptr;
    callback->handshakeError(this, ex);
  }

  if (ssl_ != nullptr) {
    SSL_free(ssl_);
    ssl_ = nullptr;
  }

  // Close the socket.
  TAsyncSocket::closeNow();
}

void TAsyncSSLSocket::shutdownWrite() {
  // SSL sockets do not support half-shutdown, so just perform a full shutdown.
  //
  // (Performing a full shutdown here is more desirable than doing nothing at
  // all.  The purpose of shutdownWrite() is normally to notify the other end
  // of the connection that no more data will be sent.  If we do nothing, the
  // other end will never know that no more data is coming, and this may result
  // in protocol deadlock.)
  close();
}

void TAsyncSSLSocket::shutdownWriteNow() {
  closeNow();
}

bool TAsyncSSLSocket::good() const {
  return (TAsyncSocket::good() &&
          (sslState_ == STATE_ACCEPTING || sslState_ == STATE_CONNECTING ||
           sslState_ == STATE_ESTABLISHED));
}

// The TAsyncTransport definition of 'good' states that the transport is
// ready to perform reads and writes, so sslState_ == UNINIT must report !good.
// connecting can be true when the sslState_ == UNINIT because the TAsyncSocket
// is connected but we haven't initiated the call to SSL_connect.
bool TAsyncSSLSocket::connecting() const {
  return (!server_ &&
          (TAsyncSocket::connecting() ||
           (TAsyncSocket::good() && (sslState_ == STATE_UNINIT ||
                                     sslState_ == STATE_CONNECTING))));
}

bool TAsyncSSLSocket::isEorTrackingEnabled() const {
  const BIO *wb = SSL_get_wbio(ssl_);
  return wb && wb->method == &eorAwareBioMethod;
}

void TAsyncSSLSocket::setEorTracking(bool track) {
  BIO *wb = SSL_get_wbio(ssl_);
  if (!wb) {
    throw TTransportException(TTransportException::INVALID_STATE,
                              "setting EOR tracking without an initialized "
                              "BIO");
  }

  if (track) {
    if (wb->method != &eorAwareBioMethod) {
      // only do this if we didn't
      wb->method = &eorAwareBioMethod;
      BIO_set_app_data(wb, this);
      appEorByteNo_ = 0;
      minEorRawByteNo_ = 0;
    }
  } else if (wb->method == &eorAwareBioMethod) {
    wb->method = BIO_s_socket();
    BIO_set_app_data(wb, nullptr);
    appEorByteNo_ = 0;
    minEorRawByteNo_ = 0;
  } else {
    CHECK(wb->method == BIO_s_socket());
  }
}

size_t TAsyncSSLSocket::getRawBytesWritten() const {
  BIO *b;
  if (!ssl_ || !(b = SSL_get_wbio(ssl_))) {
    return 0;
  }

  return BIO_number_written(b);
}

size_t TAsyncSSLSocket::getRawBytesReceived() const {
  BIO *b;
  if (!ssl_ || !(b = SSL_get_rbio(ssl_))) {
    return 0;
  }

  return BIO_number_read(b);
}


void TAsyncSSLSocket::invalidState(HandshakeCallback* callback) {
  T_ERROR("TAsyncSSLSocket(this=%p, fd=%d, state=%d, sslState=%d, "
          "events=%#x, server=%hd): "
          "sslAccept/Connect() called in invalid "
          "state, handshake callback %p, new callback %p",
          this, fd_, int(state_), sslState_, eventFlags_, short(server_),
          handshakeCallback_, callback);
  assert(!handshakeTimeout_.isScheduled());
  sslState_ = STATE_ERROR;

  TTransportException ex(TTransportException::INVALID_STATE,
                         "sslAccept() called with socket in invalid state");

  if (callback) {
    callback->handshakeError(this, ex);
  }

  // Check the socket state not the ssl state here.
  if (state_ != StateEnum::CLOSED || state_ != StateEnum::ERROR) {
    failHandshake(__func__, ex);
  }
}

void TAsyncSSLSocket::sslAccept(HandshakeCallback* callback, uint32_t timeout,
      const transport::SSLContext::SSLVerifyPeerEnum& verifyPeer) {
  DestructorGuard dg(this);
  assert(eventBase_->isInEventBaseThread());
  verifyPeer_ = verifyPeer;

  // Make sure we're in the uninitialized state
  if (!server_ || sslState_ != STATE_UNINIT || handshakeCallback_ != nullptr) {
    return invalidState(callback);
  }

  sslState_ = STATE_ACCEPTING;
  handshakeCallback_ = callback;

  if (timeout > 0) {
    handshakeTimeout_.scheduleTimeout(timeout);
  }

  /* register for a read operation (waiting for CLIENT HELLO) */
  updateEventRegistration(TEventHandler::READ, TEventHandler::WRITE);
}

#if OPENSSL_VERSION_NUMBER >= 0x009080bfL
void TAsyncSSLSocket::attachSSLContext(
  const std::shared_ptr<transport::SSLContext>& ctx) {

  // Check to ensure we are in client mode. Changing a server's ssl
  // context doesn't make sense since clients of that server would likely
  // become confused when the server's context changes.
  DCHECK(!server_);
  DCHECK(!ctx_);
  DCHECK(ctx);
  DCHECK(ctx->getSSLCtx());
  ctx_ = ctx;

  // In order to call attachSSLContext, detachSSLContext must have been
  // previously called which sets the socket's context to the dummy
  // context. Thus we must acquire this lock.
  SpinLockGuard guard(dummyCtxLock);
  SSL_set_SSL_CTX(ssl_, ctx->getSSLCtx());
}

void TAsyncSSLSocket::detachSSLContext() {
  DCHECK(ctx_);
  ctx_.reset();
  // We aren't using the initial_ctx for now, and it can introduce race
  // conditions in the destructor of the SSL object.
#ifndef OPENSSL_NO_TLSEXT
  if (ssl_->initial_ctx) {
    SSL_CTX_free(ssl_->initial_ctx);
    ssl_->initial_ctx = nullptr;
  }
#endif
  SpinLockGuard guard(dummyCtxLock);
  if (nullptr == dummyCtx) {
    // We need to lazily initialize the dummy context so we don't
    // accidentally override any programmatic settings to openssl
    dummyCtx = new SSLContext;
  }
  // We must remove this socket's references to its context right now
  // since this socket could get passed to any thread. If the context has
  // had its locking disabled, just doing a set in attachSSLContext()
  // would not be thread safe.
  SSL_set_SSL_CTX(ssl_, dummyCtx->getSSLCtx());
}
#endif

#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)
void TAsyncSSLSocket::switchServerSSLContext(
  const std::shared_ptr<transport::SSLContext>& handshakeCtx) {
  CHECK(server_);
  if (sslState_ != STATE_ACCEPTING) {
    // We log it here and allow the switch.
    // It should not affect our re-negotiation support (which
    // is not supported now).
    VLOG(6) << "fd=" << getFd()
            << " renegotation detected when switching SSL_CTX";
  }

  setup_SSL_CTX(handshakeCtx->getSSLCtx());
  SSL_CTX_set_info_callback(handshakeCtx->getSSLCtx(),
                            TAsyncSSLSocket::sslInfoCallback);
  handshakeCtx_ = handshakeCtx;
  SSL_set_SSL_CTX(ssl_, handshakeCtx->getSSLCtx());
}

bool TAsyncSSLSocket::isServerNameMatch() const {
  CHECK(!server_);

  if (!ssl_) {
    return false;
  }

  SSL_SESSION *ss = SSL_get_session(ssl_);
  if (!ss) {
    return false;
  }

  return (ss->tlsext_hostname ? true : false);
}

void TAsyncSSLSocket::setServerName(std::string serverName) noexcept {
  tlsextHostname_ = std::move(serverName);
}

#endif

void TAsyncSSLSocket::timeoutExpired() noexcept {
  if (state_ == StateEnum::ESTABLISHED &&
      (sslState_ == STATE_CACHE_LOOKUP ||
       sslState_ == STATE_RSA_ASYNC_PENDING)) {
    sslState_ = STATE_ERROR;
    // We are expecting a callback in restartSSLAccept.  The cache lookup
    // and rsa-call necessarily have pointers to this ssl socket, so delay
    // the cleanup until he calls us back.
  } else {
    assert(state_ == StateEnum::ESTABLISHED &&
           (sslState_ == STATE_CONNECTING || sslState_ == STATE_ACCEPTING));
    DestructorGuard dg(this);
    TTransportException ex(TTransportException::TIMED_OUT,
                           (sslState_ == STATE_CONNECTING) ?
                           "SSL connect timed out" : "SSL accept timed out");
    failHandshake(__func__, ex);
  }
}

int TAsyncSSLSocket::sslExDataIndex_ = -1;
concurrency::Mutex TAsyncSSLSocket::mutex_;

int TAsyncSSLSocket::getSSLExDataIndex() {
  if (sslExDataIndex_ < 0) {
    concurrency::Guard g(mutex_);
    if (sslExDataIndex_ < 0) {
      sslExDataIndex_ = SSL_get_ex_new_index(0,
          (void*)"AsyncSSLSocket data index", nullptr, nullptr, nullptr);
    }
  }
  return sslExDataIndex_;
}

TAsyncSSLSocket* TAsyncSSLSocket::getFromSSL(const SSL *ssl) {
  return static_cast<TAsyncSSLSocket *>(SSL_get_ex_data(ssl,
      getSSLExDataIndex()));
}

void TAsyncSSLSocket::failHandshake(const char* fn,
                                    const TTransportException& ex) {
  startFail();

  if (handshakeTimeout_.isScheduled()) {
    handshakeTimeout_.cancelTimeout();
  }
  if (handshakeCallback_ != nullptr) {
    HandshakeCallback* callback = handshakeCallback_;
    handshakeCallback_ = nullptr;
    callback->handshakeError(this, ex);
  }

  finishFail();
}

void TAsyncSSLSocket::invokeHandshakeCallback() {
  if (handshakeTimeout_.isScheduled()) {
    handshakeTimeout_.cancelTimeout();
  }
  if (handshakeCallback_) {
    HandshakeCallback* callback = handshakeCallback_;
    handshakeCallback_ = nullptr;
    callback->handshakeSuccess(this);
  }
}

void TAsyncSSLSocket::connect(ConnectCallback* callback,
                              const folly::SocketAddress& address,
                              int timeout,
                              const OptionMap &options,
                              const folly::SocketAddress& bindAddr)
                              noexcept {
  assert(!server_);
  assert(state_ == StateEnum::UNINIT);
  assert(sslState_ == STATE_UNINIT);
  TAsyncSSLSocketConnector *connector =
    new TAsyncSSLSocketConnector(this, callback, timeout);
  TAsyncSocket::connect(connector, address, timeout, options, bindAddr);
}

void TAsyncSSLSocket::applyVerificationOptions(SSL * ssl) {
  // apply the settings specified in verifyPeer_
  if (verifyPeer_ == SSLContext::SSLVerifyPeerEnum::USE_CTX) {
    if(ctx_->needsPeerVerification()) {
      SSL_set_verify(ssl, ctx_->getVerificationMode(),
        TAsyncSSLSocket::sslVerifyCallback);
    }
  } else {
    if (verifyPeer_ == SSLContext::SSLVerifyPeerEnum::VERIFY ||
        verifyPeer_ == SSLContext::SSLVerifyPeerEnum::VERIFY_REQ_CLIENT_CERT) {
      SSL_set_verify(ssl, SSLContext::getVerificationMode(verifyPeer_),
        TAsyncSSLSocket::sslVerifyCallback);
    }
  }
}

void TAsyncSSLSocket::sslConnect(HandshakeCallback* callback, uint64_t timeout,
        const transport::SSLContext::SSLVerifyPeerEnum& verifyPeer) {
  DestructorGuard dg(this);
  assert(eventBase_->isInEventBaseThread());

  verifyPeer_ = verifyPeer;

  // Make sure we're in the uninitialized state
  if (server_ || sslState_ != STATE_UNINIT || handshakeCallback_ != nullptr) {
    return invalidState(callback);
  }

  sslState_ = STATE_CONNECTING;
  handshakeCallback_ = callback;

  try {
    ssl_ = ctx_->createSSL();
  } catch (std::exception &e) {
    sslState_ = STATE_ERROR;
    TTransportException ex(TTransportException::INTERNAL_ERROR,
                           "error calling SSLContext::createSSL()");
    T_ERROR("TAsyncSSLSocket::sslConnect(this=%p, fd=%d): %s",
            this, fd_, e.what());
    return failHandshake(__func__, ex);
  }

  applyVerificationOptions(ssl_);

  SSL_set_fd(ssl_, fd_);
  if (sslSession_ != nullptr) {
    SSL_set_session(ssl_, sslSession_);
    SSL_SESSION_free(sslSession_);
    sslSession_ = nullptr;
  }
#if OPENSSL_VERSION_NUMBER >= 0x1000105fL && !defined(OPENSSL_NO_TLSEXT)
  if (tlsextHostname_.size()) {
    SSL_set_tlsext_host_name(ssl_, tlsextHostname_.c_str());
  }
#endif

  SSL_set_ex_data(ssl_, getSSLExDataIndex(), this);

  if (timeout > 0) {
    handshakeTimeout_.scheduleTimeout(timeout);
  }

  handleConnect();
}

SSL_SESSION *TAsyncSSLSocket::getSSLSession() {
  if (ssl_ != nullptr && sslState_ == STATE_ESTABLISHED) {
    return SSL_get1_session(ssl_);
  }

  return sslSession_;
}

void TAsyncSSLSocket::setSSLSession(SSL_SESSION *session, bool takeOwnership) {
  sslSession_ = session;
  if (!takeOwnership && session != nullptr) {
    // Increment the reference count
    CRYPTO_add(&session->references, 1, CRYPTO_LOCK_SSL_SESSION);
  }
}

void TAsyncSSLSocket::getSelectedNextProtocol(const unsigned char** protoName,
    unsigned* protoLen) const {
  if (!getSelectedNextProtocolNoThrow(protoName, protoLen)) {
    throw TTransportException(TTransportException::NOT_SUPPORTED,
                              "NPN not supported");
  }
}

bool TAsyncSSLSocket::getSelectedNextProtocolNoThrow(
  const unsigned char** protoName,
  unsigned* protoLen) const {
  *protoName = nullptr;
  *protoLen = 0;
#ifdef OPENSSL_NPN_NEGOTIATED
  SSL_get0_next_proto_negotiated(ssl_, protoName, protoLen);
  return true;
#else
  return false;
#endif
}

bool TAsyncSSLSocket::getSSLSessionReused() const {
  if (ssl_ != nullptr && sslState_ == STATE_ESTABLISHED) {
    return SSL_session_reused(ssl_);
  }
  return false;
}

const char *TAsyncSSLSocket::getNegotiatedCipherName() const {
  return (ssl_ != nullptr) ? SSL_get_cipher_name(ssl_) : nullptr;
}

const char *TAsyncSSLSocket::getSSLServerName() const {
#ifdef SSL_CTRL_SET_TLSEXT_SERVERNAME_CB
  return (ssl_ != nullptr) ? SSL_get_servername(ssl_, TLSEXT_NAMETYPE_host_name)
        : nullptr;
#else
  throw TTransportException(TTransportException::NOT_SUPPORTED,
                            "SNI not supported");
#endif
}

const char *TAsyncSSLSocket::getSSLServerNameNoThrow() const {
  try {
    return getSSLServerName();
  } catch (TTransportException& ex) {
    return nullptr;
  }
}

int TAsyncSSLSocket::getSSLVersion() const {
  return (ssl_ != nullptr) ? SSL_version(ssl_) : 0;
}

int TAsyncSSLSocket::getSSLCertSize() const {
  int certSize = 0;
  X509 *cert = (ssl_ != nullptr) ? SSL_get_certificate(ssl_) : nullptr;
  if (cert) {
    EVP_PKEY *key = X509_get_pubkey(cert);
    certSize = EVP_PKEY_bits(key);
    EVP_PKEY_free(key);
  }
  return certSize;
}

bool TAsyncSSLSocket::willBlock(int ret, int *errorOut) noexcept {
  int error = *errorOut = SSL_get_error(ssl_, ret);
  if (error == SSL_ERROR_WANT_READ) {
    // Register for read event if not already.
    updateEventRegistration(TEventHandler::READ, TEventHandler::WRITE);
    return true;
  } else if (error == SSL_ERROR_WANT_WRITE) {
    T_DEBUG_L(3, "TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
              "SSL_ERROR_WANT_WRITE",
            fd_, int(state_), sslState_, eventFlags_);
    // Register for write event if not already.
    updateEventRegistration(TEventHandler::WRITE, TEventHandler::READ);
    return true;
#ifdef SSL_ERROR_WANT_SESS_CACHE_LOOKUP
  } else if (error == SSL_ERROR_WANT_SESS_CACHE_LOOKUP) {
    // We will block but we can't register our own socket.  The callback that
    // triggered this code will re-call handleAccept at the appropriate time.

    // We can only get here if the linked libssl.so has support for this feature
    // as well, otherwise SSL_get_error cannot return our error code.
    sslState_ = STATE_CACHE_LOOKUP;

    // Unregister for all events while blocked here
    updateEventRegistration(TEventHandler::NONE,
                            TEventHandler::READ | TEventHandler::WRITE);

    // The timeout (if set) keeps running here
    return true;
#endif
#ifdef SSL_ERROR_WANT_RSA_ASYNC_PENDING
  } else if (error == SSL_ERROR_WANT_RSA_ASYNC_PENDING) {
    // Our custom openssl function has kicked off an async request to do
    // modular exponentiation.  When that call returns, a callback will
    // be invoked that will re-call handleAccept.
    sslState_ = STATE_RSA_ASYNC_PENDING;

    // Unregister for all events while blocked here
    updateEventRegistration(
      TEventHandler::NONE,
      TEventHandler::READ | TEventHandler::WRITE
    );

    // The timeout (if set) keeps running here
    return true;
#endif
  } else {
    // SSL_ERROR_ZERO_RETURN is processed here so we can get some detail
    // in the log
    long lastError = ERR_get_error();
    VLOG(6) << "TAsyncSSLSocket(fd=" << fd_ << ", "
            << "state=" << state_ << ", "
            << "sslState=" << sslState_ << ", "
            << "events=" << std::hex << eventFlags_ << "): "
            << "SSL error: " << error << ", "
            << "errno: " << errno << ", "
            << "ret: " << ret << ", "
            << "read: " << BIO_number_read(SSL_get_rbio(ssl_)) << ", "
            << "written: " << BIO_number_written(SSL_get_wbio(ssl_)) << ", "
            << "func: " << ERR_func_error_string(lastError) << ", "
            << "reason: " << ERR_reason_error_string(lastError);
    if (error != SSL_ERROR_SYSCALL) {
      if (error == SSL_ERROR_SSL) {
        *errorOut = lastError;
      }
      if ((unsigned long)lastError < 0x8000) {
        errno = ENOSYS;
      } else {
        errno = lastError;
      }
    }
    ERR_clear_error();
    return false;
  }
}

void TAsyncSSLSocket::checkForImmediateRead() noexcept {
  // openssl may have buffered data that it read from the socket already.
  // In this case we have to process it immediately, rather than waiting for
  // the socket to become readable again.
  if (ssl_ != nullptr && SSL_pending(ssl_) > 0) {
    TAsyncSocket::handleRead();
  }
}

void
TAsyncSSLSocket::restartSSLAccept()
{
  T_DEBUG_L(3, "TAsyncSSLSocket::restartSSLAccept() this=%p, fd=%d, state=%d, "
            "sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);
  DestructorGuard dg(this);
  assert(
    sslState_ == STATE_CACHE_LOOKUP ||
    sslState_ == STATE_RSA_ASYNC_PENDING ||
    sslState_ == STATE_ERROR ||
    sslState_ == STATE_CLOSED
  );
  if (sslState_ == STATE_CLOSED) {
    // I sure hope whoever closed this socket didn't delete it already,
    // but this is not strictly speaking an error
    return;
  }
  if (sslState_ == STATE_ERROR) {
    // go straight to fail if timeout expired during lookup
    TTransportException ex(TTransportException::TIMED_OUT,
                           "SSL accept timed out");
    failHandshake(__func__, ex);
    return;
  }
  sslState_ = STATE_ACCEPTING;
  this->handleAccept();
}

void
TAsyncSSLSocket::handleAccept() noexcept {
  T_DEBUG_L(3, "TAsyncSSLSocket::handleAccept() this=%p, fd=%d, state=%d, "
            "sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);
  assert(server_);
  assert(state_ == StateEnum::ESTABLISHED &&
         sslState_ == STATE_ACCEPTING);
  if (!ssl_) {
    /* lazily create the SSL structure */
    try {
      ssl_ = ctx_->createSSL();
    } catch (std::exception &e) {
      sslState_ = STATE_ERROR;
      TTransportException ex(TTransportException::INTERNAL_ERROR,
                             "error calling SSLContext::createSSL()");
      T_ERROR("TAsyncSSLSocket::handleAccept(this=%p, fd=%d): %s",
              this, fd_, e.what());
      return failHandshake(__func__, ex);
    }
    SSL_set_fd(ssl_, fd_);
    SSL_set_ex_data(ssl_, getSSLExDataIndex(), this);

    applyVerificationOptions(ssl_);
  }

  if (server_ && parseClientHello_) {
    SSL_set_msg_callback_arg(ssl_, this);
    SSL_set_msg_callback(ssl_, &TAsyncSSLSocket::clientHelloParsingCallback);
  }

  errno = 0;
  int ret = SSL_accept(ssl_);
  if (ret <= 0) {
    int error;
    if (willBlock(ret, &error)) {
      return;
    } else {
      sslState_ = STATE_ERROR;
      TSSLException ex(error, errno);
      return failHandshake(__func__, ex);
    }
  }

  handshakeComplete_ = true;
  updateEventRegistration(0, TEventHandler::READ | TEventHandler::WRITE);

  // Move into STATE_ESTABLISHED in the normal case that we are in
  // STATE_ACCEPTING.
  sslState_ = STATE_ESTABLISHED;

  T_DEBUG_L(3, "TAsyncSSLSocket %p: fd %d successfully accepted; "
            "state=%d, sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);

  // Remember the TEventBase we are attached to, before we start invoking any
  // callbacks (since the callbacks may call detachEventBase()).
  TEventBase* originalEventBase = eventBase_;

  // Call the accept callback.
  invokeHandshakeCallback();

  // Note that the accept callback may have changed our state.
  // (set or unset the read callback, called write(), closed the socket, etc.)
  // The following code needs to handle these situations correctly.
  //
  // If the socket has been closed, readCallback_ and writeReqHead_ will
  // always be nullptr, so that will prevent us from trying to read or write.
  //
  // The main thing to check for is if eventBase_ is still originalEventBase.
  // If not, we have been detached from this event base, so we shouldn't
  // perform any more operations.
  if (eventBase_ != originalEventBase) {
    return;
  }

  TAsyncSocket::handleInitialReadWrite();
}

void
TAsyncSSLSocket::handleConnect() noexcept {
  T_DEBUG_L(3, "TAsyncSSLSocket::handleConnect() this=%p, fd=%d, state=%d, "
            "sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);
  assert(!server_);
  if (state_ < StateEnum::ESTABLISHED) {
    return TAsyncSocket::handleConnect();
  }

  assert(state_ == StateEnum::ESTABLISHED &&
         sslState_ == STATE_CONNECTING);
  assert(ssl_);

  errno = 0;
  int ret = SSL_connect(ssl_);
  if (ret <= 0) {
    int error;
    if (willBlock(ret, &error)) {
      return;
    } else {
      sslState_ = STATE_ERROR;
      TSSLException ex(error, errno);
      return failHandshake(__func__, ex);
    }
  }

  handshakeComplete_ = true;
  updateEventRegistration(0, TEventHandler::READ | TEventHandler::WRITE);

  // Move into STATE_ESTABLISHED in the normal case that we are in
  // STATE_CONNECTING.
  sslState_ = STATE_ESTABLISHED;

  T_DEBUG_L(3, "TAsyncSSLSocket %p: fd %d successfully connected; "
            "state=%d, sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);

  // Remember the TEventBase we are attached to, before we start invoking any
  // callbacks (since the callbacks may call detachEventBase()).
  TEventBase* originalEventBase = eventBase_;

  // Call the handshake callback.
  invokeHandshakeCallback();

  // Note that the connect callback may have changed our state.
  // (set or unset the read callback, called write(), closed the socket, etc.)
  // The following code needs to handle these situations correctly.
  //
  // If the socket has been closed, readCallback_ and writeReqHead_ will
  // always be nullptr, so that will prevent us from trying to read or write.
  //
  // The main thing to check for is if eventBase_ is still originalEventBase.
  // If not, we have been detached from this event base, so we shouldn't
  // perform any more operations.
  if (eventBase_ != originalEventBase) {
    return;
  }

  TAsyncSocket::handleInitialReadWrite();
}

void
TAsyncSSLSocket::handleRead() noexcept {
  T_DEBUG_L(5, "TAsyncSSLSocket::handleRead() this=%p, fd=%d, state=%d, "
            "sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);
  if (state_ < StateEnum::ESTABLISHED) {
    return TAsyncSocket::handleRead();
  }


  if (sslState_ == STATE_ACCEPTING) {
    assert(server_);
    handleAccept();
    return;
  }
  else if (sslState_ == STATE_CONNECTING) {
    assert(!server_);
    handleConnect();
    return;
  }

  // Normal read
  TAsyncSocket::handleRead();
}

ssize_t
TAsyncSSLSocket::performRead(void* buf, size_t buflen) {
  errno = 0;
  ssize_t bytes = SSL_read(ssl_, buf, buflen);
  if (server_ && renegotiateAttempted_) {
    T_ERROR("TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
            "client intitiated SSL renegotiation not permitted",
            fd_, int(state_), sslState_, eventFlags_);
    // We pack our own SSLerr here with a dummy function
    errno = ERR_PACK(ERR_LIB_USER, TASYNCSSLSOCKET_F_PERFORM_READ,
                     SSL_CLIENT_RENEGOTIATION_ATTEMPT);
    ERR_clear_error();
    return READ_ERROR;
  }
  if (bytes <= 0) {
    int error = SSL_get_error(ssl_, bytes);
    if (error == SSL_ERROR_WANT_READ) {
      // The caller will register for read event if not already.
      return READ_BLOCKING;
    } else if (error == SSL_ERROR_WANT_WRITE) {
      // TODO: Even though we are attempting to read data, SSL_read() may
      // need to write data if renegotiation is being performed.  We currently
      // don't support this and just fail the read.
      T_ERROR("TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
              "unsupported SSL renegotiation during read",
              fd_, int(state_), sslState_, eventFlags_);
      errno = ERR_PACK(ERR_LIB_USER, TASYNCSSLSOCKET_F_PERFORM_READ,
                       SSL_INVALID_RENEGOTIATION);
      ERR_clear_error();
      return READ_ERROR;
    } else {
      // TODO: Fix this code so that it can return a proper error message
      // to the callback, rather than relying on TAsyncSocket code which
      // can't handle SSL errors.
      long lastError = ERR_get_error();

      VLOG(6) << "TAsyncSSLSocket(fd=" << fd_ << ", "
              << "state=" << state_ << ", "
              << "sslState=" << sslState_ << ", "
              << "events=" << std::hex << eventFlags_ << "): "
              << "bytes: " << bytes << ", "
              << "error: " << error << ", "
              << "errno: " << errno << ", "
              << "func: " << ERR_func_error_string(lastError) << ", "
              << "reason: " << ERR_reason_error_string(lastError);
      ERR_clear_error();
      if (zero_return(error, bytes)) {
        return bytes;
      }
      if (error != SSL_ERROR_SYSCALL) {
        if ((unsigned long)lastError < 0x8000) {
          errno = ENOSYS;
        } else {
          errno = lastError;
        }
      }
      return READ_ERROR;
    }
  } else {
    appBytesReceived_ += bytes;
    return bytes;
  }
}

void TAsyncSSLSocket::handleWrite() noexcept {
  T_DEBUG_L(5, "TAsyncSSLSocket::handleWrite() this=%p, fd=%d, state=%d, "
            "sslState=%d, events=%#x",
            this, fd_, int(state_), sslState_, eventFlags_);
  if (state_ < StateEnum::ESTABLISHED) {
    return TAsyncSocket::handleWrite();
  }

  if (sslState_ == STATE_ACCEPTING) {
    assert(server_);
    handleAccept();
    return;
  }

  if (sslState_ == STATE_CONNECTING) {
    assert(server_);
    handleConnect();
    return;
  }

  // Normal write
  TAsyncSocket::handleWrite();
}

ssize_t TAsyncSSLSocket::performWrite(const iovec* vec,
                                      uint32_t count,
                                      WriteFlags flags,
                                      uint32_t* countWritten,
                                      uint32_t* partialWritten) {
  if (sslState_ != STATE_ESTABLISHED) {
      T_ERROR("TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
              "TODO: TAsyncSSLSocket currently does not support calling "
              "write() before the handshake has fully completed",
              fd_, int(state_), sslState_, eventFlags_);
      errno = ERR_PACK(ERR_LIB_USER, TASYNCSSLSOCKET_F_PERFORM_WRITE,
                       SSL_EARLY_WRITE);
      return -1;
  }

  bool cork = isSet(flags, WriteFlags::CORK);
  CorkGuard guard(fd_, count > 1, cork, &corked_);

  *countWritten = 0;
  *partialWritten = 0;
  ssize_t totalWritten = 0;
  size_t bytesStolenFromNextBuffer = 0;
  for (int i = 0; i < count; i++) {
    const iovec* v = vec + i;
    size_t offset = bytesStolenFromNextBuffer;
    bytesStolenFromNextBuffer = 0;
    size_t len = v->iov_len - offset;
    const void* buf;
    if (len == 0) {
      (*countWritten)++;
      continue;
    }
    buf = ((const char*)v->iov_base) + offset;

    ssize_t bytes;
    errno = 0;
    uint32_t buffersStolen = 0;
    if ((len < MIN_WRITE_SIZE) && ((i + 1) < count)) {
      // Combine this buffer with part or all of the next buffers in
      // order to avoid really small-grained calls to SSL_write().
      // Each call to SSL_write() produces a separate record in
      // the egress SSL stream, and we've found that some low-end
      // mobile clients can't handle receiving an HTTP response
      // header and the first part of the response body in two
      // separate SSL records (even if those two records are in
      // the same TCP packet).
      char combinedBuf[MIN_WRITE_SIZE];
      memcpy(combinedBuf, buf, len);
      do {
        // INVARIANT: i + buffersStolen == complete chunks serialized
        uint32_t nextIndex = i + buffersStolen + 1;
        bytesStolenFromNextBuffer = std::min(vec[nextIndex].iov_len,
                                             MIN_WRITE_SIZE - len);
        memcpy(combinedBuf + len, vec[nextIndex].iov_base,
               bytesStolenFromNextBuffer);
        len += bytesStolenFromNextBuffer;
        if (bytesStolenFromNextBuffer < vec[nextIndex].iov_len) {
          // couldn't steal the whole buffer
          break;
        } else {
          bytesStolenFromNextBuffer = 0;
          buffersStolen++;
        }
      } while ((i + buffersStolen + 1) < count && (len < MIN_WRITE_SIZE));
      bytes = eorAwareSSLWrite(
        ssl_, combinedBuf, len,
        (isSet(flags, WriteFlags::EOR) && i + buffersStolen + 1 == count));

    } else {
      bytes = eorAwareSSLWrite(ssl_, buf, len,
                           (isSet(flags, WriteFlags::EOR) && i + 1 == count));
    }

    if (bytes <= 0) {
      int error = SSL_get_error(ssl_, bytes);
      if (error == SSL_ERROR_WANT_WRITE) {
        // The caller will register for write event if not already.
        *partialWritten = offset;
        return totalWritten;
      } else if (error == SSL_ERROR_WANT_READ) {
        // TODO: Even though we are attempting to write data, SSL_write() may
        // need to read data if renegotiation is being performed.  We currently
        // don't support this and just fail the write.
        T_ERROR("TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
                "unsupported SSL renegotiation during write",
                fd_, int(state_), sslState_, eventFlags_);
        errno = ERR_PACK(ERR_LIB_USER, TASYNCSSLSOCKET_F_PERFORM_WRITE,
                         SSL_INVALID_RENEGOTIATION);
        ERR_clear_error();
        return -1;
      } else {
        // TODO: Fix this code so that it can return a proper error message
        // to the callback, rather than relying on TAsyncSocket code which
        // can't handle SSL errors.
        long lastError = ERR_get_error();
        T_DEBUG_L(
          3,
          "ERROR: TAsyncSSLSocket(fd=%d, state=%d, sslState=%d, events=%#x): "
          "SSL error: %d, errno: %d, func: %s, reason: %s",
          fd_, int(state_), sslState_, eventFlags_, error, errno,
          ERR_func_error_string(lastError),
          ERR_reason_error_string(lastError)
        );
        if (error != SSL_ERROR_SYSCALL) {
          if ((unsigned long)lastError < 0x8000) {
            errno = ENOSYS;
          } else {
            errno = lastError;
          }
        }
        ERR_clear_error();
        if (!zero_return(error, bytes)) {
          return -1;
        } // else fall through to below to correctly record totalWritten
      }
    }

    totalWritten += bytes;

    if (bytes == len) {
      // The full iovec is written.
      (*countWritten) += 1 + buffersStolen;
      i += buffersStolen;
      // continue
    } else {
      bytes += offset; // adjust bytes to account for all of v
      while (bytes >= v->iov_len) {
        // We combined this buf with part or all of the next one, and
        // we managed to write all of this buf but not all of the bytes
        // from the next one that we'd hoped to write.
        bytes -= v->iov_len;
        (*countWritten)++;
        v = &(vec[++i]);
      }
      *partialWritten = bytes;
      return totalWritten;
    }
  }

  return totalWritten;
}

int TAsyncSSLSocket::eorAwareSSLWrite(SSL *ssl, const void *buf, int n,
                                      bool eor) {
  if (eor && SSL_get_wbio(ssl)->method == &eorAwareBioMethod) {
    if (appEorByteNo_) {
      // cannot track for more than one app byte EOR
      CHECK(appEorByteNo_ == appBytesWritten_ + n);
    } else {
      appEorByteNo_ = appBytesWritten_ + n;
    }

    // 1. It is fine to keep updating minEorRawByteNo_.
    // 2. It is _min_ in the sense that SSL record will add some overhead.
    minEorRawByteNo_ = getRawBytesWritten() + n;
  }

  n = sslWriteImpl(ssl, buf, n);
  if (n > 0) {
    appBytesWritten_ += n;
    if (appEorByteNo_) {
      if (getRawBytesWritten() >= minEorRawByteNo_) {
        minEorRawByteNo_ = 0;
      }
      if(appBytesWritten_ == appEorByteNo_) {
        appEorByteNo_ = 0;
      } else {
        CHECK(appBytesWritten_ < appEorByteNo_);
      }
    }
  }
  return n;
}

void
TAsyncSSLSocket::sslInfoCallback(const SSL *ssl, int where, int ret) {
  TAsyncSSLSocket *sslSocket = TAsyncSSLSocket::getFromSSL(ssl);
  if (sslSocket->handshakeComplete_ && (where & SSL_CB_HANDSHAKE_START)) {
    sslSocket->renegotiateAttempted_ = true;
  }
}

int TAsyncSSLSocket::eorAwareBioWrite(BIO *b, const char *in, int inl) {
  int ret;
  struct msghdr msg;
  struct iovec iov;
  int flags = 0;
  TAsyncSSLSocket *tsslSock;

  iov.iov_base = const_cast<char *>(in);
  iov.iov_len = inl;
  memset(&msg, 0, sizeof(msg));
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  tsslSock =
    reinterpret_cast<TAsyncSSLSocket*>(BIO_get_app_data(b));
  if (tsslSock &&
      tsslSock->minEorRawByteNo_ &&
      tsslSock->minEorRawByteNo_ <= BIO_number_written(b) + inl) {
    flags = MSG_EOR;
  }

  errno = 0;
  ret = sendmsg(b->num, &msg, flags);
  BIO_clear_retry_flags(b);
  if (ret <= 0) {
    if (BIO_sock_should_retry(ret))
      BIO_set_retry_write(b);
  }
  return(ret);
}

int TAsyncSSLSocket::sslVerifyCallback(int preverifyOk,
                                       X509_STORE_CTX* x509Ctx) {
  SSL* ssl = (SSL*) X509_STORE_CTX_get_ex_data(
    x509Ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
  TAsyncSSLSocket* self = TAsyncSSLSocket::getFromSSL(ssl);

  T_DEBUG_L(3, "TAsyncSSLSocket::sslVerifyCallback() this=%p, "
            "fd=%d, preverifyOk=%d", self, self->fd_, preverifyOk);
  return (self->handshakeCallback_) ?
    self->handshakeCallback_->handshakeVerify(self, preverifyOk, x509Ctx) :
    preverifyOk;
}

void TAsyncSSLSocket::enableClientHelloParsing()  {
    parseClientHello_ = true;
    clientHelloInfo_.reset(new ClientHelloInfo());
}

void TAsyncSSLSocket::resetClientHelloParsing(SSL *ssl)  {
  SSL_set_msg_callback(ssl, nullptr);
  SSL_set_msg_callback_arg(ssl, nullptr);
  clientHelloInfo_->clientHelloBuf_.clear();
}

void
TAsyncSSLSocket::clientHelloParsingCallback(int written, int version,
    int contentType, const void *buf, size_t len, SSL *ssl, void *arg)
{
  TAsyncSSLSocket *sock = static_cast<TAsyncSSLSocket*>(arg);
  if (written != 0) {
    sock->resetClientHelloParsing(ssl);
    return;
  }
  if (contentType != SSL3_RT_HANDSHAKE) {
    sock->resetClientHelloParsing(ssl);
    return;
  }
  if (len == 0) {
    return;
  }

  auto& clientHelloBuf = sock->clientHelloInfo_->clientHelloBuf_;
  clientHelloBuf.append(IOBuf::wrapBuffer(buf, len));
  try {
    Cursor cursor(clientHelloBuf.front());
    if (cursor.read<uint8_t>() != SSL3_MT_CLIENT_HELLO) {
      sock->resetClientHelloParsing(ssl);
      return;
    }

    if (cursor.totalLength() < 3) {
      clientHelloBuf.trimEnd(len);
      clientHelloBuf.append(IOBuf::copyBuffer(buf, len));
      return;
    }

    uint32_t messageLength = cursor.read<uint8_t>();
    messageLength <<= 8;
    messageLength |= cursor.read<uint8_t>();
    messageLength <<= 8;
    messageLength |= cursor.read<uint8_t>();
    if (cursor.totalLength() < messageLength) {
      clientHelloBuf.trimEnd(len);
      clientHelloBuf.append(IOBuf::copyBuffer(buf, len));
      return;
    }

    sock->clientHelloInfo_->clientHelloMajorVersion_ = cursor.read<uint8_t>();
    sock->clientHelloInfo_->clientHelloMinorVersion_ = cursor.read<uint8_t>();

    cursor.skip(4); // gmt_unix_time
    cursor.skip(28); // random_bytes

    cursor.skip(cursor.read<uint8_t>()); // session_id

    uint16_t cipherSuitesLength = cursor.readBE<uint16_t>();
    for (int i = 0; i < cipherSuitesLength; i += 2) {
      sock->clientHelloInfo_->
        clientHelloCipherSuites_.push_back(cursor.readBE<uint16_t>());
    }

    uint8_t compressionMethodsLength = cursor.read<uint8_t>();
    for (int i = 0; i < compressionMethodsLength; ++i) {
      sock->clientHelloInfo_->
        clientHelloCompressionMethods_.push_back(cursor.readBE<uint8_t>());
    }

    if (cursor.totalLength() > 0) {
      uint16_t extensionsLength = cursor.readBE<uint16_t>();
      while (extensionsLength) {
        sock->clientHelloInfo_->
          clientHelloExtensions_.push_back(cursor.readBE<uint16_t>());
        extensionsLength -= 2;
        uint16_t extensionDataLength = cursor.readBE<uint16_t>();
        extensionsLength -= 2;
        cursor.skip(extensionDataLength);
        extensionsLength -= extensionDataLength;
      }
    }
  } catch (std::out_of_range& e) {
    // we'll use what we found and cleanup below.
    VLOG(4) << "TAsyncSSLSocket::clientHelloParsingCallback(): "
      << "buffer finished unexpectedly." << " TAsyncSSLSocket socket=" << sock;
  }

  sock->resetClientHelloParsing(ssl);
}

}}} // apache::thrift::async
