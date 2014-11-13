# cython: profile=False
from libc.stdint cimport (int64_t, uint32_t, int32_t, int16_t, int8_t, uint8_t,
                          INT8_MIN, INT8_MAX, INT16_MIN, INT16_MAX,
                          INT32_MIN, INT32_MAX, INT64_MIN, INT64_MAX)
from libcpp.string cimport string
from libcpp cimport bool
from cpython cimport PY_MAJOR_VERSION
from libc.string cimport memcpy
from cpython.ref cimport PyObject, PyTypeObject
import logging

cdef extern from * namespace 'std' nogil:
    cdef cppclass shared_ptr[T]:
        shared_ptr()
        shared_ptr(T*)
        T* get()

cdef extern from 'thrift/lib/cpp/protocol/TProtocol.h' namespace 'apache::thrift::protocol' nogil:
    enum TType:
        T_STOP   = 1
        T_VOID   = 1
        T_BOOL   = 2
        T_I08    = 3
        T_I16    = 6
        T_I32    = 8
        T_I64    = 10
        T_DOUBLE = 4
        T_STRING = 11
        T_STRUCT = 12
        T_MAP    = 13
        T_SET    = 14
        T_LIST   = 15
        T_FLOAT  = 19
    enum TMessageType: pass


cdef extern from 'thrift/lib/cpp/transport/TCallbackTransport.h' namespace 'apache::thrift::transport':
    ctypedef uint32_t (*ReadCallback)(void* userdata, uint8_t *buf, uint32_t length)
    cdef cppclass TCallbackTransport:
        TCallbackTransport()
        void setReadCallback(ReadCallback read_callback, void* userdata)

cdef extern from 'thrift/lib/cpp/transport/TBufferTransports.h' namespace 'apache::thrift::transport' nogil:
    cdef cppclass TTransport:
        TTransport()

    cdef cppclass TMemoryBuffer:
        TMemoryBuffer()
        string getBufferAsString()
        uint32_t getBufferSize()

cdef extern from 'thrift/lib/cpp/protocol/TCompactProtocol.h' namespace 'apache::thrift::protocol' nogil:
    cdef cppclass TCompactProtocolT[T]:
        TCompactProtocolT(T*)

cdef extern from 'thrift/lib/cpp/protocol/TBinaryProtocol.h' namespace 'apache::thrift::protocol' nogil:
    cdef cppclass TBinaryProtocolT[T]:
        TBinaryProtocolT(T*)

cdef extern from 'thrift/lib/cpp/protocol/TProtocol.h' namespace 'apache::thrift::protocol' nogil:
    cdef cppclass TProtocol:
        uint32_t writeMessageBegin(const string& name,
                                   const TMessageType messageType,
                                   const int32_t seqid) except +
        uint32_t writeMessageEnd()
        uint32_t writeStructBegin(const char* name) except +
        uint32_t writeStructEnd()
        uint32_t writeFieldBegin(const char* name,
                                 const TType fieldType,
                                 const int16_t fieldId) except +

        uint32_t writeFieldEnd()
        uint32_t writeFieldStop()
        uint32_t writeListBegin(const TType elemType,
                                const uint32_t size) except +
        uint32_t writeListEnd()
        uint32_t writeSetBegin(const TType elemtype,
                               const uint32_t size) except +
        uint32_t writeSetEnd()
        uint32_t writeMapBegin(const TType keyType,
                               const TType valType,
                               const uint32_t size) except +
        uint32_t writeMapEnd()
        uint32_t writeBool(const bool value) except +
        uint32_t writeByte(const int8_t byte) except +
        uint32_t writeI16(const int16_t i16) except +
        uint32_t writeI32(const int32_t i32) except +
        uint32_t writeI64(const int64_t i64) except +
        uint32_t writeDouble(const double dub) except +
        uint32_t writeFloat(const float flt) except +
        uint32_t writeString(const string& astr) except +
        uint32_t readMessageBegin(string& name, TMessageType& messageType,
                                  int32_t& sequid) except +
        uint32_t readMessageEnd()
        uint32_t readStructBegin(string& name) except +
        uint32_t readStructEnd()
        uint32_t readFieldBegin(string& name, TType& fieldType,
                                int16_t& fieldId) except +
        uint32_t readFieldEnd()
        uint32_t readMapBegin(TType& keyType, TType& valType, uint32_t& size,
                              bint& sizeUnknown) except +
        uint32_t readMapEnd()
        uint32_t readListBegin(TType& elemType, uint32_t& size,
                               bint& sizeUnkown) except +
        uint32_t readListEnd()
        uint32_t readSetBegin(TType& elemType, uint32_t& size,
                              bint& sizeUnkown) except +
        uint32_t readSetEnd()
        uint32_t readBool(bool& value) except +
        uint32_t readByte(int8_t& byte) except +
        uint32_t readI16(int16_t& i16) except +
        uint32_t readI32(int32_t& i32) except +
        uint32_t readI64(int64_t& i64) except +
        uint32_t readDouble(double& dub) except +
        uint32_t readFloat(float& flt) except +
        uint32_t readString(string& data) except +
        uint32_t skip(TType ttype) except +

