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

#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <folly/Memory.h>
#include <folly/ScopeGuard.h>

using namespace apache::thrift;
using apache::thrift::transport::THeader;
using namespace boost::python;

namespace {

object makePythonAddress(const transport::TSocketAddress& sa) {
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

}

class ContextData {
public:
  void copyContextContents(Cpp2RequestContext* ctx) {
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
  transport::TSocketAddress peerAddress_;
  transport::TSocketAddress localAddress_;
};

class PythonAsyncProcessor : public AsyncProcessor {
public:
  explicit PythonAsyncProcessor(std::shared_ptr<object> adapter)
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

    std::unique_ptr<IOBuf> outbuf;

    {
      PyGILState_STATE state = PyGILState_Ensure();
      SCOPE_EXIT { PyGILState_Release(state); };

      auto cd_ctor = adapter_->attr("CONTEXT_DATA");
      object contextData = cd_ctor();
      extract<ContextData&>(contextData)().copyContextContents(context);

      object output =
        adapter_->attr("call_processor")(
          input, int(clientType), int(protType), contextData);
      if (output.is_none()) {
        return;
      }

      outbuf = folly::IOBuf::copyBuffer(
        extract<const char *>(output), extract<int>(output.attr("__len__")()));
    }
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
  std::shared_ptr<object> adapter_;
};

class PythonAsyncProcessorFactory : public AsyncProcessorFactory {
public:
  explicit PythonAsyncProcessorFactory(std::shared_ptr<object> adapter)
    : adapter_(adapter) {}

  virtual std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() {
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

  void serveNoGil() {
    // So, normal order of operations:
    // setup, drop GIL, loop, cleanup, acquire GIL.

    // Hold the GIL while setup() is running.  This makes
    // getPythonAddress thread-safe, since it will hold the GIL, too.

    setup();

    // Deadlock avoidance: consider a thrift worker thread is doing
    // something in C++-land having relinquished the GIL.  This thread
    // acquires the GIL, stops the workers, and waits for the worker
    // threads to complete.  The worker thread now finishes its work,
    // and tries to reacquire the GIL, but deadlocks with the current
    // thread, which holds the GIL and is waiting for the worker to
    // complete.  So we do cleanUp() without the GIL, and reacquire it
    // only once thrift is all cleaned up.

    // The first SCOPE_EXIT, and therefore the last to be executed,
    // reacquires the GIL, if it was dropped.  (This code accounts for
    // the possibility of PyEval_SaveThread() failing, which is
    // admittedly unlikely.)

    PyThreadState* save_state = nullptr;

    SCOPE_EXIT {
      // Now arrange to clean up thrift.

      cleanUp();

      // Acquire the GIL

      if (save_state) {
        PyEval_RestoreThread(save_state);
      }
    };

    // Drop the GIL

    save_state = PyEval_SaveThread();

    // Thrift main loop.  This will run indefinitely, until stop() is
    // called.  Then the cleanups will happen in reverse declared
    // order above: cleanUp() will happen without the GIL, then the
    // GIL will be reacquired.

    getServeEventBase()->loop();
  }
};

BOOST_PYTHON_MODULE(_cpp_server_wrapper) {
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
    .def("getAddress", &CppServerWrapper::getAddress)
    .def("serve", &CppServerWrapper::serveNoGil)

    // methods directly passed to the C++ impl
    .def("setPort", &CppServerWrapper::setPort)
    .def("stop", &CppServerWrapper::stop)
    ;
}
