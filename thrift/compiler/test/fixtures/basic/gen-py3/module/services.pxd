#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from thrift.py3.server cimport ServiceInterface


cdef class MyServiceInterface(ServiceInterface):
    pass

cdef class MyServiceFastInterface(ServiceInterface):
    pass

cdef class MyServiceEmptyInterface(ServiceInterface):
    pass

cdef class MyServicePrioParentInterface(ServiceInterface):
    pass

cdef class MyServicePrioChildInterface(module.services.MyServicePrioParentInterface):
    pass

