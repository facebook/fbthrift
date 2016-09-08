#include <src/gen_py3/module_service_wrapper.h>


MyServiceWrapper::MyServiceWrapper(PyObject *obj)
  : if_object(obj)
  {
    Py_XINCREF(this->if_object);
  }

MyServiceWrapper::~MyServiceWrapper() {
    Py_XDECREF(this->if_object);
}

std::shared_ptr<ServerInterface> MyServiceInterface(PyObject *if_object);


MyServiceFastWrapper::MyServiceFastWrapper(PyObject *obj)
  : if_object(obj)
  {
    Py_XINCREF(this->if_object);
  }

MyServiceFastWrapper::~MyServiceFastWrapper() {
    Py_XDECREF(this->if_object);
}

std::shared_ptr<ServerInterface> MyServiceFastInterface(PyObject *if_object);
