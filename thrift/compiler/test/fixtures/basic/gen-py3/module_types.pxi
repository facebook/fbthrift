from libcpp.memory cimport shared_ptr
from libcpp.string cimport string
from libc.stdint cimport int16_t, int32_t, int64_t

from module_types cimport (
  cMyStruct
)

cdef class MyStruct:
    cdef shared_ptr[cMyStruct] c_MyStruct

    def __init__(self, int64_t MyIntField, string MyStringField):
        self.c_MyStruct.get().MyIntField = MyIntField
        if MyStringField is not None:
            self.c_MyStruct.get().MyStringField = MyStringField.encode()

    @staticmethod
    cdef create(shared_ptr[cMyStruct] c_MyStruct):
        inst = <MyStruct>MyStruct.__new__(MyStruct)
        inst.c_MyStruct = c_MyStruct

        @property
        def MyIntField(self):
            return self.c_MyStruct.get().MyIntField

        @property
        def MyStringField(self):
            return self.c_MyStruct.get().MyStringField.decode()


