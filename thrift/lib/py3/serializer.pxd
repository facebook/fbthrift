# distutils: language = c++

from libcpp.string cimport string
from libc.stdint cimport uint32_t
from folly.iobuf cimport cIOBuf, cIOBufQueue


cdef extern from "thrift/lib/cpp2/protocol/Protocol.h":
    enum cExternalBufferSharing "apache::thrift::ExternalBufferSharing":
        COPY_EXTERNAL_BUFFER "apache::thrift::COPY_EXTERNAL_BUFFER"
        SHARE_EXTERNAL_BUFFER "apache::thrift::SHARE_EXTERNAL_BUFFER"


cdef extern from "<thrift/lib/cpp2/protocol/Serializer.h>" nogil:
    void CompactSerialize "apache::thrift::CompactSerializer::serialize"[T](const T& obj, cIOBufQueue* out, cExternalBufferSharing) except+
    uint32_t CompactDeserialize "apache::thrift::CompactSerializer::deserialize"[T](const cIOBuf* buf, T& obj, cExternalBufferSharing) except+
    void BinarySerialize "apache::thrift::BinarySerializer::serialize"[T](const T& obj, cIOBufQueue* out, cExternalBufferSharing) except+
    uint32_t BinaryDeserialize "apache::thrift::BinarySerializer::deserialize"[T](const cIOBuf* buf, T& obj, cExternalBufferSharing) except+
    void JSONSerialize "apache::thrift::SimpleJSONSerializer::serialize"[T](const T& obj, cIOBufQueue* out, cExternalBufferSharing) except+
    uint32_t JSONDeserialize "apache::thrift::SimpleJSONSerializer::deserialize"[T](const cIOBuf* buf, T& obj, cExternalBufferSharing) except+
    void CompactJSONSerialize "apache::thrift::JSONSerializer::serialize"[T](const T& obj, cIOBufQueue* out, cExternalBufferSharing) except+
    uint32_t CompactJSONDeserialize "apache::thrift::JSONSerializer::deserialize"[T](const cIOBuf* buf, T& obj, cExternalBufferSharing) except+