cdef inline overflow_check(int64_t value, int64_t min, int64_t max):
    if value > max or value < min:
        raise OverflowError('{} is out of range ({}, {})'
                            .format(value, min, max))


cdef inline string to_string(value):
    cdef string c_str
    if not isinstance(value, bytes):
        c_str = value.encode('UTF-8')
    else:
        c_str = value
    return c_str


dictitems = dict.items if PY_MAJOR_VERSION >= 3 else dict.iteritems

# Thrift Spec List of Tuples
# field_key, thrift_type, name, type_args, default_value, Field Req Type)
# Not sure if it matters to us.
# Field Req Type is T_REQUIRED = 0, T_OPTIONAL = 1, T_OPT_IN_REQ_OUT = 2

# Type_args contains
# for structs (Class, Thrift_spec) or
# for maps k:y (Thrift_Type, type_args, Thrift_type, type_args) recursive
# or for set/list (Thrift_type, type_args)  recursive

cdef const char* EMPTY = ''


cdef inline encode_struct(TProtocol* proto, tuple type_args, value):
    _, thrift_spec = type_args
    proto.writeStructBegin(EMPTY)
    cdef tuple field
    cdef int16_t field_id
    cdef TType field_type
    cdef object field_type_args
    for field_id in range(len(thrift_spec)):  # Loop through the struct
        field = thrift_spec[field_id]
        if not field:
            continue

        #Unpack the tuple
        (field_type, field_name, _field_type_args, field_default) = field[1:-1]
        if field_type == T_STRUCT:
            field_type_args = tuple(_field_type_args)
        else:
            field_type_args = _field_type_args

        field_value = getattr(value, field_name)
        if field_value is None or field_value == field_default:
            # Don't write empty fields
            continue

        proto.writeFieldBegin(EMPTY, field_type, field_id)
        encode_thing(proto, field_type, field_value, field_type_args)
        proto.writeFieldEnd()
    proto.writeFieldStop()
    proto.writeStructEnd()


cdef inline decode_struct(TProtocol* proto, output, tuple type_args,
                          bint decodeutf8=False):
    _, thrift_spec = type_args
    cdef string name
    cdef TType field_type = T_STOP
    cdef TType our_field_type = T_STOP
    cdef object field_type_args
    cdef tuple field_spec
    cdef int16_t field_id = 0
    cdef bint skip = False
    proto.readStructBegin(name)
    while True:
        skip = False
        proto.readFieldBegin(name, field_type, field_id)
        if field_type == T_STOP:
            break
        if field_id >= 0 and field_id < len(thrift_spec):
            field_spec = thrift_spec[field_id]
        else:
            field_spec = None

        if not field_spec:
            skip = True
        else:
            (_, our_field_type, field_name,
             _field_type_args, _, _) = field_spec
            if our_field_type == T_STRUCT:
                field_type_args = tuple(_field_type_args)
            else:
                field_type_args = _field_type_args
            if field_type == our_field_type:
                setattr(output, field_name,
                        decode_val(proto, field_type, field_type_args,
                                   decodeutf8))
            else:
                skip = True

        if skip and not <bint>proto.skip(field_type):
            raise TypeError('Bad fields in struct, unable to skip')
        proto.readFieldEnd()
    proto.readStructEnd()


