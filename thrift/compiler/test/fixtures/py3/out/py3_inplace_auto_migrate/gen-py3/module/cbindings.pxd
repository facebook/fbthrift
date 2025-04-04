#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/py3/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
from libc.stdint cimport (
    int8_t as cint8_t,
    int16_t as cint16_t,
    int32_t as cint32_t,
    int64_t as cint64_t,
    uint16_t as cuint16_t,
    uint32_t as cuint32_t,
)
from libcpp.string cimport string
from libcpp cimport bool as cbool, nullptr, nullptr_t
from cpython cimport bool as pbool
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.vector cimport vector
from libcpp.set cimport set as cset
from libcpp.map cimport map as cmap, pair as cpair
from libcpp.unordered_map cimport unordered_map as cumap
cimport folly.iobuf as _fbthrift_iobuf
from thrift.python.exceptions cimport cTException
from thrift.py3.types cimport (
    bstring,
    field_ref as __field_ref,
    optional_field_ref as __optional_field_ref,
    required_field_ref as __required_field_ref,
    terse_field_ref as __terse_field_ref,
    union_field_ref as __union_field_ref,
    get_union_field_value as __get_union_field_value,
)
from thrift.python.common cimport cThriftMetadata as __fbthrift_cThriftMetadata


cdef extern from * nogil:
    cdef cppclass _std_unordered_map "::std::unordered_map"[T, U]:
        ctypedef T key_type
        ctypedef U mapped_type
        ctypedef size_t size_type

        cppclass iterator:
            cpair[T, U]& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cpair[T, U]& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _std_unordered_map() except +
        _std_unordered_map(_std_unordered_map&) except +

        U& operator[](T&)
        iterator find(const T&)
        const_iterator const_find "find"(const T&)
        size_type count(const T&)
        size_type size()
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from *:
    ctypedef bstring foo_Bar "foo::Bar"
cdef extern from *:
    ctypedef cbool _MyType "::MyType"
cdef extern from *:
    ctypedef cint64_t _MyType "::MyType"
cdef extern from *:
    ctypedef double _MyType "::MyType"
cdef extern from *:
    ctypedef string _MyType "::MyType"
cdef extern from *:
    ctypedef bstring _MyType "::MyType"
cdef extern from * nogil:
    cdef cppclass _MyType "::MyType":
        ctypedef cint32_t value_type
        ctypedef size_t size_type

        cppclass iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _MyType() except +
        _MyType(_MyType&) except +

        cint32_t& operator[](size_type)
        void push_back(cint32_t&) except +
        size_type size()
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from * nogil:
    cdef cppclass _MyType "::MyType":
        ctypedef cint32_t value_type
        ctypedef size_t size_type

        cppclass iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _MyType() except +
        _MyType(_MyType&) except +

        cpair[iterator, bint] insert(const cint32_t&) except +
        size_type size()
        size_type count(const cint32_t&)
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from * nogil:
    cdef cppclass _MyType "::MyType":
        ctypedef cint32_t key_type
        ctypedef cint32_t mapped_type
        ctypedef size_t size_type

        cppclass iterator:
            cpair[cint32_t, cint32_t]& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cpair[cint32_t, cint32_t]& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _MyType() except +
        _MyType(_MyType&) except +

        cint32_t& operator[](cint32_t&)
        iterator find(const cint32_t&)
        const_iterator const_find "find"(const cint32_t&)
        size_type count(const cint32_t&)
        size_type size()
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from *:
    ctypedef string _py3_simple_AdaptedString "::py3::simple::AdaptedString"
cdef extern from *:
    ctypedef bstring _py3_simple_AdaptedBinary "::py3::simple::AdaptedBinary"
