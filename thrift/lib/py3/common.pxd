from libcpp.map cimport map as cmap
from libc.stdint cimport int64_t
from libcpp.string cimport string
from thrift.py3.std_libcpp cimport milliseconds


cdef extern from "thrift/lib/cpp/concurrency/Thread.h":

    enum cPriority "apache::thrift::concurrency::PRIORITY":
        cHIGH_IMPORTANT "apache::thrift::concurrency::HIGH_IMPORTANT"
        cHIGH "apache::thrift::concurrency::HIGH"
        cIMPORTANT "apache::thrift::concurrency::IMPORTANT"
        cNORMAL "apache::thrift::concurrency::NORMAL"
        cBEST_EFFORT "apache::thrift::concurrency::BEST_EFFORT"
        cN_PRIORITIES "apache::thrift::concurrency::N_PRIORITIES"


cdef inline cPriority Priority_to_cpp(object value):
    cdef int cvalue = value.value
    return <cPriority>cvalue


cdef class Headers:
    cdef object __weakref__
    cdef const cmap[string, string]* _getMap(self)


cdef extern from "thrift/lib/cpp2/async/RequestChannel.h" namespace "apache::thrift":
    cdef cppclass cRpcOptions "apache::thrift::RpcOptions":
        cRpcOptions()
        cRpcOptions& setTimeout(milliseconds timeout)
        milliseconds getTimeout()
        cRpcOptions& setPriority(cPriority priority)
        cPriority getPriority()
        cRpcOptions& setChunkTimeout(milliseconds timeout)
        milliseconds getChunkTimeout()
        cRpcOptions& setQueueTimeout(milliseconds timeout)
        milliseconds getQueueTimeout()
        void setWriteHeader(const string& key, const string value)
        const cmap[string, string]& getReadHeaders()
        const cmap[string, string]& getWriteHeaders()


cdef class RpcOptions:
    cdef object __weakref__
    cdef object _readheaders
    cdef object _writeheaders
    cdef cRpcOptions _cpp_obj


cdef class ReadHeaders(Headers):
    cdef RpcOptions _parent
    @staticmethod
    cdef create(RpcOptions rpc_options)


cdef class WriteHeaders(Headers):
    cdef RpcOptions _parent
    @staticmethod
    cdef create(RpcOptions rpc_options)