cdef decode_val(TProtocol* proto, TType ttype, object type_args,
                bint decodeutf8=False):
    #We assign default values here because we want to suppress cython warnings
    cdef bool v_bool = False
    cdef int8_t v_byte = 0
    cdef int16_t v_i16 = 0
    cdef int32_t v_i32 = 0
    cdef int64_t v_i64 = 0
    cdef double v_double = 0.0
    cdef float v_float = 0.0
    cdef uint32_t i, size = 0
    cdef TType sub_ttype = T_STOP
    cdef TType val_ttype = T_STOP
    cdef object sub_typeargs
    cdef object val_typeargs
    cdef string v_string
    cdef object klass
    if ttype == T_BOOL:
        proto.readBool(v_bool)
        return v_bool
    elif ttype == T_I08:
        proto.readByte(v_byte)
        return v_byte
    elif ttype == T_I16:
        proto.readI16(v_i16)
        return v_i16
    elif ttype == T_I32:
        proto.readI32(v_i32)
        return v_i32
    elif ttype == T_I64:
        proto.readI64(v_i64)
        return v_i64
    elif ttype == T_DOUBLE:
        proto.readDouble(v_double)
        return v_double
    elif ttype == T_FLOAT:
        proto.readFloat(v_float)
        return v_float
    elif ttype == T_STRING:
        proto.readString(v_string)
        if decodeutf8 and type_args:
            try:
                return v_string.decode('UTF-8')
            except UnicodeDecodeError:
                pass
        return v_string
    elif ttype == T_LIST:
        proto.readListBegin(sub_ttype, size, v_bool)
        sub_ttype, sub_typeargs = type_args
        # should we check that the elemType == sub_ttype?
        alist = []
        for i in range(size):
            alist.append(decode_val(proto, sub_ttype, sub_typeargs,
                                    decodeutf8))
        proto.readListEnd()
        return alist
    elif ttype == T_SET:
        proto.readSetBegin(sub_ttype, size, v_bool)
        sub_ttype, sub_typeargs = type_args
        aset = set()
        for i in range(size):
            aset.add(decode_val(proto, sub_ttype, sub_typeargs, decodeutf8))
        proto.readSetEnd()
        return aset
    elif ttype == T_MAP:
        proto.readMapBegin(sub_ttype, val_ttype, size, v_bool)
        sub_ttype, sub_typeargs, val_type, val_typeargs = type_args
        amap = {}
        for i in range(size):
            key = decode_val(proto, sub_ttype, sub_typeargs, decodeutf8)
            amap[key] = decode_val(proto, val_ttype, val_typeargs,
                                   decodeutf8)
        proto.readMapEnd()
        return amap
    elif ttype == T_STRUCT:
        klass = type_args[0]
        output = klass()
        decode_struct(proto, output, type_args, decodeutf8)
        return output
    else:
        raise TypeError('Unexpected TType')


cdef encode_thing(TProtocol* proto, TType ttype, value,
                  object type_args):
    #Switch Statement (after translation that is)
    if ttype == T_BOOL:
        proto.writeBool(value)
    elif ttype == T_I08:
        overflow_check(value, INT8_MIN, INT8_MAX)
        proto.writeByte(value)
    elif ttype == T_I16:
        overflow_check(value, INT16_MIN, INT16_MAX)
        proto.writeI16(value)
    elif ttype == T_I32:
        overflow_check(value, INT32_MIN, INT32_MAX)
        proto.writeI32(value)
    elif ttype == T_I64:
        overflow_check(value, INT64_MIN, INT64_MAX)
        proto.writeI64(value)
    elif ttype == T_DOUBLE or ttype == T_FLOAT:
        if ttype == T_DOUBLE:
            proto.writeDouble(value)
        else:
            proto.writeFloat(value)
    elif ttype == T_STRING:
        proto.writeString(to_string(value))
    elif ttype == T_STRUCT:
        #No Cdefs in switch
        encode_struct(proto, type_args, value)
    elif ttype == T_LIST:
        elm_type, elm_type_args = type_args
        proto.writeListBegin(elm_type, len(value))
        for e in value:
            encode_thing(proto, elm_type, e, elm_type_args)
        proto.writeListEnd()
    elif ttype == T_SET:
        elm_type, elm_type_args = type_args
        proto.writeSetBegin(elm_type, len(value))
        for e in value:
            encode_thing(proto, elm_type, e, elm_type_args)
        proto.writeSetEnd()
    elif ttype == T_MAP:
        key_type, key_type_args, value_type, value_type_args = type_args
        proto.writeMapBegin(key_type, value_type, len(value))
        for k, v in dictitems(value):
            encode_thing(proto, key_type, k, key_type_args)
            encode_thing(proto, value_type, v, value_type_args)
        proto.writeMapEnd()
    else:
        raise TypeError('Unexpected TType')


