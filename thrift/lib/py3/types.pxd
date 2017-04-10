cdef class Struct:
    pass


cdef class BadEnum:
    cdef readonly object the_enum
    cdef readonly int value
    cdef readonly str name
