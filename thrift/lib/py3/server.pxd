# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from libc.stdint cimport uint16_t, int32_t, uint32_t, int64_t
from libcpp.string cimport string
from libcpp cimport bool as cbool
from libcpp.map cimport map
from libcpp.memory cimport shared_ptr, unique_ptr
from folly.iobuf cimport cIOBuf
from folly.range cimport StringPiece
from folly cimport cFollyExecutor
from cpython.ref cimport PyObject
from thrift.py3.common cimport cPriority, Priority_to_cpp, Headers, cThriftMetadata
from thrift.py3.std_libcpp cimport milliseconds, seconds


cdef extern from "thrift/lib/py3/server.h" namespace "::thrift::py3":
    cdef cppclass cfollySocketAddress "folly::SocketAddress":
        uint16_t getPort()
        bint isFamilyInet()
        bint empty()
        string getAddressStr()
        string getPath()

    cfollySocketAddress makeFromPath "folly::SocketAddress::makeFromPath"(StringPiece path)

    cdef cppclass AddressHandler:  # This isn't true but its easier for cython
        pass

    AddressHandler object_partial(void(*)(PyObject*, cfollySocketAddress), PyObject*)

    cdef cppclass Py3ServerEventHandler:
        Py3ServerEventHandler(cFollyExecutor*, AddressHandler) nogil

    cdef cppclass cIsOverloadedFunc "apache::thrift::IsOverloadedFunc":
        pass

cdef extern from "thrift/lib/cpp2/async/AsyncProcessor.h" \
        namespace "apache::thrift":
    cdef cppclass cAsyncProcessor "apache::thrift::AsyncProcessor":
        pass

    cdef cppclass cGeneratedAsyncProcessor "apache::thrift::GeneratedAsyncProcessor"(cAsyncProcessor):
        const char* getServiceName()

    cdef cppclass cAsyncProcessorFactory \
            "apache::thrift::AsyncProcessorFactory":
        unique_ptr[cAsyncProcessor] getProcessor()

    cdef cppclass cServerInterface \
            "apache::thrift::ServerInterface"(cAsyncProcessorFactory):
        pass

    cdef cGeneratedAsyncProcessor* dynamic_cast_gen "dynamic_cast<apache::thrift::GeneratedAsyncProcessor*>"(...)

cdef extern from "thrift/lib/cpp2/server/TransportRoutingHandler.h" \
        namespace "apache::thrift":
    cdef cppclass cTransportRoutingHandler "apache::thrift::TransportRoutingHandler":
        pass

cdef extern from "thrift/lib/cpp2/server/ThriftServer.h" \
        namespace "apache::thrift":

    cdef cppclass cSSLPolicy "apache::thrift::SSLPolicy":
        bint operator==(cSSLPolicy&)

    cSSLPolicy SSLPolicy__DISABLED "apache::thrift::SSLPolicy::DISABLED"
    cSSLPolicy SSLPolicy__PERMITTED "apache::thrift::SSLPolicy::PERMITTED"
    cSSLPolicy SSLPolicy__REQUIRED "apache::thrift::SSLPolicy::REQUIRED"

    cdef cppclass cBaseThriftServerMetadata "apache::thrift::BaseThriftServer::Metadata":
        string wrapper
        string languageFramework

    cdef cppclass cThriftServer "apache::thrift::ThriftServer":
        ThriftServer() nogil except +
        void setPort(uint16_t port) nogil
        uint16_t getPort() nogil
        void setAddress(cfollySocketAddress& addr) nogil
        void setAddress(string ip, uint16_t port) nogil
        void setInterface(shared_ptr[cServerInterface]) nogil
        void setProcessorFactory(shared_ptr[cAsyncProcessorFactory]) nogil
        void serve() nogil except +
        void stop() nogil except +
        void stopListening() nogil except +
        cSSLPolicy getSSLPolicy() nogil
        void setSSLPolicy(cSSLPolicy policy) nogil
        void setServerEventHandler(shared_ptr[Py3ServerEventHandler] handler) nogil
        int32_t getActiveRequests()
        uint32_t getMaxRequests()
        void setMaxRequests(uint32_t maxRequests)
        uint32_t getMaxConnections()
        void setMaxConnections(uint32_t maxConnections)
        int getListenBacklog()
        void setListenBacklog(int listenBacklog)
        void setNumIOWorkerThreads(uint32_t numIOWorkerThreads)
        uint32_t getNumIOWorkerThreads()
        void setNumCPUWorkerThreads(uint32_t numCPUWorkerThreads)
        uint32_t getNumCPUWorkerThreads()
        void setWorkersJoinTimeout(seconds timeout)
        void setAllowPlaintextOnLoopback(cbool allow)
        cbool isPlaintextAllowedOnLoopback()
        void setIdleTimeout(milliseconds idleTimeout)
        milliseconds getIdleTimeout()
        void setQueueTimeout(milliseconds timeout)
        milliseconds getQueueTimeout()
        void setIsOverloaded(cIsOverloadedFunc isOverloaded)
        void useExistingSocket(int socket) except +
        cBaseThriftServerMetadata& metadata()
        void setThreadManagerFromExecutor(cFollyExecutor*)
        void setStopWorkersOnStopListening(cbool stopWorkers)
        cbool getStopWorkersOnStopListening()