# Stollen from cStringIO.c
cdef struct IOobject:
  Py_ssize_t ob_refcnt
  PyTypeObject *ob_type
  char *buf
  Py_ssize_t pos, string_size


# A rewrite for cython
cdef Py_ssize_t IO_cread_into(IOobject *io, uint8_t* buf, Py_ssize_t n):
    cdef char* output
    cdef Py_ssize_t l

    if not io.buf:
        return -1

    l = io.string_size - io.pos
    if n < 0 or n > l:
        n = l
        if n < 0:
            n = 0


    output = io.buf + io.pos
    io.pos += n
    memcpy(<void *>buf, <void *>output, n)
    return n


cdef struct ReadData:
    PyObject* io_obj
    PyObject* refill


cdef uint32_t read_callback(void* userdata, uint8_t* buf, uint32_t length):
    cdef ReadData* readdata = <ReadData *>userdata
    cdef bytes partial

    while True:
        rc = IO_cread_into(<IOobject *>readdata.io_obj, buf, length)
        if rc == length:
            return length
        elif rc == -1:
            return 0
        else:
            partial = (<char *>buf)[:rc]
            io_obj = (<object>readdata.refill)(partial, length)
            readdata.io_obj = <PyObject *>io_obj

cdef enum ProtocolID:
    T_BAD_PROTOCOL = -1
    T_BINARY_PROTOCOL = 0
    T_COMPACT_PROTOCOL = 2

cdef ProtocolID proto_type(py_proto):
    import thrift.protocol.TBinaryProtocol as pTBinaryProtocol
    import thrift.protocol.THeaderProtocol as pTHeaderProtocol
    import thrift.protocol.TCompactProtocol as pTCompactProtocol

    if isinstance(py_proto, pTBinaryProtocol.TBinaryProtocol):
        return T_BINARY_PROTOCOL
    elif isinstance(py_proto, pTCompactProtocol.TCompactProtocol):
        return T_COMPACT_PROTOCOL
    elif isinstance(py_proto, pTHeaderProtocol.THeaderProtocol):
        return py_proto.get_protocol_id()
    else:
        return T_BAD_PROTOCOL

def decode(py_proto, py_struct, type_args, utf8strings=False):
    cdef TTransport *trans = <TTransport *>new TCallbackTransport()
    cdef ReadData readdata
    readdata.io_obj = <PyObject *>py_proto.trans.cstringio_buf
    readdata.refill = <PyObject *>py_proto.trans.cstringio_refill
    (<TCallbackTransport *>trans).setReadCallback(read_callback,
                                                  <void *>&readdata)
    cdef ProtocolID protocol_id = proto_type(py_proto)
    cdef TProtocol *proto

    try:
        if protocol_id == T_BINARY_PROTOCOL:
            proto = <TProtocol *>new TBinaryProtocolT[TTransport](trans)
        elif protocol_id == T_COMPACT_PROTOCOL:
            proto = <TProtocol *>new TCompactProtocolT[TTransport](trans)
        else:
            return False
        decode_struct(proto, py_struct, type_args, utf8strings)
    finally:
        if proto != NULL:
            del proto
        del trans
    return True

def encode(py_proto, py_struct, type_args):
    cdef TTransport *trans = <TTransport *>new TMemoryBuffer()
    cdef TProtocol *proto
    cdef ProtocolID protocol_id = proto_type(py_proto)
    cdef bytes encoded
    try:
        if protocol_id == T_BINARY_PROTOCOL:
            proto = <TProtocol *>new TBinaryProtocolT[TTransport](trans)
        elif protocol_id == T_COMPACT_PROTOCOL:
            proto = <TProtocol *>new TCompactProtocolT[TTransport](trans)
        else:
            return False
        encode_struct(proto, type_args, py_struct)
        encoded = (<TMemoryBuffer *>trans).getBufferAsString()
        py_proto.trans.write(encoded)
    finally:
        if proto != NULL:
            del proto
        del trans

    return True
