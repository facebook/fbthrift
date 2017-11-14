from libcpp.string cimport string
from libc.stdint cimport uint32_t
from folly.iobuf cimport IOBuf

cdef extern from "<thrift/lib/cpp2/protocol/Serializer.h>" nogil:
    void CompactSerialize "apache::thrift::CompactSerializer::serialize"[T](const T& obj, string* out) except+
    uint32_t CompactDeserialize "apache::thrift::CompactSerializer::deserialize"[T](const IOBuf* buf, T& obj) except+
    void BinarySerialize "apache::thrift::BinarySerializer::serialize"[T](const T& obj, string* out) except+
    uint32_t BinaryDeserialize "apache::thrift::BinarySerializer::deserialize"[T](const IOBuf* buf, T& obj) except+
    void JSONSerialize "apache::thrift::SimpleJSONSerializer::serialize"[T](const T& obj, string* out) except+
    uint32_t JSONDeserialize "apache::thrift::SimpleJSONSerializer::deserialize"[T](const IOBuf* buf, T& obj) except+
