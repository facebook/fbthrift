# stdlib functions that have not been wrapped in the cython distribution
from libc.stdint cimport int64_t
from libcpp cimport bool as cbool

cdef extern from "<chrono>" namespace "std::chrono" nogil:
    cdef cppclass milliseconds:
        milliseconds(int64_t) except +
        int64_t count()

cdef extern from "<iterator>" namespace "std" nogil:
    cdef cppclass iterator_traits[T]:
        cppclass difference_type:
            pass
    cdef iterator_traits[InputIter].difference_type distance[InputIter](
        InputIter first,
        InputIter second)

    cdef InputIter next[InputIter](
        InputIter it,
        int64_t n)

    cdef InputIter prev[InputIter](
        InputIter it,
        int64_t n)

cdef extern from "<algorithm>" namespace "std" nogil:
    cdef InputIter find[InputIter, T](
        InputIter first,
        InputIter second,
        const T& val)
    cdef iterator_traits[InputIter].difference_type count[InputIter, T](
        InputIter first,
        InputIter second,
        const T& val)
    cdef cbool includes[Iter1, Iter2](
        Iter1 first1,
        Iter1 last1,
        Iter2 first2,
        Iter2 last2)
    cdef OutputIter set_intersection[Iter1, Iter2, OutputIter](
        Iter1 first1,
        Iter1 last1,
        Iter2 first2,
        Iter2 last2,
        OutputIter result)
    cdef OutputIter set_difference[Iter1, Iter2, OutputIter](
        Iter1 first1,
        Iter1 last1,
        Iter2 first2,
        Iter2 last2,
        OutputIter result)
    cdef OutputIter set_union[Iter1, Iter2, OutputIter](
        Iter1 first1,
        Iter1 last1,
        Iter2 first2,
        Iter2 last2,
        OutputIter result)
    cdef OutputIter set_symmetric_difference[Iter1, Iter2, OutputIter](
        Iter1 first1,
        Iter1 last1,
        Iter2 first2,
        Iter2 last2,
        OutputIter result)
