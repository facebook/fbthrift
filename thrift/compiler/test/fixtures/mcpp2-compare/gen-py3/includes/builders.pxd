#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
from cpython cimport bool as pbool, int as pint, float as pfloat

cimport folly.iobuf as _fbthrift_iobuf

cimport thrift.py3.builder


cimport includes.types as _includes_types

cdef class AStruct_Builder(thrift.py3.builder.StructBuilder):
    cdef public pint FieldA


cdef class AStructB_Builder(thrift.py3.builder.StructBuilder):
    cdef public object FieldA


