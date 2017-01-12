from libcpp cimport bool as cbool

cdef extern from "folly/Unit.h" namespace "folly":
    struct cFollyUnit "folly::Unit":
        pass

    cFollyUnit c_unit "folly::unit"

cdef extern from "folly/futures/Promise.h" namespace "folly":
    cdef cppclass cFollyPromise "folly::Promise"[T]:
        void setValue[M](M& value)
        void setException[E](E& value)
