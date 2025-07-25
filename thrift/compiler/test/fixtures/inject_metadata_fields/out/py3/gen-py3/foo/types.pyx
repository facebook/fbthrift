#
# Autogenerated by Thrift for foo.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
cimport cython as __cython
from cpython.object cimport PyTypeObject
from libcpp.memory cimport shared_ptr, make_shared, unique_ptr
from libcpp.optional cimport optional as __optional
from libcpp.string cimport string
from libcpp cimport bool as cbool
from libcpp.iterator cimport inserter as cinserter
from libcpp.utility cimport move as cmove
from cpython cimport bool as pbool
from cython.operator cimport dereference as deref, preincrement as inc, address as ptr_address
import thrift.py3.types
from thrift.py3.types import _IsSet as _fbthrift_IsSet
from thrift.py3.types cimport make_unique
cimport thrift.py3.types
cimport thrift.py3.exceptions
cimport thrift.python.exceptions
import thrift.python.converter
from thrift.python.types import EnumMeta as __EnumMeta
from thrift.python.std_libcpp cimport sv_to_str as __sv_to_str, string_view as __cstring_view
from thrift.python.types cimport BadEnum as __BadEnum
from thrift.py3.types cimport (
    richcmp as __richcmp,
    init_unicode_from_cpp as __init_unicode_from_cpp,
    set_iter as __set_iter,
    map_iter as __map_iter,
    reference_shared_ptr as __reference_shared_ptr,
    get_field_name_by_index as __get_field_name_by_index,
    reset_field as __reset_field,
    translate_cpp_enum_to_python,
    const_pointer_cast,
    make_const_shared,
    constant_shared_ptr,
    mixin_deprecation_log_error,
)
from thrift.py3.types cimport _ensure_py3_or_raise, _ensure_py3_container_or_raise
cimport thrift.py3.serializer as serializer
from thrift.python.protocol cimport Protocol as __Protocol
import folly.iobuf as _fbthrift_iobuf
from folly.optional cimport cOptional
from folly.memory cimport to_shared_ptr as __to_shared_ptr
from folly.range cimport Range as __cRange

import sys
from collections.abc import Sequence, Set, Mapping, Iterable
import weakref as __weakref
import builtins as _builtins
import importlib

import foo.thrift_types as _fbthrift_python_types


_fbthrift__module_name__ = "foo.types"

cdef object get_types_reflection():
    return importlib.import_module(
        "foo.types_reflection"
    )

@__cython.auto_pickle(False)
@__cython.final
cdef class Fields(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(Fields self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_foo_cbindings.cFields]()
        self._fields_setter = _fbthrift_types_fields.__Fields_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(Fields self, **kwargs):
        if not kwargs:
            return self
        cdef Fields __fbthrift_inst = Fields.__new__(Fields)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_foo_cbindings.cFields](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__Fields_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("Fields", {
          "injected_field": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_field_ref().has_value(),
          "injected_structured_annotation_field": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_structured_annotation_field_ref().has_value(),
          "injected_unstructured_annotation_field": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_unstructured_annotation_field_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_foo_cbindings.cFields] cpp_obj):
        __fbthrift_inst = <Fields>Fields.__new__(Fields)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline injected_field_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_field_ref().value()).decode('UTF-8')

    @property
    def injected_field(self):
        return self.injected_field_impl()

    cdef inline injected_structured_annotation_field_impl(self):
        if not deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_structured_annotation_field_ref().has_value():
            return None
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_structured_annotation_field_ref().value()).decode('UTF-8')

    @property
    def injected_structured_annotation_field(self):
        return self.injected_structured_annotation_field_impl()

    cdef inline injected_unstructured_annotation_field_impl(self):
        if not deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_unstructured_annotation_field_ref().has_value():
            return None
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).injected_unstructured_annotation_field_ref().value()).decode('UTF-8')

    @property
    def injected_unstructured_annotation_field(self):
        return self.injected_unstructured_annotation_field_impl()


    def __hash__(Fields self):
        return super().__hash__()

    def __repr__(Fields self):
        return super().__repr__()

    def __str__(Fields self):
        return super().__str__()


    def __copy__(Fields self):
        return self

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_foo_cbindings.cFields](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<Fields>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Fields()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _foo_cbindings.StructMetadata[_foo_cbindings.cFields].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "foo.Fields"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_foo_cbindings.cFields](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 3

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(Fields self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_foo_cbindings.cFields](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(Fields self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_foo_cbindings.cFields]()
        with nogil:
            needed = serializer.cdeserialize[_foo_cbindings.cFields](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.Fields,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("foo.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.Fields, self)


