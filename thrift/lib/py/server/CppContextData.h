/*
 * Copyright 2016 Facebook, Inc.
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

#ifndef CPP_CONTEXT_DATA_H
#define CPP_CONTEXT_DATA_H


#include <boost/python/object.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>

namespace apache { namespace thrift {

class Cpp2ConnContext;

boost::python::object makePythonAddress(const folly::SocketAddress& sa) {
  if (!sa.isInitialized()) {
    return boost::python::object();  // None
  }

  // This constructs a tuple in the same form as socket.getpeername()
  if (sa.getFamily() == AF_INET) {
    return boost::python::make_tuple(sa.getAddressStr(), sa.getPort());
  } else if (sa.getFamily() == AF_INET6) {
    return boost::python::make_tuple(sa.getAddressStr(), sa.getPort(), 0, 0);
  } else {
    LOG(FATAL) << "CppServerWrapper can't create a non-inet thrift endpoint";
    abort();
  }
}

class CppContextData {
public:
  CppContextData()
    : ctx_(nullptr) {}

  void copyContextContents(Cpp2ConnContext* ctx) {
    if (!ctx) {
      return;
    }
    ctx_ = ctx;

    clientIdentity_ = ctx->getPeerCommonName();
    if (clientIdentity_.empty()) {
    auto ss = ctx->getSaslServer();
      if (ss) {
        clientIdentity_ = ss->getClientIdentity();
      }
    }

    auto pa = ctx->getPeerAddress();
    if (pa) {
      peerAddress_ = *pa;
    } else {
      peerAddress_.reset();
    }

    auto la = ctx->getLocalAddress();
    if (la) {
      localAddress_ = *la;
    } else {
      localAddress_.reset();
    }
  }
  boost::python::object getClientIdentity() const {
    if (clientIdentity_.empty()) {
      return boost::python::object();
    } else {
      return boost::python::str(clientIdentity_);
    }
  }
  boost::python::object getPeerAddress() const {
    return makePythonAddress(peerAddress_);
  }
  boost::python::object getLocalAddress() const {
    return makePythonAddress(localAddress_);
  }
  const Cpp2ConnContext * getCtx() const {
    return ctx_;
  }
private:
  const apache::thrift::Cpp2ConnContext* ctx_;
  std::string clientIdentity_;
  folly::SocketAddress peerAddress_;
  folly::SocketAddress localAddress_;
};
}}

#endif