cdef extern from * nogil:
    cdef cppclass _py3_simple_AdaptedList "::py3::simple::AdaptedList":
        ctypedef cint32_t value_type
        ctypedef size_t size_type

        cppclass iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _py3_simple_AdaptedList() except +
        _py3_simple_AdaptedList(_py3_simple_AdaptedList&) except +

        cint32_t& operator[](size_type)
        void push_back(cint32_t&) except +
        size_type size()
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from * nogil:
    cdef cppclass _py3_simple_AdaptedSet "::py3::simple::AdaptedSet":
        ctypedef cint32_t value_type
        ctypedef size_t size_type

        cppclass iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cint32_t& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _py3_simple_AdaptedSet() except +
        _py3_simple_AdaptedSet(_py3_simple_AdaptedSet&) except +

        cpair[iterator, bint] insert(const cint32_t&) except +
        size_type size()
        size_type count(const cint32_t&)
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()

cdef extern from * nogil:
    cdef cppclass _py3_simple_AdaptedMap "::py3::simple::AdaptedMap":
        ctypedef cint32_t key_type
        ctypedef cint32_t mapped_type
        ctypedef size_t size_type

        cppclass iterator:
            cpair[cint32_t, cint32_t]& operator*()
            iterator operator++()
            bint operator==(iterator)
            bint operator!=(iterator)
        cppclass reverse_iterator:
            cpair[cint32_t, cint32_t]& operator*()
            iterator operator++()
            bint operator==(reverse_iterator)
            bint operator!=(reverse_iterator)
        cppclass const_iterator(iterator):
            pass
        cppclass const_reverse_iterator(reverse_iterator):
            pass

        _py3_simple_AdaptedMap() except +
        _py3_simple_AdaptedMap(_py3_simple_AdaptedMap&) except +

        cint32_t& operator[](cint32_t&)
        iterator find(const cint32_t&)
        const_iterator const_find "find"(const cint32_t&)
        size_type count(const cint32_t&)
        size_type size()
        iterator begin()
        const_iterator const_begin "begin"()
        iterator end()
        const_iterator const_end "end"()
        reverse_iterator rbegin()
        const_reverse_iterator const_rbegin "rbegin"()
        reverse_iterator rend()
        const_reverse_iterator const_rend "rend"()
        void clear()
        bint empty()


cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass EnumMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_types.h" namespace "::py3::simple":
    cdef cppclass cAnEnum "::py3::simple::AnEnum":
        pass

    cdef cppclass cAnEnumRenamed "::py3::simple::AnEnumRenamed":
        pass

    cdef cppclass cFlags "::py3::simple::Flags":
        pass

cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass ExceptionMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_metadata.h" namespace "apache::thrift::detail::md":
    cdef cppclass StructMetadata[T]:
        @staticmethod
        void gen(__fbthrift_cThriftMetadata &metadata)
cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_types_custom_protocol.h" namespace "::py3::simple":

    cdef cppclass cSimpleException "::py3::simple::SimpleException"(cTException):
        cSimpleException() except +
        cSimpleException(const cSimpleException&) except +
        bint operator==(cSimpleException&)
        bint operator!=(cSimpleException&)
        bint operator<(cSimpleException&)
        bint operator>(cSimpleException&)
        bint operator<=(cSimpleException&)
        bint operator>=(cSimpleException&)
        __field_ref[cint16_t] err_code_ref "err_code_ref" ()


    cdef cppclass cOptionalRefStruct "::py3::simple::OptionalRefStruct":
        cOptionalRefStruct() except +
        cOptionalRefStruct(const cOptionalRefStruct&) except +
        bint operator==(cOptionalRefStruct&)
        bint operator!=(cOptionalRefStruct&)
        bint operator<(cOptionalRefStruct&)
        bint operator>(cOptionalRefStruct&)
        bint operator<=(cOptionalRefStruct&)
        bint operator>=(cOptionalRefStruct&)
        __optional_field_ref[unique_ptr[_fbthrift_iobuf.cIOBuf]] optional_blob_ref "optional_blob_ref" ()


    cdef cppclass cSimpleStruct "::py3::simple::SimpleStruct":
        cSimpleStruct() except +
        cSimpleStruct(const cSimpleStruct&) except +
        bint operator==(cSimpleStruct&)
        bint operator!=(cSimpleStruct&)
        __field_ref[cbool] is_on_ref "is_on_ref" ()
        __field_ref[cint8_t] tiny_int_ref "tiny_int_ref" ()
        __field_ref[cint16_t] small_int_ref "small_int_ref" ()
        __field_ref[cint32_t] nice_sized_int_ref "nice_sized_int_ref" ()
        __field_ref[cint64_t] big_int_ref "big_int_ref" ()
        __field_ref[double] real_ref "real_ref" ()
        __field_ref[float] smaller_real_ref "smaller_real_ref" ()
        __field_ref[_std_unordered_map[cint32_t,cint32_t]] something_ref "something_ref" ()
        __optional_field_ref[cint32_t] opt_default_int_ref "opt_default_int_ref" ()
        __optional_field_ref[string] opt_default_str_ref "opt_default_str_ref" ()
        __optional_field_ref[cAnEnum] opt_default_enum_ref "opt_default_enum_ref" ()


    cdef cppclass cHiddenTypeFieldsStruct "::py3::simple::HiddenTypeFieldsStruct":
        cHiddenTypeFieldsStruct() except +
        cHiddenTypeFieldsStruct(const cHiddenTypeFieldsStruct&) except +
        bint operator==(cHiddenTypeFieldsStruct&)
        bint operator!=(cHiddenTypeFieldsStruct&)


    cdef cppclass cComplexStruct "::py3::simple::ComplexStruct":
        cComplexStruct() except +
        cComplexStruct(const cComplexStruct&) except +
        bint operator==(cComplexStruct&)
        bint operator!=(cComplexStruct&)
        __field_ref[cSimpleStruct] structOne_ref "structOne_ref" ()
        __field_ref[cSimpleStruct] structTwo_ref "structTwo_ref" ()
        __field_ref[cint32_t] an_integer_ref "an_integer_ref" ()
        __field_ref[string] name_ref "name_ref" ()
        __field_ref[cAnEnum] an_enum_ref "an_enum_ref" ()
        __field_ref[string] some_bytes_ref "some_bytes_ref" ()
        __field_ref[string] sender_ref "from_ref" ()
        __field_ref[string] cdef__ref "cdef_ref" ()
        __field_ref[foo_Bar] bytes_with_cpp_type_ref "bytes_with_cpp_type_ref" ()

    cdef enum cBinaryUnion__type "::py3::simple::BinaryUnion::Type":
        cBinaryUnion__type___EMPTY__ "::py3::simple::BinaryUnion::Type::__EMPTY__",
        cBinaryUnion__type_iobuf_val "::py3::simple::BinaryUnion::Type::iobuf_val",

    cdef cppclass cBinaryUnion "::py3::simple::BinaryUnion":
        cBinaryUnion() except +
        cBinaryUnion(const cBinaryUnion&) except +
        cBinaryUnion__type getType() const
        const _fbthrift_iobuf.cIOBuf& get_iobuf_val "get_iobuf_val" () const
        _fbthrift_iobuf.cIOBuf& set_iobuf_val "set_iobuf_val" (const _fbthrift_iobuf.cIOBuf&)


    cdef cppclass cBinaryUnionStruct "::py3::simple::BinaryUnionStruct":
        cBinaryUnionStruct() except +
        cBinaryUnionStruct(const cBinaryUnionStruct&) except +
        __field_ref[cBinaryUnion] u_ref "u_ref" ()


    cdef cppclass cCustomFields "::py3::simple::CustomFields":
        cCustomFields() except +
        cCustomFields(const cCustomFields&) except +
        bint operator==(cCustomFields&)
        bint operator!=(cCustomFields&)
        __field_ref[_MyType] bool_field_ref "bool_field_ref" ()
        __field_ref[_MyType] integer_field_ref "integer_field_ref" ()
        __field_ref[_MyType] double_field_ref "double_field_ref" ()
        __field_ref[_MyType] string_field_ref "string_field_ref" ()
        __field_ref[_MyType] binary_field_ref "binary_field_ref" ()
        __field_ref[_MyType] list_field_ref "list_field_ref" ()
        __field_ref[_MyType] set_field_ref "set_field_ref" ()
        __field_ref[_MyType] map_field_ref "map_field_ref" ()
        __field_ref[cSimpleStruct] struct_field_ref "struct_field_ref" ()


    cdef cppclass cCustomTypedefFields "::py3::simple::CustomTypedefFields":
        cCustomTypedefFields() except +
        cCustomTypedefFields(const cCustomTypedefFields&) except +
        bint operator==(cCustomTypedefFields&)
        bint operator!=(cCustomTypedefFields&)
        __field_ref[_MyType] bool_field_ref "bool_field_ref" ()
        __field_ref[_MyType] integer_field_ref "integer_field_ref" ()
        __field_ref[_MyType] double_field_ref "double_field_ref" ()
        __field_ref[_MyType] string_field_ref "string_field_ref" ()
        __field_ref[_MyType] binary_field_ref "binary_field_ref" ()
        __field_ref[_MyType] list_field_ref "list_field_ref" ()
        __field_ref[_MyType] set_field_ref "set_field_ref" ()
        __field_ref[_MyType] map_field_ref "map_field_ref" ()
        __field_ref[cSimpleStruct] struct_field_ref "struct_field_ref" ()


    cdef cppclass cAdaptedTypedefFields "::py3::simple::AdaptedTypedefFields":
        cAdaptedTypedefFields() except +
        cAdaptedTypedefFields(const cAdaptedTypedefFields&) except +
        bint operator==(cAdaptedTypedefFields&)
        bint operator!=(cAdaptedTypedefFields&)
        __field_ref[cbool] bool_field_ref "bool_field_ref" ()
        __field_ref[cint64_t] integer_field_ref "integer_field_ref" ()
        __field_ref[double] double_field_ref "double_field_ref" ()
        __field_ref[_py3_simple_AdaptedString] string_field_ref "string_field_ref" ()
        __field_ref[_py3_simple_AdaptedBinary] binary_field_ref "binary_field_ref" ()
        __field_ref[_py3_simple_AdaptedList] list_field_ref "list_field_ref" ()
        __field_ref[_py3_simple_AdaptedSet] set_field_ref "set_field_ref" ()
        __field_ref[_py3_simple_AdaptedMap] map_field_ref "map_field_ref" ()
        __field_ref[cSimpleStruct] struct_field_ref "struct_field_ref" ()

