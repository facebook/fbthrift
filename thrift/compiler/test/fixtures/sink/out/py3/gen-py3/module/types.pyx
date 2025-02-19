#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/sink/src/module.thrift
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
)
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

import module.thrift_types as _fbthrift_python_types


_fbthrift__module_name__ = "module.types"

cdef object get_types_reflection():
    return importlib.import_module(
        "module.types_reflection"
    )

@__cython.auto_pickle(False)
@__cython.final
cdef class InitialResponse(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(InitialResponse self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cInitialResponse]()
        self._fields_setter = _fbthrift_types_fields.__InitialResponse_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(InitialResponse self, **kwargs):
        if not kwargs:
            return self
        cdef InitialResponse __fbthrift_inst = InitialResponse.__new__(InitialResponse)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cInitialResponse](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__InitialResponse_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("InitialResponse", {
          "content": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cInitialResponse] cpp_obj):
        __fbthrift_inst = <InitialResponse>InitialResponse.__new__(InitialResponse)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline content_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().value()).decode('UTF-8')

    @property
    def content(self):
        return self.content_impl()


    def __hash__(InitialResponse self):
        return super().__hash__()

    def __repr__(InitialResponse self):
        return super().__repr__()

    def __str__(InitialResponse self):
        return super().__str__()


    def __copy__(InitialResponse self):
        cdef shared_ptr[_module_cbindings.cInitialResponse] cpp_obj = make_shared[_module_cbindings.cInitialResponse](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return InitialResponse._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cInitialResponse](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<InitialResponse>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__InitialResponse()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.StructMetadata[_module_cbindings.cInitialResponse].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.InitialResponse"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cInitialResponse](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(InitialResponse self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cInitialResponse](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(InitialResponse self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cInitialResponse]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cInitialResponse](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.InitialResponse,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.InitialResponse, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class FinalResponse(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(FinalResponse self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cFinalResponse]()
        self._fields_setter = _fbthrift_types_fields.__FinalResponse_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(FinalResponse self, **kwargs):
        if not kwargs:
            return self
        cdef FinalResponse __fbthrift_inst = FinalResponse.__new__(FinalResponse)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cFinalResponse](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__FinalResponse_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("FinalResponse", {
          "content": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cFinalResponse] cpp_obj):
        __fbthrift_inst = <FinalResponse>FinalResponse.__new__(FinalResponse)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline content_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().value()).decode('UTF-8')

    @property
    def content(self):
        return self.content_impl()


    def __hash__(FinalResponse self):
        return super().__hash__()

    def __repr__(FinalResponse self):
        return super().__repr__()

    def __str__(FinalResponse self):
        return super().__str__()


    def __copy__(FinalResponse self):
        cdef shared_ptr[_module_cbindings.cFinalResponse] cpp_obj = make_shared[_module_cbindings.cFinalResponse](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return FinalResponse._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cFinalResponse](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<FinalResponse>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__FinalResponse()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.StructMetadata[_module_cbindings.cFinalResponse].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.FinalResponse"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cFinalResponse](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(FinalResponse self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cFinalResponse](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(FinalResponse self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cFinalResponse]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cFinalResponse](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.FinalResponse,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.FinalResponse, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class SinkPayload(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(SinkPayload self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkPayload]()
        self._fields_setter = _fbthrift_types_fields.__SinkPayload_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(SinkPayload self, **kwargs):
        if not kwargs:
            return self
        cdef SinkPayload __fbthrift_inst = SinkPayload.__new__(SinkPayload)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkPayload](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__SinkPayload_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("SinkPayload", {
          "content": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cSinkPayload] cpp_obj):
        __fbthrift_inst = <SinkPayload>SinkPayload.__new__(SinkPayload)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline content_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).content_ref().value()).decode('UTF-8')

    @property
    def content(self):
        return self.content_impl()


    def __hash__(SinkPayload self):
        return super().__hash__()

    def __repr__(SinkPayload self):
        return super().__repr__()

    def __str__(SinkPayload self):
        return super().__str__()


    def __copy__(SinkPayload self):
        cdef shared_ptr[_module_cbindings.cSinkPayload] cpp_obj = make_shared[_module_cbindings.cSinkPayload](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return SinkPayload._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cSinkPayload](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<SinkPayload>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__SinkPayload()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.StructMetadata[_module_cbindings.cSinkPayload].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.SinkPayload"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cSinkPayload](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(SinkPayload self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cSinkPayload](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(SinkPayload self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkPayload]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cSinkPayload](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.SinkPayload,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.SinkPayload, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class CompatibleWithKeywordSink(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(CompatibleWithKeywordSink self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cCompatibleWithKeywordSink]()
        self._fields_setter = _fbthrift_types_fields.__CompatibleWithKeywordSink_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(CompatibleWithKeywordSink self, **kwargs):
        if not kwargs:
            return self
        cdef CompatibleWithKeywordSink __fbthrift_inst = CompatibleWithKeywordSink.__new__(CompatibleWithKeywordSink)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cCompatibleWithKeywordSink](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__CompatibleWithKeywordSink_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("CompatibleWithKeywordSink", {
          "sink": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).sink_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cCompatibleWithKeywordSink] cpp_obj):
        __fbthrift_inst = <CompatibleWithKeywordSink>CompatibleWithKeywordSink.__new__(CompatibleWithKeywordSink)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline sink_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).sink_ref().value()).decode('UTF-8')

    @property
    def sink(self):
        return self.sink_impl()


    def __hash__(CompatibleWithKeywordSink self):
        return super().__hash__()

    def __repr__(CompatibleWithKeywordSink self):
        return super().__repr__()

    def __str__(CompatibleWithKeywordSink self):
        return super().__str__()


    def __copy__(CompatibleWithKeywordSink self):
        cdef shared_ptr[_module_cbindings.cCompatibleWithKeywordSink] cpp_obj = make_shared[_module_cbindings.cCompatibleWithKeywordSink](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return CompatibleWithKeywordSink._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cCompatibleWithKeywordSink](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<CompatibleWithKeywordSink>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__CompatibleWithKeywordSink()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.StructMetadata[_module_cbindings.cCompatibleWithKeywordSink].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.CompatibleWithKeywordSink"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cCompatibleWithKeywordSink](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(CompatibleWithKeywordSink self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cCompatibleWithKeywordSink](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(CompatibleWithKeywordSink self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cCompatibleWithKeywordSink]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cCompatibleWithKeywordSink](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.CompatibleWithKeywordSink,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.CompatibleWithKeywordSink, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class InitialException(thrift.py3.exceptions.GeneratedError):
    __module__ = _fbthrift__module_name__

    def __init__(InitialException self, *args, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cInitialException]()
        self._fields_setter = _fbthrift_types_fields.__InitialException_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__( *args, **kwargs)

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("InitialException", {
          "reason": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cInitialException] cpp_obj):
        __fbthrift_inst = <InitialException>InitialException.__new__(InitialException, (<bytes>deref(cpp_obj).what()).decode('utf-8'))
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        _builtins.Exception.__init__(__fbthrift_inst, *(v for _, v in __fbthrift_inst))
        return __fbthrift_inst

    cdef inline reason_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().value()).decode('UTF-8')

    @property
    def reason(self):
        return self.reason_impl()


    def __hash__(InitialException self):
        return super().__hash__()

    def __repr__(InitialException self):
        return super().__repr__()

    def __str__(InitialException self):
        return super().__str__()


    def __copy__(InitialException self):
        cdef shared_ptr[_module_cbindings.cInitialException] cpp_obj = make_shared[_module_cbindings.cInitialException](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return InitialException._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cInitialException](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<InitialException>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__InitialException()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.ExceptionMetadata[_module_cbindings.cInitialException].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.InitialException"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cInitialException](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(InitialException self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cInitialException](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(InitialException self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cInitialException]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cInitialException](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.InitialException,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.InitialException, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class SinkException1(thrift.py3.exceptions.GeneratedError):
    __module__ = _fbthrift__module_name__

    def __init__(SinkException1 self, *args, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkException1]()
        self._fields_setter = _fbthrift_types_fields.__SinkException1_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__( *args, **kwargs)

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("SinkException1", {
          "reason": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cSinkException1] cpp_obj):
        __fbthrift_inst = <SinkException1>SinkException1.__new__(SinkException1, (<bytes>deref(cpp_obj).what()).decode('utf-8'))
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        _builtins.Exception.__init__(__fbthrift_inst, *(v for _, v in __fbthrift_inst))
        return __fbthrift_inst

    cdef inline reason_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().value()).decode('UTF-8')

    @property
    def reason(self):
        return self.reason_impl()


    def __hash__(SinkException1 self):
        return super().__hash__()

    def __repr__(SinkException1 self):
        return super().__repr__()

    def __str__(SinkException1 self):
        return super().__str__()


    def __copy__(SinkException1 self):
        cdef shared_ptr[_module_cbindings.cSinkException1] cpp_obj = make_shared[_module_cbindings.cSinkException1](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return SinkException1._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cSinkException1](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<SinkException1>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__SinkException1()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.ExceptionMetadata[_module_cbindings.cSinkException1].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.SinkException1"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cSinkException1](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(SinkException1 self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cSinkException1](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(SinkException1 self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkException1]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cSinkException1](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.SinkException1,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.SinkException1, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class SinkException2(thrift.py3.exceptions.GeneratedError):
    __module__ = _fbthrift__module_name__

    def __init__(SinkException2 self, *args, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkException2]()
        self._fields_setter = _fbthrift_types_fields.__SinkException2_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__( *args, **kwargs)

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("SinkException2", {
          "reason": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_module_cbindings.cSinkException2] cpp_obj):
        __fbthrift_inst = <SinkException2>SinkException2.__new__(SinkException2, (<bytes>deref(cpp_obj).what()).decode('utf-8'))
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        _builtins.Exception.__init__(__fbthrift_inst, *(v for _, v in __fbthrift_inst))
        return __fbthrift_inst

    cdef inline reason_impl(self):
        return deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).reason_ref().value()

    @property
    def reason(self):
        return self.reason_impl()


    def __hash__(SinkException2 self):
        return super().__hash__()

    def __repr__(SinkException2 self):
        return super().__repr__()

    def __str__(SinkException2 self):
        return super().__str__()


    def __copy__(SinkException2 self):
        cdef shared_ptr[_module_cbindings.cSinkException2] cpp_obj = make_shared[_module_cbindings.cSinkException2](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return SinkException2._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_module_cbindings.cSinkException2](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<SinkException2>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__SinkException2()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _module_cbindings.ExceptionMetadata[_module_cbindings.cSinkException2].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.SinkException2"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_module_cbindings.cSinkException2](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(SinkException2 self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_module_cbindings.cSinkException2](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(SinkException2 self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_module_cbindings.cSinkException2]()
        with nogil:
            needed = serializer.cdeserialize[_module_cbindings.cSinkException2](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.SinkException2,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("module.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.SinkException2, self)


