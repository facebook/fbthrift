from libcpp.memory cimport shared_ptr, make_shared
from libcpp.string cimport string
from libc.stdint cimport int16_t, int32_t, int64_t
from cython.operator cimport dereference as deref

from module_types cimport (
    move,
    cSimpleStruct
)

cdef class SimpleStruct:
    cdef shared_ptr[cSimpleStruct] c_SimpleStruct

    def __init__(self, int key, int value):
        deref(self.c_SimpleStruct).key = key
        deref(self.c_SimpleStruct).value = value

    @staticmethod
    cdef create(shared_ptr[cSimpleStruct] c_SimpleStruct):
        inst = <SimpleStruct>SimpleStruct.__new__(SimpleStruct)
        inst.c_SimpleStruct = c_SimpleStruct
        return inst

    @property
    def key(self):
        return self.c_SimpleStruct.get().key

    @property
    def value(self):
        return self.c_SimpleStruct.get().value


