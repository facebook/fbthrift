from thrift.py3.exceptions cimport TException as Exception


cdef class Struct:
    pass


cdef class BadEnum:
    cdef readonly object the_enum
    cdef readonly int value
    cdef readonly str name