cdef extern from "folly/ssl/OpenSSLCertUtils.h":
    # I need a opque id for x509 structs
    cdef cppclass X509:
        pass

cdef extern from "folly/ssl/OpenSSLCertUtils.h" \
        namespace "folly::ssl::OpenSSLCertUtils":
    unique_ptr[cIOBuf] derEncode(X509& cert)

cdef extern from "thrift/lib/cpp/transport/THeader.h" namespace "apache::thrift":
    cdef cppclass THeader:
        const map[string, string]& getWriteHeaders()
        const map[string, string]& getHeaders()
        void setHeader(string& key, string& value)

cdef extern from "thrift/lib/cpp2/server/Cpp2ConnContext.h" \
        namespace "apache::thrift":

    cdef cppclass Cpp2ConnContext:
        string getSecurityProtocol()
        string getPeerCommonName()
        shared_ptr[X509] getPeerCertificate()
        cfollySocketAddress* getPeerAddress()
        cfollySocketAddress* getLocalAddress()

    cdef cppclass Cpp2RequestContext:
        Cpp2ConnContext* getConnectionContext()
        cPriority getCallPriority()
        THeader* getHeader()
        string getMethodName()


cdef class AsyncProcessorFactory:
    cdef shared_ptr[cAsyncProcessorFactory] _cpp_obj


cdef class ServiceInterface(AsyncProcessorFactory):
    pass


cdef class ThriftServer:
    cdef shared_ptr[cThriftServer] server
    cdef AsyncProcessorFactory factory
    cdef object loop
    cdef object address_future
    cdef void set_is_overloaded(self, cIsOverloadedFunc is_overloaded)


cdef class ConnectionContext:
    cdef Cpp2ConnContext* _ctx
    cdef object _peer_address
    cdef object _local_address

    @staticmethod
    cdef ConnectionContext create(Cpp2ConnContext* ctx)


cdef class RequestContext:
    cdef ConnectionContext _c_ctx
    cdef Cpp2RequestContext* _ctx
    cdef object _readheaders
    cdef object _writeheaders

    @staticmethod
    cdef RequestContext create(Cpp2RequestContext* ctx)


cdef class ReadHeaders(Headers):
    cdef RequestContext _parent
    @staticmethod
    cdef create(RequestContext ctx)


cdef class WriteHeaders(Headers):
    cdef RequestContext _parent
    @staticmethod
    cdef create(RequestContext ctx)

cdef extern from "<utility>" namespace "std" nogil:
    cdef unique_ptr[cTransportRoutingHandler] move(unique_ptr[cTransportRoutingHandler])

cdef object THRIFT_REQUEST_CONTEXT
