#pragma once
#include <src/gen-cpp2/SimpleService.h>
#include <Python.h>
#include <memory.h>

namespace py3 {
namespace simple {

class SimpleServiceWrapper : public SimpleServiceSvIf {
  private:
    PyObject *if_object;
  public:
    explicit SimpleServiceWrapper(PyObject *if_object);
    virtual ~SimpleServiceWrapper();
      folly::Future<int32_t> future_get_five();
      folly::Future<int32_t> future_add_five(
        int32_t num
      );
      folly::Future<folly::Unit> future_do_nothing();
      folly::Future<std::unique_ptr<std::string>> future_concat(
        std::unique_ptr<std::string> first,
        std::unique_ptr<std::string> second
      );
      folly::Future<int32_t> future_get_value(
        std::unique_ptr<py3::simple::SimpleStruct> simple_struct
      );
};

std::shared_ptr<apache::thrift::ServerInterface> SimpleServiceInterface(PyObject *if_object);
} // namespace py3
} // namespace simple
