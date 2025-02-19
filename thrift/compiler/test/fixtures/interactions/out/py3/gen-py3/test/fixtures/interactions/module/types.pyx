#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/interactions/src/module.thrift
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
import asyncio
from folly.coro cimport bridgeCoroTaskWith
cimport test.fixtures.another_interactions.shared.types as _test_fixtures_another_interactions_shared_types
import test.fixtures.another_interactions.shared.types as _test_fixtures_another_interactions_shared_types

import test.fixtures.interactions.module.thrift_types as _fbthrift_python_types


_fbthrift__module_name__ = "test.fixtures.interactions.module.types"

cdef object get_types_reflection():
    return importlib.import_module(
        "test.fixtures.interactions.module.types_reflection"
    )

@__cython.auto_pickle(False)
@__cython.final
cdef class CustomException(thrift.py3.exceptions.GeneratedError):
    __module__ = _fbthrift__module_name__

    def __init__(CustomException self, *args, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_test_fixtures_interactions_module_cbindings.cCustomException]()
        self._fields_setter = _fbthrift_types_fields.__CustomException_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__( *args, **kwargs)

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("CustomException", {
          "message": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).message_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_test_fixtures_interactions_module_cbindings.cCustomException] cpp_obj):
        __fbthrift_inst = <CustomException>CustomException.__new__(CustomException, (<bytes>deref(cpp_obj).what()).decode('utf-8'))
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        _builtins.Exception.__init__(__fbthrift_inst, *(v for _, v in __fbthrift_inst))
        return __fbthrift_inst

    cdef inline message_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).message_ref().value()).decode('UTF-8')

    @property
    def message(self):
        return self.message_impl()


    def __hash__(CustomException self):
        return super().__hash__()

    def __repr__(CustomException self):
        return super().__repr__()

    def __str__(CustomException self):
        return super().__str__()


    def __copy__(CustomException self):
        cdef shared_ptr[_test_fixtures_interactions_module_cbindings.cCustomException] cpp_obj = make_shared[_test_fixtures_interactions_module_cbindings.cCustomException](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return CustomException._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_test_fixtures_interactions_module_cbindings.cCustomException](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<CustomException>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__CustomException()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _test_fixtures_interactions_module_cbindings.ExceptionMetadata[_test_fixtures_interactions_module_cbindings.cCustomException].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.CustomException"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_test_fixtures_interactions_module_cbindings.cCustomException](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(CustomException self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_test_fixtures_interactions_module_cbindings.cCustomException](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(CustomException self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_test_fixtures_interactions_module_cbindings.cCustomException]()
        with nogil:
            needed = serializer.cdeserialize[_test_fixtures_interactions_module_cbindings.cCustomException](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.CustomException,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("test.fixtures.interactions.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.CustomException, self)

@__cython.auto_pickle(False)
@__cython.final
cdef class ShouldBeBoxed(thrift.py3.types.Struct):
    __module__ = _fbthrift__module_name__

    def __init__(ShouldBeBoxed self, **kwargs):
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed]()
        self._fields_setter = _fbthrift_types_fields.__ShouldBeBoxed_FieldsSetter._fbthrift_create(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        super().__init__(**kwargs)

    def __call__(ShouldBeBoxed self, **kwargs):
        if not kwargs:
            return self
        cdef ShouldBeBoxed __fbthrift_inst = ShouldBeBoxed.__new__(ShouldBeBoxed)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE))
        __fbthrift_inst._fields_setter = _fbthrift_types_fields.__ShouldBeBoxed_FieldsSetter._fbthrift_create(__fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get())
        for __fbthrift_name, _fbthrift_value in kwargs.items():
            (<thrift.py3.types.Struct>__fbthrift_inst)._fbthrift_set_field(__fbthrift_name, _fbthrift_value)
        return __fbthrift_inst

    cdef void _fbthrift_set_field(self, str name, object value) except *:
        self._fields_setter.set_field(name.encode("utf-8"), value)

    cdef object _fbthrift_isset(self):
        return _fbthrift_IsSet("ShouldBeBoxed", {
          "sessionId": deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).sessionId_ref().has_value(),
        })

    @staticmethod
    cdef _create_FBTHRIFT_ONLY_DO_NOT_USE(shared_ptr[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed] cpp_obj):
        __fbthrift_inst = <ShouldBeBoxed>ShouldBeBoxed.__new__(ShouldBeBoxed)
        __fbthrift_inst._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = cmove(cpp_obj)
        return __fbthrift_inst

    cdef inline sessionId_impl(self):
        return (<bytes>deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE).sessionId_ref().value()).decode('UTF-8')

    @property
    def sessionId(self):
        return self.sessionId_impl()


    def __hash__(ShouldBeBoxed self):
        return super().__hash__()

    def __repr__(ShouldBeBoxed self):
        return super().__repr__()

    def __str__(ShouldBeBoxed self):
        return super().__str__()


    def __copy__(ShouldBeBoxed self):
        cdef shared_ptr[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed] cpp_obj = make_shared[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](
            deref(self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE)
        )
        return ShouldBeBoxed._create_FBTHRIFT_ONLY_DO_NOT_USE(cmove(cpp_obj))

    def __richcmp__(self, other, int op):
        r = self._fbthrift_cmp_sametype(other, op)
        return __richcmp[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](
            self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            (<ShouldBeBoxed>other)._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE,
            op,
        ) if r is None else r

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__ShouldBeBoxed()

    @staticmethod
    def __get_metadata__():
        cdef __fbthrift_cThriftMetadata meta
        _test_fixtures_interactions_module_cbindings.StructMetadata[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed].gen(meta)
        return __MetadataBox.box(cmove(meta))

    @staticmethod
    def __get_thrift_name__():
        return "module.ShouldBeBoxed"

    @classmethod
    def _fbthrift_get_field_name_by_index(cls, idx):
        return __sv_to_str(__get_field_name_by_index[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](idx))

    @classmethod
    def _fbthrift_get_struct_size(cls):
        return 1

    cdef _fbthrift_iobuf.IOBuf _fbthrift_serialize(ShouldBeBoxed self, __Protocol proto):
        cdef unique_ptr[_fbthrift_iobuf.cIOBuf] data
        with nogil:
            data = cmove(serializer.cserialize[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto))
        return _fbthrift_iobuf.from_unique_ptr(cmove(data))

    cdef cuint32_t _fbthrift_deserialize(ShouldBeBoxed self, const _fbthrift_iobuf.cIOBuf* buf, __Protocol proto) except? 0:
        cdef cuint32_t needed
        self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE = make_shared[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed]()
        with nogil:
            needed = serializer.cdeserialize[_test_fixtures_interactions_module_cbindings.cShouldBeBoxed](buf, self._cpp_obj_FBTHRIFT_ONLY_DO_NOT_USE.get(), proto)
        return needed


    def _to_python(self):
        return thrift.python.converter.to_python_struct(
            _fbthrift_python_types.ShouldBeBoxed,
            self,
        )

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        import thrift.util.converter
        py_deprecated_types = importlib.import_module("test.fixtures.interactions.ttypes")
        return thrift.util.converter.to_py_struct(py_deprecated_types.ShouldBeBoxed, self)



