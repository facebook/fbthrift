from py3_thrift_server cimport cServerInterface

cdef extern from "src/gen-cpp2/SimpleService.h" namespace "py3::simple":
    cdef cppclass cSimpleServiceSvAsyncIf "py3::simple::SimpleServiceSvAsyncIf":
      pass

    cdef cppclass cSimpleServiceSvIf "py3::simple::SimpleServiceSvIf"(cSimpleServiceSvAsyncIf, cServerInterface):
      pass

