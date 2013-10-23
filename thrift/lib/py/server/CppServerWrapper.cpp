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
#include <glog/logging.h>

#include "boost/python/class.hpp"
#include "boost/python/def.hpp"
#include "boost/python/import.hpp"
#include "boost/python/errors.hpp"
#include "boost/python/module.hpp"

#include "thrift/lib/cpp2/async/AsyncProcessor.h"
#include "thrift/lib/cpp2/server/ThriftServer.h"
#include "folly/Memory.h"

using namespace apache::thrift;
using namespace boost::python;

class PythonAsyncProcessor : public AsyncProcessor {
public:
  explicit PythonAsyncProcessor(object adapter)
    : adapter_(adapter) {}

  virtual void process(std::unique_ptr<ResponseChannel::Request> req,
                       std::unique_ptr<folly::IOBuf> buf,
                       apache::thrift::protocol::PROTOCOL_TYPES protType,
                       Cpp2RequestContext* context,
                       apache::thrift::async::TEventBase* eb,
                       apache::thrift::concurrency::ThreadManager* tm) {
    auto input = buf->moveToFbString().toStdString();
    auto clientType = context->getHeader()->getClientType();
    if (clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
      // SASL processing is already done, and we're not going to put
      // it back.  So just use standard header here.
      clientType = THRIFT_HEADER_CLIENT_TYPE;
    }
    object client_principal;
    if (context->getSaslServer()) {
      client_principal = str(context->getSaslServer()->getClientIdentity());
    }

    object output =
      adapter_.attr("call_processor")(
        input, clientType, int(protType), client_principal);
    if (output.is_none()) {
      return;
    }

    auto outbuf = folly::IOBuf::copyBuffer(
      extract<const char *>(output), extract<int>(output.attr("__len__")()));
    req->sendReply(std::move(outbuf));
  }

  virtual bool isOnewayMethod(const folly::IOBuf* buf,
                              const THeader* header) {
    // TODO mhorowitz: I have no idea how to make this work.  I'm not
    // sure python understands oneway.  I'm not even sure C++
    // meaningfully does.
    return false;
  }

private:
  object adapter_;
};

class PythonAsyncProcessorFactory : public AsyncProcessorFactory {
public:
  explicit PythonAsyncProcessorFactory(object adapter)
    : adapter_(adapter) {}

  virtual std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() {
    return folly::make_unique<PythonAsyncProcessor>(adapter_);
  }

private:
  object adapter_;
};

class CppServerWrapper {
public:
  CppServerWrapper()
    : server_(new ThriftServer) {}

  void setAdapter(object adapter) {
    server_->setProcessorFactory(
      folly::make_unique<PythonAsyncProcessorFactory>(adapter));
  }

  void setPort(int port) {
    server_->setPort(port);
  }

  void serve() {
    server_->serve();
  }

private:
  std::shared_ptr<ThriftServer> server_;
};

BOOST_PYTHON_MODULE(_cpp_server_wrapper) {
  class_<CppServerWrapper>("CppServerWrapper")
    .def("setAdapter", &CppServerWrapper::setAdapter)
    .def("setPort", &CppServerWrapper::setPort)
    .def("serve", &CppServerWrapper::serve)
    ;
}
