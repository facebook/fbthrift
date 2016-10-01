
cdef class Promise_i32:
    cdef shared_ptr[cFollyPromise[int32_t]] cPromise

    @staticmethod
    cdef create(shared_ptr[cFollyPromise[int32_t]] cPromise):
        inst = <Promise_i32>Promise_i32.__new__(Promise_i32)
        inst.cPromise = cPromise
        return inst

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

