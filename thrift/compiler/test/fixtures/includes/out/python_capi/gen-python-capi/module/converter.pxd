
#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#


cdef extern from "thrift/compiler/test/fixtures/includes/gen-cpp2/module_types.h":
    cdef cppclass cMyStruct "::cpp2::MyStruct":
        cMyStruct()

cdef cMyStruct MyStruct_convert_to_cpp(object inst) except*
cdef object MyStruct_from_cpp(const cMyStruct& c_struct)

