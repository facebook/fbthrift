from cy_thrift_server cimport cServerInterface

cdef extern from "src/gen_cpp2/MyService.h":
    cdef cppclass cMyServiceSvAsyncIf "MyServiceSvAsyncIf":
      pass

    cdef cppclass cMyServiceSvIf "MyServiceSvIf"(cMyServiceSvAsyncIf, cServerInterface):
      pass

cdef extern from "src/gen_cpp2/MyServiceFast.h":
    cdef cppclass cMyServiceFastSvAsyncIf "MyServiceFastSvAsyncIf":
      pass

    cdef cppclass cMyServiceFastSvIf "MyServiceFastSvIf"(cMyServiceFastSvAsyncIf, cServerInterface):
      pass

