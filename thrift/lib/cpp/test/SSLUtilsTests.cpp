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

#include <gtest/gtest.h>
#include <glog/logging.h>

#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <string>

#include <thrift/lib/cpp/ssl/SSLUtils.h>
#include <thrift/lib/cpp/transport/TSocketAddress.h>
#include <thrift/lib/cpp/transport/TSSLSocket.h>

using apache::thrift::ssl::OpenSSLUtils;
using apache::thrift::transport::SSLContext;
using apache::thrift::transport::TSocketAddress;

class X509Cert {
 public:
  explicit X509Cert(const std::string& certPath) {
    ctx_.loadCertificate(certPath.c_str(), "PEM");
    ssl_ = ctx_.createSSL();
    CHECK(ssl_ != nullptr);
  }
  ~X509Cert() {
    SSL_free(ssl_);
  }
  X509* getX509() {
    return SSL_get_certificate(ssl_);
  }
 private:
  X509Cert(const X509Cert&) = delete;
  X509Cert& operator=(const X509Cert&) = delete;

  SSLContext ctx_;
  SSL* ssl_;
};

TEST(SSLUtilsTest, ValidatePeerCertNamesIPSanityTest) {
  X509Cert cert("thrift/lib/cpp/test/ssl/tests-cert.pem");
  TSocketAddress addr;

  addr.setFromIpPort("127.0.0.1", 1);
  EXPECT_TRUE(OpenSSLUtils::validatePeerCertNames(
                  cert.getX509(), addr.getAddress(), addr.getActualSize()));
  addr.setFromIpPort("::1", 1);
  EXPECT_TRUE(OpenSSLUtils::validatePeerCertNames(
                  cert.getX509(), addr.getAddress(), addr.getActualSize()));
  addr.setFromIpPort("127.0.0.2", 1);
  EXPECT_FALSE(OpenSSLUtils::validatePeerCertNames(
                   cert.getX509(), addr.getAddress(), addr.getActualSize()));
  addr.setFromIpPort("::2", 1);
  EXPECT_FALSE(OpenSSLUtils::validatePeerCertNames(
                   cert.getX509(), addr.getAddress(), addr.getActualSize()));
}
