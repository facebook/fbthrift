
cdef class Promise_void:
    cdef shared_ptr[cFollyPromise[cFollyUnit]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[cFollyUnit]] cPromise):
        inst = <Promise_void>Promise_void.__new__(Promise_void)
        inst.cPromise = cPromise
        return inst

cdef class Promise_string:
    cdef shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[unique_ptr[string]]] cPromise):
        inst = <Promise_string>Promise_string.__new__(Promise_string)
        inst.cPromise = cPromise
        return inst

cdef class Promise_bool:
    cdef shared_ptr[cFollyPromise[]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[]] cPromise):
        inst = <Promise_bool>Promise_bool.__new__(Promise_bool)
        inst.cPromise = cPromise
        return inst

