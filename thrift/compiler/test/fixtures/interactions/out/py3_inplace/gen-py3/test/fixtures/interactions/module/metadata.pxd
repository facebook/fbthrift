#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/interactions/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from apache.thrift.metadata.cbindings cimport cThriftMetadata
from thrift.python.common cimport (
    cThriftMetadata as __fbthrift_cThriftMetadata,
)

cdef extern from "thrift/compiler/test/fixtures/interactions/gen-py3/module/metadata.h" :
    cdef cThriftMetadata cGetThriftModuleMetadata "::cpp2::module_getThriftModuleMetadata"()
