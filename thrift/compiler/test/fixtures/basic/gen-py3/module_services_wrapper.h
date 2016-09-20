#pragma once
#include <src/gen-cpp2/MyService.h>
#include <src/gen-cpp2/MyServiceFast.h>
#include <Python.h>
#include <memory.h>

namespace cpp2 {

class MyServiceWrapper : public MyServiceSvAsyncIf {
  private:
    PyObject *if_object;
  public:
    explicit MyServiceWrapper(PyObject *if_object);
    virtual ~MyServiceWrapper();
      folly::Future<folly::Unit> future_ping();
      folly::Future<std::unique_ptr<std::string>> future_getRandomData();
      folly::Future<bool> future_hasDataById(
        int64_t id
      );
      folly::Future<std::unique_ptr<std::string>> future_getDataById(
        int64_t id
      );
      folly::Future<folly::Unit> future_putDataById(
        int64_t id,
        std::unique_ptr<std::string> data
      );
      folly::Future<folly::Unit> future_lobDataById(
        int64_t id,
        std::unique_ptr<std::string> data
      );
};

std::shared_ptr<ServerInterface> MyServiceInterface(PyObject *if_object);


class MyServiceFastWrapper : public MyServiceFastSvAsyncIf {
  private:
    PyObject *if_object;
  public:
    explicit MyServiceFastWrapper(PyObject *if_object);
    virtual ~MyServiceFastWrapper();
      folly::Future<folly::Unit> future_ping();
      folly::Future<std::unique_ptr<std::string>> future_getRandomData();
      folly::Future<bool> future_hasDataById(
        int64_t id
      );
      folly::Future<std::unique_ptr<std::string>> future_getDataById(
        int64_t id
      );
      folly::Future<folly::Unit> future_putDataById(
        int64_t id,
        std::unique_ptr<std::string> data
      );
      folly::Future<folly::Unit> future_lobDataById(
        int64_t id,
        std::unique_ptr<std::string> data
      );
};

std::shared_ptr<ServerInterface> MyServiceFastInterface(PyObject *if_object);
} // namespace cpp2
