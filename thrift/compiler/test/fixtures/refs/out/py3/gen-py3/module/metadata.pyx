#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/refs/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.memory cimport shared_ptr, make_shared
from libcpp.utility cimport move as cmove

from apache.thrift.metadata.cbindings cimport (
    cThriftMetadata,
)
from apache.thrift.metadata.converter cimport (
    ThriftMetadata_from_cpp
)

from module.metadata cimport cGetThriftModuleMetadata

def getThriftModuleMetadata():
    cdef shared_ptr[cThriftMetadata] metadata
    metadata = make_shared[cThriftMetadata](cGetThriftModuleMetadata())
    return ThriftMetadata_from_cpp(cmove(metadata))
