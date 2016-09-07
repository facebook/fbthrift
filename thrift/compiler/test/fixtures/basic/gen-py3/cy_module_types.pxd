
from libcpp.string cimport string
from libc.stdint cimport int16_t, int32_t, int64_t



cdef extern from "src/gen_cpp2/module_types.h":
    cdef cppclass cMyStruct "MyStruct":
        int64_t MyIntField
        string MyStringField
