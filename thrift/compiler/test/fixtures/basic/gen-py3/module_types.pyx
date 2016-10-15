from libcpp.memory cimport shared_ptr, make_shared, unique_ptr, make_unique
from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from cython.operator cimport dereference as deref
from thrift.lib.py3.thrift_server cimport TException

from module_types cimport (
    cMyStruct
)

cdef class MyStruct:

    def __init__(
        self,
        int MyIntField,
        str MyStringField
    ):
        self.c_MyStruct = make_shared[cMyStruct]()
        deref(self.c_MyStruct).MyIntField = MyIntField
        if MyStringField is not None:
            deref(self.c_MyStruct).MyStringField = make_shared[string](<string> MyStringField.encode('UTF-8'))
        
    @staticmethod
    cdef create(shared_ptr[cMyStruct] c_MyStruct):
        inst = <MyStruct>MyStruct.__new__(MyStruct)
        inst.c_MyStruct = c_MyStruct
        return inst

    @property
    def MyIntField(self):
        return self.c_MyStruct.get().MyIntField

    @property
    def MyStringField(self):
        return self.c_MyStruct.get().MyStringField.decode()


