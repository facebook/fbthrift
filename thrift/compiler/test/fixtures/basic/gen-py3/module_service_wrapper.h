#pragma once
#include <src/gen_cpp2/MyService.h>
#include <src/gen_cpp2/MyServiceFast.h>
#include <Python.h>
#include <memory.h>


class MyServiceWrapper : public MyServiceSvAsyncIf {
  private:
    PyObject *if_object;
  public:
    explicit MyServiceWrapper(PyObject *if_object);
    virtual ~MyServiceWrapper();
}

std::shared_ptr<ServerInterface> MyServiceInterface(PyObject *if_object);


class MyServiceFastWrapper : public MyServiceFastSvAsyncIf {
  private:
    PyObject *if_object;
  public:
    explicit MyServiceFastWrapper(PyObject *if_object);
    virtual ~MyServiceFastWrapper();
}

std::shared_ptr<ServerInterface> MyServiceFastInterface(PyObject *if_object);
