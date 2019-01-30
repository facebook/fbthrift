# distutils: language = c++

cdef extern from "folly/Optional.h" namespace "folly" nogil:
    cdef cppclass cOptional "folly::Optional"[T]:
        cOptional()
        cOptional(T val)
        bint has_value()
        T value()
        T* get_pointer()
        void reset()
        void assign(T&& newValue)
