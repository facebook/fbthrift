/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#pragma once
#if __has_include(<thrift/compiler/test/fixtures/single_file_service/gen-cpp2/A.h>)
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/A.h>
#else
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/module_handlers.h>
#endif
#if __has_include(<thrift/compiler/test/fixtures/single_file_service/gen-cpp2/B.h>)
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/B.h>
#else
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/module_handlers.h>
#endif
#if __has_include(<thrift/compiler/test/fixtures/single_file_service/gen-cpp2/C.h>)
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/C.h>
#else
#include <thrift/compiler/test/fixtures/single_file_service/gen-cpp2/module_handlers.h>
#endif
#include <folly/python/futures.h>
#include <Python.h>

#include <memory>

namespace cpp2 {

class AWrapper : virtual public ASvIf {
  protected:
    PyObject *if_object;
    folly::Executor *executor;
  public:
    explicit AWrapper(PyObject *if_object, folly::Executor *exc);
    void async_tm_foo(std::unique_ptr<apache::thrift::HandlerCallback<std::unique_ptr<::cpp2::Foo>>> callback) override;
    std::unique_ptr<IIf> createI() override;
folly::SemiFuture<folly::Unit> semifuture_onStartServing() override;
folly::SemiFuture<folly::Unit> semifuture_onStopRequested() override;
};

std::shared_ptr<apache::thrift::ServerInterface> AInterface(PyObject *if_object, folly::Executor *exc);


class BWrapper : public ::cpp2::AWrapper, virtual public BSvIf {
  public:
    explicit BWrapper(PyObject *if_object, folly::Executor *exc);
    void async_tm_bar(std::unique_ptr<apache::thrift::HandlerCallback<void>> callback
        , std::unique_ptr<::cpp2::Foo> foo
    ) override;
    void async_tm_stream_stuff(std::unique_ptr<apache::thrift::HandlerCallback<apache::thrift::ServerStream<int32_t>>> callback) override;
folly::SemiFuture<folly::Unit> semifuture_onStartServing() override;
folly::SemiFuture<folly::Unit> semifuture_onStopRequested() override;
};

std::shared_ptr<apache::thrift::ServerInterface> BInterface(PyObject *if_object, folly::Executor *exc);


class CWrapper : virtual public CSvIf {
  protected:
    PyObject *if_object;
    folly::Executor *executor;
  public:
    explicit CWrapper(PyObject *if_object, folly::Executor *exc);
    std::unique_ptr<IIf> createI() override;
folly::SemiFuture<folly::Unit> semifuture_onStartServing() override;
folly::SemiFuture<folly::Unit> semifuture_onStopRequested() override;
};

std::shared_ptr<apache::thrift::ServerInterface> CInterface(PyObject *if_object, folly::Executor *exc);
} // namespace cpp2