cdef extern from "thrift/compiler/test/fixtures/py3/gen-cpp2/module_constants.h" namespace "::py3::simple":
    cdef cbool cA_BOOL "::py3::simple::module_constants::A_BOOL"
    cdef cint8_t cA_BYTE "::py3::simple::module_constants::A_BYTE"
    cdef cint16_t cTHE_ANSWER "::py3::simple::module_constants::THE_ANSWER"
    cdef cint32_t cA_NUMBER "::py3::simple::module_constants::A_NUMBER"
    cdef cint64_t cA_BIG_NUMBER "::py3::simple::module_constants::A_BIG_NUMBER"
    cdef double cA_REAL_NUMBER "::py3::simple::module_constants::A_REAL_NUMBER"
    cdef double cA_FAKE_NUMBER "::py3::simple::module_constants::A_FAKE_NUMBER"
    cdef const char* cA_WORD "::py3::simple::module_constants::A_WORD"()
    cdef string cSOME_BYTES "::py3::simple::module_constants::SOME_BYTES"()
    cdef cSimpleStruct cA_STRUCT "::py3::simple::module_constants::A_STRUCT"()
    cdef cSimpleStruct cEMPTY "::py3::simple::module_constants::EMPTY"()
    cdef vector[string] cWORD_LIST "::py3::simple::module_constants::WORD_LIST"()
    cdef vector[cmap[cint32_t,double]] cSOME_MAP "::py3::simple::module_constants::SOME_MAP"()
    cdef cset[cint32_t] cDIGITS "::py3::simple::module_constants::DIGITS"()
    cdef cmap[string,cSimpleStruct] cA_CONST_MAP "::py3::simple::module_constants::A_CONST_MAP"()
    cdef cmap[cAnEnumRenamed,cint32_t] cANOTHER_CONST_MAP "::py3::simple::module_constants::ANOTHER_CONST_MAP"()
