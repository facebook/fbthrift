from libcpp.string cimport string
from libc.stdint cimport int16_t, int32_t, int64_t

cdef extern from "src/gen-cpp2/module_types.h":
    cdef cppclass cMyStruct "MyStruct":
        cMyStruct() except +
        int64_t MyIntField
        string MyStringField

