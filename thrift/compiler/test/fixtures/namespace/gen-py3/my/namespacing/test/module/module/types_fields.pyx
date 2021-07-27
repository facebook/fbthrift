#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
cimport cython as __cython
from cython.operator cimport dereference as deref
from libcpp.memory cimport make_unique, unique_ptr, shared_ptr
from thrift.py3.types cimport assign_unique_ptr, assign_shared_ptr, assign_shared_const_ptr

cimport thrift.py3.types
from thrift.py3.types cimport (
    reset_field as __reset_field,
    StructFieldsSetter as __StructFieldsSetter
)

from thrift.py3.types cimport const_pointer_cast


@__cython.auto_pickle(False)
cdef class __Foo_FieldsSetter(__StructFieldsSetter):

    @staticmethod
    cdef __Foo_FieldsSetter create(_my_namespacing_test_module_module_types.cFoo* struct_cpp_obj):
        cdef __Foo_FieldsSetter __fbthrift_inst = __Foo_FieldsSetter.__new__(__Foo_FieldsSetter)
        __fbthrift_inst._struct_cpp_obj = struct_cpp_obj
        __fbthrift_inst._setters[__cstring_view(<const char*>"MyInt")] = __Foo_FieldsSetter._set_field_0
        return __fbthrift_inst

    cdef void set_field(__Foo_FieldsSetter self, const char* name, object value) except *:
        cdef __cstring_view cname = __cstring_view(name)
        cdef cumap[__cstring_view, __Foo_FieldsSetterFunc].iterator found = self._setters.find(cname)
        if found == self._setters.end():
            raise TypeError(f"invalid field name {name.decode('utf-8')}")
        deref(found).second(self, value)

    cdef void _set_field_0(self, _fbthrift_value) except *:
        # for field MyInt
        if _fbthrift_value is None:
            __reset_field[_my_namespacing_test_module_module_types.cFoo](deref(self._struct_cpp_obj), 0)
            return
        if not isinstance(_fbthrift_value, int):
            raise TypeError(f'MyInt is not a { int !r}.')
        _fbthrift_value = <cint64_t> _fbthrift_value
        deref(self._struct_cpp_obj).MyInt_ref().assign(_fbthrift_value)

