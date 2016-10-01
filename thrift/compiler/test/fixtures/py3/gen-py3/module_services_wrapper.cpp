#include <src/gen-py3/module_services_wrapper.h>
#include <src/gen-py3/module_server.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace py3 {
namespace simple {

SimpleServiceWrapper::SimpleServiceWrapper(PyObject *obj)
  : if_object(obj)
  {
    Py_XINCREF(this->if_object);
  }

SimpleServiceWrapper::~SimpleServiceWrapper() {
    Py_XDECREF(this->if_object);
}

folly::Future<int32_t> SimpleServiceWrapper::future_get_five() {
  auto promise = std::make_shared<folly::Promise<int32_t>>();
  call_cy_SimpleService_get_five(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<int32_t> SimpleServiceWrapper::future_add_five(
  int32_t num
) {
  auto promise = std::make_shared<folly::Promise<int32_t>>();
  call_cy_SimpleService_add_five(
    this->if_object,
    promise,
    num
  );
  return promise->getFuture();
}

folly::Future<folly::Unit> SimpleServiceWrapper::future_do_nothing() {
  auto promise = std::make_shared<folly::Promise<folly::Unit>>();
  call_cy_SimpleService_do_nothing(
    this->if_object,
    promise
  );
  return promise->getFuture();
}

folly::Future<std::unique_ptr<std::string>> SimpleServiceWrapper::future_concat(
  std::unique_ptr<std::string> first,
  std::unique_ptr<std::string> second
) {
  auto promise = std::make_shared<folly::Promise<std::unique_ptr<std::string>>>();
  call_cy_SimpleService_concat(
    this->if_object,
    promise,
    std::move(first),
    std::move(second)
  );
  return promise->getFuture();
}

folly::Future<int32_t> SimpleServiceWrapper::future_get_value(
  std::unique_ptr<py3::simple::SimpleStruct> simple_struct
) {
  auto promise = std::make_shared<folly::Promise<int32_t>>();
  call_cy_SimpleService_get_value(
    this->if_object,
    promise,
    std::move(simple_struct)
  );
  return promise->getFuture();
}

std::shared_ptr<apache::thrift::ServerInterface> SimpleServiceInterface(PyObject *if_object) {
  return std::make_shared<SimpleServiceWrapper>(if_object);
}
} // namespace py3
} // namespace simple