cdef class ClientBufferedStream__bool(ClientBufferedStream):

    @staticmethod
    cdef _fbthrift_create(cClientBufferedStream[cbool]& c_obj, __RpcOptions rpc_options):
        __fbthrift_inst = ClientBufferedStream__bool(rpc_options)
        __fbthrift_inst._gen = make_unique[cClientBufferedStreamWrapper[cbool]](c_obj)
        return __fbthrift_inst

    @staticmethod
    cdef void callback(
        cFollyTry[__cOptional[cbool]]&& result,
        PyObject* userdata,
    ) noexcept:
        cdef __cOptional[cbool] opt_val
        cdef cbool _value
        stream, pyfuture, rpc_options = <object> userdata
        if result.hasException():
            pyfuture.set_exception(
                thrift.python.exceptions.create_py_exception(result.exception(), <__RpcOptions>rpc_options)
            )
        else:
            opt_val = result.value()
            if opt_val.has_value():
                try:
                    _value = opt_val.value()
                    pyfuture.set_result(<bint>_value)
                except Exception as ex:
                    pyfuture.set_exception(ex.with_traceback(None))
            else:
                pyfuture.set_exception(StopAsyncIteration())

    def __anext__(self):
        __loop = asyncio.get_event_loop()
        __future = __loop.create_future()
        # to avoid "Future exception was never retrieved" error at SIGINT
        __future.add_done_callback(lambda x: x.exception())
        __userdata = (self, __future, self._rpc_options)
        bridgeCoroTaskWith[__cOptional[cbool]](
            self._executor,
            deref(self._gen).getNext(),
            ClientBufferedStream__bool.callback,
            <PyObject *>__userdata,
        )
        return asyncio.shield(__future)

