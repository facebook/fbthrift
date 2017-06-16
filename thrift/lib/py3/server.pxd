from libc.stdint cimport uint16_t, uint64_t
from libc.string cimport const_uchar
from libcpp.string cimport string
from libcpp.memory cimport shared_ptr, unique_ptr

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
        void setInterface(shared_ptr[cServerInterface]) nogil
        void serve() nogil except +
        void stop() nogil except +
        void setSSLPolicy(cSSLPolicy policy) nogil


cdef extern from "folly/io/IOBuf.h" namespace "folly":
    cdef cppclass IOBuf:
        uint64_t length()
        const_uchar* data()

cdef extern from "folly/ssl/OpenSSLCertUtils.h":
    # I need a opque id for x509 structs
    cdef cppclass X509:
        pass

cdef extern from '<utility>' namespace 'std':
    unique_ptr[IOBuf] move(unique_ptr[IOBuf])

cdef extern from "folly/ssl/OpenSSLCertUtils.h" \
        namespace "folly::ssl::OpenSSLCertUtils":
    unique_ptr[IOBuf] derEncode(X509& cert)

cdef extern from "thrift/lib/cpp2/server/Cpp2ConnContext.h" \
        namespace "apache::thrift":

    cdef cppclass cfollySocketAddress "folly::SocketAddress":
        uint16_t getPort()
        bint isFamilyInet()
        string getAddressStr()
        string getPath()

    cdef cppclass Cpp2ConnContext:
        bint isTls()
        string getPeerCommonName()
        shared_ptr[X509] getPeerCertificate()
        cfollySocketAddress* getPeerAddress()

    cdef cppclass Cpp2RequestContext:
        Cpp2ConnContext* getConnectionContext()


cdef class ServiceInterface:
    cdef shared_ptr[cServerInterface] interface_wrapper


cdef class ThriftServer:
    cdef unique_ptr[cThriftServer] server
    cdef ServiceInterface handler
    cdef object loop


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
