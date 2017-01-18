#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from libcpp.string cimport string
from libcpp cimport bool as cbool
from cpython cimport bool as pbool
from libc.stdint cimport int8_t, int16_t, int32_t, int64_t
from libcpp.memory cimport shared_ptr, unique_ptr
from libcpp.vector cimport vector
from libcpp.set cimport set as cset
from libcpp.map cimport map as cmap, pair as cpair
from thrift.py3.exceptions cimport cTException, TException


cdef extern from "src/gen-cpp2/module_types.h" namespace "cpp2":
    cdef cppclass cEmptyEnum "cpp2::EmptyEnum":
        bint operator==(cEmptyEnum&)
    cdef cppclass cCity "cpp2::City":
        bint operator==(cCity&)
    cCity City__NYC "cpp2::City::NYC"
    cCity City__MPK "cpp2::City::MPK"
    cCity City__SEA "cpp2::City::SEA"
    cCity City__LON "cpp2::City::LON"
    cdef cppclass cCompany "cpp2::Company":
        bint operator==(cCompany&)
    cCompany Company__FACEBOOK "cpp2::Company::FACEBOOK"
    cCompany Company__WHATSAPP "cpp2::Company::WHATSAPP"
    cCompany Company__OCULUS "cpp2::Company::OCULUS"
    cCompany Company__INSTAGRAM "cpp2::Company::INSTAGRAM"

cdef cEmptyEnum EmptyEnum_to_cpp(value)
cdef cCity City_to_cpp(value)
cdef cCompany Company_to_cpp(value)

cdef extern from "src/gen-cpp2/module_types.h" namespace "cpp2":
    cdef cppclass cInternship "cpp2::Internship":
        cInternship() except +
        bint operator==(cInternship&)
        int32_t weeks
        string title
        cCompany employer

    cdef cppclass cUnEnumStruct "cpp2::UnEnumStruct":
        cUnEnumStruct() except +
        bint operator==(cUnEnumStruct&)
        cCity city

    cdef cppclass cRange "cpp2::Range":
        cRange() except +
        bint operator==(cRange&)
        int32_t min
        int32_t max


cdef extern from "<utility>" namespace "std" nogil:
    cdef shared_ptr[cInternship] move(unique_ptr[cInternship])
    cdef shared_ptr[cUnEnumStruct] move(unique_ptr[cUnEnumStruct])
    cdef shared_ptr[cRange] move(unique_ptr[cRange])

cdef class Internship:
    cdef shared_ptr[cInternship] c_Internship

    @staticmethod
    cdef create(shared_ptr[cInternship] c_Internship)

cdef class UnEnumStruct:
    cdef shared_ptr[cUnEnumStruct] c_UnEnumStruct

    @staticmethod
    cdef create(shared_ptr[cUnEnumStruct] c_UnEnumStruct)

cdef class Range:
    cdef shared_ptr[cRange] c_Range

    @staticmethod
    cdef create(shared_ptr[cRange] c_Range)


cdef class Map__string_i32:
    cdef shared_ptr[cmap[string,int32_t]] _map
    @staticmethod
    cdef create(shared_ptr[cmap[string,int32_t]])

cdef class List__Map__string_i32:
    cdef shared_ptr[vector[cmap[string,int32_t]]] _vector
    @staticmethod
    cdef create(shared_ptr[vector[cmap[string,int32_t]]])

cdef class List__Range:
    cdef shared_ptr[vector[cRange]] _vector
    @staticmethod
    cdef create(shared_ptr[vector[cRange]])

cdef class List__Internship:
    cdef shared_ptr[vector[cInternship]] _vector
    @staticmethod
    cdef create(shared_ptr[vector[cInternship]])

cdef class List__string:
    cdef shared_ptr[vector[string]] _vector
    @staticmethod
    cdef create(shared_ptr[vector[string]])

cdef class List__i32:
    cdef shared_ptr[vector[int32_t]] _vector
    @staticmethod
    cdef create(shared_ptr[vector[int32_t]])

cdef class Set__i32:
    cdef shared_ptr[cset[int32_t]] _set
    @staticmethod
    cdef create(shared_ptr[cset[int32_t]])

cdef class Set__string:
    cdef shared_ptr[cset[string]] _set
    @staticmethod
    cdef create(shared_ptr[cset[string]])