cdef class ServerStream__bool(ServerStream):
    pass

cdef class ClientBufferedStream__i32(ClientBufferedStream):

    @staticmethod
    cdef _fbthrift_create(cClientBufferedStream[cint32_t]& c_obj, __RpcOptions rpc_options):
        __fbthrift_inst = ClientBufferedStream__i32(rpc_options)
        __fbthrift_inst._gen = make_unique[cClientBufferedStreamWrapper[cint32_t]](c_obj)
        return __fbthrift_inst

    @staticmethod
    cdef void callback(
        cFollyTry[__cOptional[cint32_t]]&& result,
        PyObject* userdata,
    ) noexcept:
        cdef __cOptional[cint32_t] opt_val
        cdef cint32_t _value
        stream, pyfuture, rpc_options = <object> userdata
        if result.hasException():
            pyfuture.set_exception(
                thrift.python.exceptions.create_py_exception(result.exception(), <__RpcOptions>rpc_options)
            )
        else:
            opt_val = result.value()
            if opt_val.has_value():
                try:
                    _value = opt_val.value()
                    pyfuture.set_result(_value)
                except Exception as ex:
                    pyfuture.set_exception(ex.with_traceback(None))
            else:
                pyfuture.set_exception(StopAsyncIteration())

    def __anext__(self):
        __loop = asyncio.get_event_loop()
        __future = __loop.create_future()
        # to avoid "Future exception was never retrieved" error at SIGINT
        __future.add_done_callback(lambda x: x.exception())
        __userdata = (self, __future, self._rpc_options)
        bridgeCoroTaskWith[__cOptional[cint32_t]](
            self._executor,
            deref(self._gen).getNext(),
            ClientBufferedStream__i32.callback,
            <PyObject *>__userdata,
        )
        return asyncio.shield(__future)

cdef class ServerStream__i32(ServerStream):
    pass

cdef class ResponseAndClientBufferedStream__i32_i32(ResponseAndClientBufferedStream):

    @staticmethod
    cdef _fbthrift_create(cResponseAndClientBufferedStream[cint32_t, cint32_t]& c_obj, __RpcOptions rpc_options):
        __fbthrift_inst = ResponseAndClientBufferedStream__i32_i32()
        __fbthrift_inst._stream = ClientBufferedStream__i32._fbthrift_create(
            c_obj.stream, rpc_options,
        )
        cdef cint32_t _value = c_obj.response
        __fbthrift_inst._response = _value
        return __fbthrift_inst

    def __iter__(self):
        yield self._response
        yield self._stream

cdef class ResponseAndServerStream__i32_i32(ResponseAndServerStream):
    pass

