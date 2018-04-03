from libc.stdint cimport uint16_t, int32_t, uint32_t
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr, unique_ptr
from folly.iobuf cimport IOBuf
from folly.range cimport StringPiece
from folly cimport cFollyExecutor
from cpython.ref cimport PyObject

cdef extern from "thrift/lib/py3/server.h" namespace "thrift::py3":
    cdef cppclass cfollySocketAddress "folly::SocketAddress":
        uint16_t getPort()
        bint isFamilyInet()
        string getAddressStr()
        string getPath()

    cfollySocketAddress makeFromPath "folly::SocketAddress::makeFromPath"(StringPiece path)

    cdef cppclass AddressHandler:  # This isn't true but its easier for cython
        pass

    AddressHandler object_partial(void(*)(PyObject*, cfollySocketAddress), PyObject*)

    cdef cppclass Py3ServerEventHandler:
        Py3ServerEventHandler(cFollyExecutor*, AddressHandler) nogil

cdef extern from "thrift/lib/cpp2/async/AsyncProcessor.h" \
        namespace "apache::thrift":
    cdef cppclass cAsyncProcessorFactory \
            "apache::thrift::AsyncProcessorFactory":
        pass

    cdef cppclass cServerInterface \
            "apache::thrift::ServerInterface"(cAsyncProcessorFactory):
        pass

cdef extern from "thrift/lib/cpp2/server/ThriftServer.h" \
        namespace "apache::thrift":

    cdef cppclass cSSLPolicy "apache::thrift::SSLPolicy":
        bint operator==(cSSLPolicy&)

    cSSLPolicy SSLPolicy__DISABLED "apache::thrift::SSLPolicy::DISABLED"
    cSSLPolicy SSLPolicy__PERMITTED "apache::thrift::SSLPolicy::PERMITTED"
    cSSLPolicy SSLPolicy__REQUIRED "apache::thrift::SSLPolicy::REQUIRED"

    cdef cppclass cThriftServer "apache::thrift::ThriftServer":
        ThriftServer() nogil except +
        void setPort(uint16_t port) nogil
        void setAddress(cfollySocketAddress& addr) nogil
        void setAddress(string ip, uint16_t port) nogil
        void setInterface(shared_ptr[cServerInterface]) nogil
        void serve() nogil except +
        void stop() nogil except +
        void setSSLPolicy(cSSLPolicy policy) nogil
        void setServerEventHandler(shared_ptr[Py3ServerEventHandler] handler) nogil
        int32_t getActiveRequests()
        uint32_t getMaxRequests()
        void setMaxRequests(uint32_t maxRequests)
        uint32_t getMaxConnections()
        void setMaxConnections(uint32_t maxConnections)
        int getListenBacklog()
        void setListenBacklog(int listenBacklog)

cdef extern from "folly/ssl/OpenSSLCertUtils.h":
    # I need a opque id for x509 structs
    cdef cppclass X509:
        pass

cdef extern from "folly/ssl/OpenSSLCertUtils.h" \
        namespace "folly::ssl::OpenSSLCertUtils":
    unique_ptr[IOBuf] derEncode(X509& cert)

cdef extern from "thrift/lib/cpp2/server/Cpp2ConnContext.h" \
        namespace "apache::thrift":

    cdef cppclass Cpp2ConnContext:
        string getSecurityProtocol()
        string getPeerCommonName()
        shared_ptr[X509] getPeerCertificate()
        cfollySocketAddress* getPeerAddress()

    cdef cppclass Cpp2RequestContext:
        Cpp2ConnContext* getConnectionContext()


cdef class ServiceInterface:
    cdef shared_ptr[cServerInterface] interface_wrapper


cdef class ThriftServer:
    cdef shared_ptr[cThriftServer] server
    cdef ServiceInterface handler
    cdef object loop
    cdef object address_future


cdef class ConnectionContext:
    cdef Cpp2ConnContext* _ctx
    cdef object _peer_address

    @staticmethod
    cdef ConnectionContext create(Cpp2ConnContext* ctx)


cdef class RequestContext:
    cdef ConnectionContext _c_ctx
    cdef Cpp2RequestContext* _ctx

    @staticmethod
    cdef RequestContext create(Cpp2RequestContext* ctx)