cdef class Map__i32_i32:
    cdef shared_ptr[cmap[int32_t,int32_t]] _map
    @staticmethod
    cdef create(shared_ptr[cmap[int32_t,int32_t]])

cdef class Map__i32_string:
    cdef shared_ptr[cmap[int32_t,string]] _map
    @staticmethod
    cdef create(shared_ptr[cmap[int32_t,string]])

cdef class Map__string_string:
    cdef shared_ptr[cmap[string,string]] _map
    @staticmethod
    cdef create(shared_ptr[cmap[string,string]])

cdef extern from "<utility>" namespace "std" nogil:
    cdef shared_ptr[cmap[string,int32_t]] move(unique_ptr[cmap[string,int32_t]])
    cdef shared_ptr[vector[cmap[string,int32_t]]] move(unique_ptr[vector[cmap[string,int32_t]]])
    cdef shared_ptr[vector[cRange]] move(unique_ptr[vector[cRange]])
    cdef shared_ptr[vector[cInternship]] move(unique_ptr[vector[cInternship]])
    cdef shared_ptr[vector[string]] move(unique_ptr[vector[string]])
    cdef shared_ptr[vector[int32_t]] move(unique_ptr[vector[int32_t]])
    cdef shared_ptr[cset[int32_t]] move(unique_ptr[cset[int32_t]])
    cdef shared_ptr[cset[string]] move(unique_ptr[cset[string]])
    cdef shared_ptr[cmap[int32_t,int32_t]] move(unique_ptr[cmap[int32_t,int32_t]])
    cdef shared_ptr[cmap[int32_t,string]] move(unique_ptr[cmap[int32_t,string]])
    cdef shared_ptr[cmap[string,string]] move(unique_ptr[cmap[string,string]])

cdef extern from "src/gen-cpp2/module_constants.h" namespace "cpp2":
    cdef int32_t cmyInt "cpp2::module_constants::myInt"
    cdef const char* cname "cpp2::module_constants::name"()
    cdef vector[cmap[string,int32_t]] cstates "cpp2::module_constants::states"()
    cdef double cx "cpp2::module_constants::x"
    cdef double cy "cpp2::module_constants::y"
    cdef double cz "cpp2::module_constants::z"
    cdef cInternship cinstagram "cpp2::module_constants::instagram"()
    cdef vector[cRange] ckRanges "cpp2::module_constants::kRanges"()
    cdef vector[cInternship] cinternList "cpp2::module_constants::internList"()
    cdef const char* capostrophe "cpp2::module_constants::apostrophe"()
    cdef const char* ctripleApostrophe "cpp2::module_constants::tripleApostrophe"()
    cdef const char* cquotationMark "cpp2::module_constants::quotationMark"()
    cdef const char* cbackslash "cpp2::module_constants::backslash"()
    cdef const char* cescaped_a "cpp2::module_constants::escaped_a"()
    cdef cmap[string,int32_t] cchar2ascii "cpp2::module_constants::char2ascii"()
    cdef vector[string] cescaped_strings "cpp2::module_constants::escaped_strings"()
    cdef cbool cfalse_c "cpp2::module_constants::false_c"
    cdef cbool ctrue_c "cpp2::module_constants::true_c"
    cdef int8_t czero_byte "cpp2::module_constants::zero_byte"
    cdef int16_t czero16 "cpp2::module_constants::zero16"
    cdef int32_t czero32 "cpp2::module_constants::zero32"
    cdef int64_t czero64 "cpp2::module_constants::zero64"
    cdef double czero_dot_zero "cpp2::module_constants::zero_dot_zero"
    cdef const char* cempty_string "cpp2::module_constants::empty_string"()
    cdef vector[int32_t] cempty_int_list "cpp2::module_constants::empty_int_list"()
    cdef vector[string] cempty_string_list "cpp2::module_constants::empty_string_list"()
    cdef cset[int32_t] cempty_int_set "cpp2::module_constants::empty_int_set"()
    cdef cset[string] cempty_string_set "cpp2::module_constants::empty_string_set"()
    cdef cmap[int32_t,int32_t] cempty_int_int_map "cpp2::module_constants::empty_int_int_map"()
    cdef cmap[int32_t,string] cempty_int_string_map "cpp2::module_constants::empty_int_string_map"()
    cdef cmap[string,int32_t] cempty_string_int_map "cpp2::module_constants::empty_string_int_map"()
    cdef cmap[string,string] cempty_string_string_map "cpp2::module_constants::empty_string_string_map"()
