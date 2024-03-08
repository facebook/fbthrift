/**
 * Autogenerated by Thrift
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */

#pragma once
#if __has_include(<thrift/compiler/test/fixtures/interactions/gen-cpp2/MyService.h>)
#include <thrift/compiler/test/fixtures/interactions/gen-cpp2/MyService.h>
#else
#include <thrift/compiler/test/fixtures/interactions/gen-cpp2/module_handlers.h>
#endif
#include <folly/python/futures.h>
#include <Python.h>

#include <memory>

namespace cpp2 {

class MyServiceWrapper : virtual public MyServiceSvIf {
  protected:
    PyObject *if_object;
    folly::Executor *executor;
  public:
    explicit MyServiceWrapper(PyObject *if_object, folly::Executor *exc);
    void async_tm_foo(std::unique_ptr<apache::thrift::HandlerCallback<void>> callback) override;
    void async_tm_interact(std::unique_ptr<apache::thrift::HandlerCallback<void>> callback
        , int32_t arg
    ) override;
    void async_tm_interactFast(std::unique_ptr<apache::thrift::HandlerCallback<int32_t>> callback) override;
    void async_tm_serialize(std::unique_ptr<apache::thrift::HandlerCallback<apache::thrift::ResponseAndServerStream<int32_t,int32_t>>> callback) override;
    std::unique_ptr<MyInteractionIf> createMyInteraction() override;
    std::unique_ptr<MyInteractionFastIf> createMyInteractionFast() override;
    std::unique_ptr<SerialInteractionIf> createSerialInteraction() override;
folly::SemiFuture<folly::Unit> semifuture_onStartServing() override;
folly::SemiFuture<folly::Unit> semifuture_onStopRequested() override;
};

std::shared_ptr<apache::thrift::ServerInterface> MyServiceInterface(PyObject *if_object, folly::Executor *exc);
} // namespace cpp2
