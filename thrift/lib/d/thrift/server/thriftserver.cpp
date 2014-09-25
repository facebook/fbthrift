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

#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::transport;
using folly::makeMoveWrapper;

// D defined funcs
class ThriftServerInterface {
 public:
  virtual void process(ResponseChannel::Request* req,
                       TEventBase* eb,
                       unsigned char* data, size_t len, char protType);
 private:
   ~ThriftServerInterface() {}
};

// Processor to forward to D processor
class DProcessor : public AsyncProcessor {
 public:
  explicit DProcessor(ThriftServerInterface* iface) : iface_(iface) {}

  virtual void process(std::unique_ptr<ResponseChannel::Request> req,
                       std::unique_ptr<folly::IOBuf> buf,
                       protocol::PROTOCOL_TYPES protType,
                       Cpp2RequestContext* context,
                       async::TEventBase* eb,
                       concurrency::ThreadManager* tm) {
    assert(iface_);
    auto reqd = makeMoveWrapper(std::move(req));
    auto bufd = makeMoveWrapper(std::move(buf));
    tm->add(FunctionRunner::create([=] () mutable {
        (*bufd)->coalesce();
        uint64_t resp;
        char* data;
        iface_->process(
          (*reqd).release(), eb, (*bufd)->writableData(),
          (*bufd)->length(), protType);
    }));
  }

  virtual bool isOnewayMethod(const folly::IOBuf* buf,
                              const THeader* header) {
    return false;
  }
 private:
  ThriftServerInterface* iface_;
};


class DServerInterface : public ServerInterface {
 public:
  explicit DServerInterface(ThriftServerInterface* iface) : iface_(iface) {}

  virtual std::unique_ptr<AsyncProcessor> getProcessor() {
    return std::unique_ptr<AsyncProcessor>(new DProcessor(iface_));
  }
 private:
  ThriftServerInterface* iface_;
};

// Interface for D code.  Note that we use C-style functions instead of C++
// members, to avoid mismatched virtual tables.
extern "C" {

TEventBaseManager* thriftserver_getEventBaseManager(ThriftServer* server) {
  return server->getEventBaseManager();
}

const folly::SocketAddress* thriftserver_getAddress(ThriftServer* server) {
  auto& tsockAddr = server->getAddress();
  return &tsockAddr;
}

ThriftServer* thriftserver_new() {
  return new ThriftServer;
}

void thriftserver_free(ThriftServer* server) {
  delete server;
}

void thriftserver_setPort(ThriftServer* server, uint16_t port) {
  server->setPort(port);
}

void thriftserver_serve(ThriftServer* server) {
  server->serve();
}

void thriftserver_stop(ThriftServer* server) {
  server->stop();
}

void thriftserver_stopListening(ThriftServer* server) {
  server->stopListening();
}

void thriftserver_cleanUp(ThriftServer* server) {
  server->cleanUp();
}

void thriftserver_setup(ThriftServer* server) {
  server->setup();
}

void thriftserver_setInterface(
  ThriftServer* server, ThriftServerInterface* iface) {

  auto interface = std::make_shared<DServerInterface>(iface);
  server->setInterface(interface);
}

void thriftserver_sendReply(
  ResponseChannel::Request* req, TEventBase* eb,
  const char* bytes, size_t len) {

  auto buf = makeMoveWrapper(folly::IOBuf::copyBuffer(bytes, len));
  eb->runInEventBaseThread([=] () mutable {
    req->sendReply(std::move(*buf));
    delete req;
  });
}

void thriftserver_freeRequest(ResponseChannel::Request* req) {
  delete req;
}

} // extern "C"
