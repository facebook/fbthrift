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

#include <glog/logging.h>

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/import.hpp>
#include <boost/python/module.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/dict.hpp>

#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>

using namespace apache::thrift;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::transport::THeader;
using apache::thrift::server::TServerEventHandler;
using apache::thrift::server::TConnectionContext;
using namespace boost::python;

namespace {

object makePythonAddress(const folly::SocketAddress& sa) {
  if (!sa.isInitialized()) {
    return object();  // None
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

object makePythonHeaders(const std::map<std::string, std::string>& cppheaders) {
  object headers = dict();
  for (const auto& it : cppheaders) {
    headers[it.first] = it.second;
  }
  return headers;
}

}

class ContextData {
public:
  void copyContextContents(Cpp2ConnContext* ctx) {
    if (!ctx) {
      return;
    }
    auto ss = ctx->getSaslServer();
    if (ss) {
      clientIdentity_ = ss->getClientIdentity();
    } else {
      clientIdentity_.clear();
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

  object getClientIdentity() const {
    if (clientIdentity_.empty()) {
      return object();
    } else {
      return str(clientIdentity_);
    }
  }

  object getPeerAddress() const {
    return makePythonAddress(peerAddress_);
  }

  object getLocalAddress() const {
    return makePythonAddress(localAddress_);
  }

private:
  std::string clientIdentity_;
  folly::SocketAddress peerAddress_;
  folly::SocketAddress localAddress_;
};

class CppServerEventHandler : public TServerEventHandler {
public:
  explicit CppServerEventHandler(object serverEventHandler)
    : handler_(std::make_shared<object>(serverEventHandler)) {}

  void newConnection(TConnectionContext* ctx) override {
    callPythonHandler(ctx, "newConnection");
  }

  void connectionDestroyed(TConnectionContext* ctx) override {
    callPythonHandler(ctx, "connectionDestroyed");
  }

private:
  void callPythonHandler(TConnectionContext *ctx, const char* method) {
    PyGILState_STATE state = PyGILState_Ensure();
    SCOPE_EXIT { PyGILState_Release(state); };

    // This cast always succeeds because it is called from Cpp2Connection.
    Cpp2ConnContext *cpp2Ctx = dynamic_cast<Cpp2ConnContext*>(ctx);
    auto cd_cls = handler_->attr("CONTEXT_DATA");
    object contextData = cd_cls();
    extract<ContextData&>(contextData)().copyContextContents(cpp2Ctx);
    auto ctx_cls = handler_->attr("CPP_CONNECTION_CONTEXT");
    object cppConnContext = ctx_cls(contextData);
    handler_->attr(method)(cppConnContext);
  }

  std::shared_ptr<object> handler_;
};

class PythonAsyncProcessor : public AsyncProcessor {
public:
  explicit PythonAsyncProcessor(std::shared_ptr<object> adapter)
    : adapter_(adapter) {}

  void process(std::unique_ptr<ResponseChannel::Request> req,
               std::unique_ptr<folly::IOBuf> buf,
               apache::thrift::protocol::PROTOCOL_TYPES protType,
               Cpp2RequestContext* context,
               apache::thrift::async::TEventBase* eb,
               apache::thrift::concurrency::ThreadManager* tm) override {
    folly::ByteRange input_range = buf->coalesce();
    auto input_data = const_cast<unsigned char*>(input_range.data());
    auto clientType = context->getHeader()->getClientType();
    if (clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
      // SASL processing is already done, and we're not going to put
      // it back.  So just use standard header here.
      clientType = THRIFT_HEADER_CLIENT_TYPE;
    }

    std::unique_ptr<folly::IOBuf> outbuf;

    {
      PyGILState_STATE state = PyGILState_Ensure();
      SCOPE_EXIT { PyGILState_Release(state); };

#if PY_MAJOR_VERSION == 2
      auto input = handle<>(
        PyBuffer_FromMemory(input_data, input_range.size()));
#else
      auto input = handle<>(
        PyMemoryView_FromMemory(reinterpret_cast<char*>(input_data),
                                input_range.size(), PyBUF_READ));
#endif

      auto cd_ctor = adapter_->attr("CONTEXT_DATA");
      object contextData = cd_ctor();
      extract<ContextData&>(contextData)().copyContextContents(
          context->getConnectionContext());

      object output = adapter_->attr("call_processor")(
          input,
          makePythonHeaders(context->getHeader()->getHeaders()),
          int(clientType),
          int(protType),
          contextData);
      if (output.is_none()) {
        throw std::runtime_error("Unexpected error in processor method");
      }
      PyObject* output_ptr = output.ptr();
#if PY_MAJOR_VERSION == 2
      if (PyString_Check(output_ptr)) {
        int len = extract<int>(output.attr("__len__")());
        if (len == 0) {
          // assume oneway, don't call sendReply
          return;
        }
        outbuf = folly::IOBuf::copyBuffer(extract<const char *>(output), len);
      } else
#endif
        if (PyBytes_Check(output_ptr)) {
          int len = PyBytes_Size(output_ptr);
          if (len == 0) {
            // assume oneway, don't call sendReply
            return;
          }
          outbuf = folly::IOBuf::copyBuffer(PyBytes_AsString(output_ptr), len);
        } else {
          throw std::runtime_error(
            "Return from processor method is not string or bytes");
        }
    }
    req->sendReply(THeader::transform(
      std::move(outbuf),
      context->getHeader()->getWriteTransforms(),
      context->getHeader()->getMinCompressBytes()));
  }

  bool isOnewayMethod(const folly::IOBuf* buf, const THeader* header) override {
    // TODO mhorowitz: I have no idea how to make this work.  I'm not
    // sure python understands oneway.  I'm not even sure C++
    // meaningfully does.
    return false;
  }

private:
  std::shared_ptr<object> adapter_;
};

class PythonAsyncProcessorFactory : public AsyncProcessorFactory {
public:
  explicit PythonAsyncProcessorFactory(std::shared_ptr<object> adapter)
    : adapter_(adapter) {}

  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return folly::make_unique<PythonAsyncProcessor>(adapter_);
  }

private:
  std::shared_ptr<object> adapter_;
};

class CppServerWrapper : public ThriftServer {
public:
  void setAdapter(object adapter) {
    // We use a shared_ptr to manage the adapter so the processor
    // factory handing won't ever try to manipulate python reference
    // counts without the GIL.
    setProcessorFactory(
      folly::make_unique<PythonAsyncProcessorFactory>(
        std::make_shared<object>(adapter)));
  }

