#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
from cpython cimport bool as pbool, int as pint, float as pfloat

cimport folly.iobuf as _fbthrift_iobuf

cimport thrift.py3.builder


cimport module.types as _module_types

cdef class SimpleException_Builder(thrift.py3.builder.StructBuilder):
    cdef public pint err_code


cdef class OptionalRefStruct_Builder(thrift.py3.builder.StructBuilder):
    cdef public _fbthrift_iobuf.IOBuf optional_blob


cdef class SimpleStruct_Builder(thrift.py3.builder.StructBuilder):
    cdef public pbool is_on
    cdef public pint tiny_int
    cdef public pint small_int
    cdef public pint nice_sized_int
    cdef public pint big_int
    cdef public pfloat real
    cdef public pfloat smaller_real


cdef class ComplexStruct_Builder(thrift.py3.builder.StructBuilder):
    cdef public object structOne
    cdef public object structTwo
    cdef public pint an_integer
    cdef public str name
    cdef public _module_types.AnEnum an_enum
    cdef public bytes some_bytes
    cdef public str sender
    cdef public str cdef_
    cdef public bytes bytes_with_cpp_type


cdef class BinaryUnion_Builder(thrift.py3.builder.StructBuilder):
    cdef public _fbthrift_iobuf.IOBuf iobuf_val


cdef class BinaryUnionStruct_Builder(thrift.py3.builder.StructBuilder):
    cdef public object u


