#include <src/gen-py3/module_services_wrapper.h>
#include <src/gen-py3/module_server.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace cpp2 {

MyServiceWrapper::MyServiceWrapper(PyObject *obj)
  : if_object(obj)
  {
    Py_XINCREF(this->if_object);
  }

MyServiceWrapper::~MyServiceWrapper() {
    Py_XDECREF(this->if_object);
}

folly::Future<folly::Unit> MyServiceWrapper::future_ping() {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyService_ping(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<std::unique_ptr<std::string>> MyServiceWrapper::future_getRandomData() {
  auto promise = std::make_shared<folly::Promise<std::unique_ptr<std::string>>>();
  call_cy_MyService_getRandomData(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<bool> MyServiceWrapper::future_hasDataById(
  int64_t id
) {
  auto promise = std::make_shared<folly::Promise<bool>>();
  call_cy_MyService_hasDataById(
    this->if_object,
    promise,
    id
  );
  return promise->getFuture();
}

folly::Future<std::unique_ptr<std::string>> MyServiceWrapper::future_getDataById(
  int64_t id
) {
  auto promise = std::make_shared<folly::Promise<std::unique_ptr<std::string>>>();
  call_cy_MyService_getDataById(
    this->if_object,
    promise,
    id
  );
  return promise->getFuture();
}

folly::Future<folly::Unit> MyServiceWrapper::future_putDataById(
  int64_t id,
  std::unique_ptr<std::string> data
) {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyService_putDataById(
    this->if_object,
    promise,
    id,
    data
  );
  return promise->getFuture();
}

folly::Future<folly::Unit> MyServiceWrapper::future_lobDataById(
  int64_t id,
  std::unique_ptr<std::string> data
) {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyService_lobDataById(
    this->if_object,
    promise,
    id,
    data
  );
  return promise->getFuture();
}

std::shared_ptr<apache::thrift::ServerInterface> MyServiceInterface(PyObject *if_object) {
  return std::make_shared<MyServiceWrapper>(if_object);
}


MyServiceFastWrapper::MyServiceFastWrapper(PyObject *obj)
  : if_object(obj)
  {
    Py_XINCREF(this->if_object);
  }

MyServiceFastWrapper::~MyServiceFastWrapper() {
    Py_XDECREF(this->if_object);
}

folly::Future<folly::Unit> MyServiceFastWrapper::future_ping() {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyServiceFast_ping(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<std::unique_ptr<std::string>> MyServiceFastWrapper::future_getRandomData() {
  auto promise = std::make_shared<folly::Promise<std::unique_ptr<std::string>>>();
  call_cy_MyServiceFast_getRandomData(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<bool> MyServiceFastWrapper::future_hasDataById(
  int64_t id
) {
  auto promise = std::make_shared<folly::Promise<bool>>();
  call_cy_MyServiceFast_hasDataById(
    this->if_object,
    promise,
    id
  );
  return promise->getFuture();
}

folly::Future<std::unique_ptr<std::string>> MyServiceFastWrapper::future_getDataById(
  int64_t id
) {
  auto promise = std::make_shared<folly::Promise<std::unique_ptr<std::string>>>();
  call_cy_MyServiceFast_getDataById(
    this->if_object,
    promise,
    id
  );
  return promise->getFuture();
}

folly::Future<folly::Unit> MyServiceFastWrapper::future_putDataById(
  int64_t id,
  std::unique_ptr<std::string> data
) {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyServiceFast_putDataById(
    this->if_object,
    promise,
    id,
    data
  );
  return promise->getFuture();
}

folly::Future<folly::Unit> MyServiceFastWrapper::future_lobDataById(
  int64_t id,
  std::unique_ptr<std::string> data
) {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_MyServiceFast_lobDataById(
    this->if_object,
    promise,
    id,
    data
  );
  return promise->getFuture();
}

std::shared_ptr<apache::thrift::ServerInterface> MyServiceFastInterface(PyObject *if_object) {
  return std::make_shared<MyServiceFastWrapper>(if_object);
}
} // namespace cpp2