  object getAddress() {
    return makePythonAddress(ThriftServer::getAddress());
  }

  void loop() {
    PyThreadState* save_state = PyEval_SaveThread();
    SCOPE_EXIT { PyEval_RestoreThread(save_state); };

    // Thrift main loop.  This will run indefinitely, until stop() is
    // called.

    getServeEventBase()->loopForever();
  }

  void cleanUp() {
    // Deadlock avoidance: consider a thrift worker thread is doing
    // something in C++-land having relinquished the GIL.  This thread
    // acquires the GIL, stops the workers, and waits for the worker
    // threads to complete.  The worker thread now finishes its work,
    // and tries to reacquire the GIL, but deadlocks with the current
    // thread, which holds the GIL and is waiting for the worker to
    // complete.  So we do cleanUp() without the GIL, and reacquire it
    // only once thrift is all cleaned up.

    PyThreadState* save_state = PyEval_SaveThread();
    SCOPE_EXIT { PyEval_RestoreThread(save_state); };

    ThriftServer::cleanUp();
  }

  void setIdleTimeout(int timeout) {
    std::chrono::milliseconds ms(timeout);
    ThriftServer::setIdleTimeout(ms);
  }

  void setCppServerEventHandler(object serverEventHandler) {
    setServerEventHandler(std::make_shared<CppServerEventHandler>(
          serverEventHandler));
  }

  void setNewSimpleThreadManager(
      size_t count,
      size_t pendingTaskCountMax,
      bool enableTaskStats,
      size_t maxQueueLen) {
    auto tm = ThreadManager::newSimpleThreadManager(
        count, pendingTaskCountMax, enableTaskStats, maxQueueLen);
    tm->threadFactory(std::make_shared<PosixThreadFactory>());
    tm->start();
    setThreadManager(std::move(tm));
}
};

BOOST_PYTHON_MODULE(CppServerWrapper) {
  PyEval_InitThreads();

  class_<ContextData>("ContextData")
    .def("getClientIdentity", &ContextData::getClientIdentity)
    .def("getPeerAddress", &ContextData::getPeerAddress)
    .def("getLocalAddress", &ContextData::getLocalAddress)
    ;

  class_<CppServerWrapper, boost::noncopyable >(
    "CppServerWrapper")
    // methods added or customized for the python implementation
    .def("setAdapter", &CppServerWrapper::setAdapter)
    .def("setIdleTimeout", &CppServerWrapper::setIdleTimeout)
    .def("getAddress", &CppServerWrapper::getAddress)
    .def("loop", &CppServerWrapper::loop)
    .def("cleanUp", &CppServerWrapper::cleanUp)
    .def("setCppServerEventHandler",
         &CppServerWrapper::setCppServerEventHandler)
    .def("setNewSimpleThreadManager",
         &CppServerWrapper::setNewSimpleThreadManager)

    // methods directly passed to the C++ impl
    .def("setup", &CppServerWrapper::setup)
    .def("setNPoolThreads", &CppServerWrapper::setNPoolThreads)
    .def("setNWorkerThreads", &CppServerWrapper::setNWorkerThreads)
    .def("setPort", &CppServerWrapper::setPort)
    .def("stop", &CppServerWrapper::stop)
    .def("setMaxConnections", &CppServerWrapper::setMaxConnections)
    .def("getMaxConnections", &CppServerWrapper::getMaxConnections)
    ;
}
