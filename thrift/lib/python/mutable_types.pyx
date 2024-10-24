# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from libcpp.memory cimport make_unique
from libcpp.utility cimport move as std_move
from folly.iobuf cimport from_unique_ptr

from cpython.object cimport PyCallable_Check
from cpython.ref cimport Py_INCREF, Py_DECREF
from cpython.tuple cimport PyTuple_New, PyTuple_SET_ITEM
from cpython.unicode cimport PyUnicode_AsUTF8String

import enum
import copy

from thrift.python.mutable_exceptions cimport MutableGeneratedError
from thrift.python.mutable_serializer cimport c_mutable_serialize, c_mutable_deserialize
from thrift.python.mutable_typeinfos cimport (
    MutableListTypeInfo,
    MutableSetTypeInfo,
    MutableMapTypeInfo,
)
from thrift.python.types cimport (
    AdaptedTypeInfo,
    FieldInfo,
    StringTypeInfo,
    TypeInfoBase,
    getCTypeInfo,
    _fbthrift_compare_struct_less,
    _validate_union_init_kwargs,
)

from cython.operator cimport dereference as deref

import cython

cdef extern from "<Python.h>":
    cdef const char * PyUnicode_AsUTF8(object unicode)

@cython.final
cdef class _ThriftListWrapper:
    def __cinit__(self, list_data):
        self._list_data = list_data

def to_thrift_list(list_data):
    return _ThriftListWrapper(list_data)


@cython.final
cdef class _ThriftSetWrapper:
    def __cinit__(self, set_data):
        self._set_data = set_data

def to_thrift_set(set_data):
    return _ThriftSetWrapper(set_data)


@cython.final
cdef class _ThriftMapWrapper:
    def __cinit__(self, map_data):
        self._map_data = map_data

def to_thrift_map(map_data):
    return _ThriftMapWrapper(map_data)


@cython.final
cdef class _ThriftContainerWrapper:
    def __cinit__(self, container_data):
        self._container_data = container_data


def fill_specs(*structured_thrift_classes):
    """
    Completes the initialization of the given Thrift-generated Struct (and
    Union) classes.

    This is called at the end of the modules that define the corresponding
    generated types (i.e., the `thrift_types.py` files), after the given classes
    have been created but not yet fully initialized. It provides support for
    dependent classes being defined in arbitrary order.

    If struct A has a field of type struct B, but the generated class A is
    defined before B, we are not able to populate the specs for A as part of the
    class creation, hence this call.

    Args:
        *structured_thrift_classes: Sequence of class objects, each one of which
            corresponds to a `MutableStruct` (i.e., created by/instance of
            `MutableStructMeta`) or a `MutableUnion` (i.e., created by/instance
            of `MutableUnionMeta`).
    """

    for cls in structured_thrift_classes:
        cls._fbthrift_fill_spec()

    for cls in structured_thrift_classes:
        if not isinstance(cls, MutableUnionMeta):
            cls._fbthrift_store_field_values()


MutableStructOrError = cython.fused_type(MutableStruct, MutableGeneratedError)

def _isset(MutableStructOrError struct):
    """
    This is an internal function that returns the 'isset byte' of the
    corresponding field. Immutable types expose this function publicly, but for
    mutable types, it remains internal and should not be accessed by user code.
    """
    cdef MutableStructInfo info = struct._fbthrift_mutable_struct_info
    isset_bytes = struct._fbthrift_data[0]
    return {
        name: bool(isset_bytes[index])
        for name, index in info.name_to_index.items()
    }


cdef _resetFieldToStandardDefault(structOrError, field_index):
    if isinstance(structOrError, MutableStruct):
        (<MutableStruct>structOrError)._fbthrift_reset_field_to_standard_default(field_index)
    else:
        (<MutableGeneratedError>structOrError)._fbthrift_reset_field_to_standard_default(field_index)


class _MutableStructField:
    """
    The `_MutableStructField` class is a descriptor class that is used to
    manage the access to a specific field in a mutable Thrift struct object.
    It uses the descriptor protocol.
    """
    # `_field_index` is the insertion order of the field in the
    # `MutableStructInfo` (this is not the Thrift field id)
    __slots__ = ('_field_index', '_is_optional')

    def __init__(self, field_id, is_optional):
        self._field_index = field_id
        self._is_optional = is_optional

    def __get__(self, obj, objtype):
        if obj is None:
            return None

        if isinstance(obj, MutableStruct):
            return (<MutableStruct>obj)._fbthrift_get_field_value(self._field_index)
        else:
            return (<MutableGeneratedError>obj)._fbthrift_get_field_value(self._field_index)

    def __set__(self, obj, value):
        if obj is None:
            return

        if value is None and self._is_optional:
            # reseting optional field to default is setting it to `None`
            _resetFieldToStandardDefault(obj, self._field_index)
            return

        if isinstance(obj, MutableStruct):
            (<MutableStruct>obj)._fbthrift_set_field_value(self._field_index, value)
        else:
            (<MutableGeneratedError>obj)._fbthrift_set_field_value(self._field_index, value)



cdef is_cacheable_non_primitive(ThriftIdlType idl_type):
    return idl_type in (ThriftIdlType.String, ThriftIdlType.Struct)


cdef is_container(ThriftIdlType idl_type):
    return idl_type in (ThriftIdlType.List, ThriftIdlType.Set, ThriftIdlType.Map)


class _MutableStructCachedField:
    __slots__ = ('_field_index', '_is_optional')

    def __init__(self, field_id, is_optional):
        self._field_index = field_id
        self._is_optional = is_optional

    def __get__(self, obj, objtype):
        if obj is None:
            return None

        if isinstance(obj, MutableStruct):
            return (<MutableStruct>obj)._fbthrift_get_cached_field_value(self._field_index)
        else:
            return (<MutableGeneratedError>obj)._fbthrift_get_cached_field_value(self._field_index)

    def __set__(self, obj, value):
        if obj is None:
            return

        if value is None and self._is_optional:
            # reseting optional field to default is setting it to `None`
            _resetFieldToStandardDefault(obj, self._field_index)
            obj._fbthrift_field_cache[self._field_index] = None
            return

        if isinstance(obj, MutableStruct):
            (<MutableStruct>obj)._fbthrift_set_field_value(self._field_index, value)
        else:
            (<MutableGeneratedError>obj)._fbthrift_set_field_value(self._field_index, value)

        obj._fbthrift_field_cache[self._field_index] = None


cdef void set_mutable_struct_field(list struct_list, int16_t index, value) except *:
    """
    Updates the given `struct_list` to have the given `value` for the field at
    the given `index`.

    The "isset" byte for the corresponding field (i.e., the `index`-th byte of
     the first element of `struct_list` is set to 1.

     Args:
        struct_list: see `createImmutableStructListWithDefaultValues()`
        index: field index, as defined by its insertion order in the parent
            `StructInfo` (this is not the field id).
        value: new value for this field, in "internal data" represntation (as
            opposed to "Python value" representation - see `*TypeInfo` classes).
    """
    setMutableStructIsset(struct_list, index, 1)
    struct_list[index + 1] = value


cdef class MutableStructOrUnion:
    cdef IOBuf _fbthrift_serialize(self, Protocol proto):
        raise NotImplementedError("Not implemented on base MutableStructOrUnion class")
    cdef uint32_t _fbthrift_deserialize(self, IOBuf buf, Protocol proto) except? 0:
        raise NotImplementedError("Not implemented on base MutableStructOrUnion class")
    cdef _fbthrift_get_field_value(self, int16_t index):
        raise NotImplementedError("Not implemented on base MutableStructOrUnion class")


def _unpickle_struct(klass, bytes data):
    cdef IOBuf iobuf = IOBuf(data)
    inst = klass.__new__(klass)
    (<MutableStruct>inst)._fbthrift_deserialize(iobuf, Protocol.COMPACT)
    return inst


cdef class MutableStruct(MutableStructOrUnion):
    """
    Base class for all generated (mutable) classes corresponding to Thrift
    struct in thrift-python.

    Instance variables:
        _fbthrift_data: "mutable struct list" that holds the "isset" flag array and
            values for all fields. See `createMutableStructListWithDefaultValues()`.

        _fbthrift_field_cache: This is a list that stores instances of a field's
            Python value. It is especially useful when creating a Python value is
            relatively expensive, such as when calling the `TypeInfo.to_python_value()`
            method. For example, in the case of adapted types, we store the Python
            value in this list to avoid repeated calls to the adapter class.
            This list also stores instances when we want to return the same instance
            multiple times. For instance, if a struct field is a Thrift `list`, we store
            the `MutableList` instance in this list. This allows us to return the same
            `MutableList` instance for all attribute accesses.
    """

    def __cinit__(self, **kwargs):
        """
        Args:
            **kwargs: names and values of the Thrift fields to set for this
                 instance. All names must match declared fields of this Thrift
                 Struct (or a `TypeError` will be raised). Values are in
                 "Python value" representation, as opposed to "internal data"
                 representation (see `*TypeInfo` classes).
        """
        self._initStructListWithValues(kwargs)
        cdef MutableStructInfo mutable_struct_info = type(self)._fbthrift_mutable_struct_info
        self._fbthrift_field_cache = [None] * len(mutable_struct_info.fields)

    def __init__(self, **kwargs):
        pass

    def __call__(self, **kwargs):
        self_copy = copy.deepcopy(self)

        for field_name, value in kwargs.items():
            if value is None:
                self_copy._fbthrift_internal_resetFieldToStandardDefault(field_name)
            else:
                setattr(self_copy, field_name, value)

        return self_copy

    def __deepcopy__(self, memo):
        return self._fbthrift_create(copy.deepcopy(self._fbthrift_data, memo))

    cdef _initStructListWithValues(self, kwargs) except *:
        cdef MutableStructInfo mutable_struct_info = self._fbthrift_mutable_struct_info

        # If no keyword arguments are provided, initialize the Struct with
        # default values.
        if not kwargs:
            self._fbthrift_data = createMutableStructListWithDefaultValues(
                mutable_struct_info.cpp_obj.get().getStructInfo()
            )
            return

        # Instantiate a list with 'None' values, then assign the provided
        # keyword arguments to the respective fields.
        self._fbthrift_data = createStructListWithNones(
            mutable_struct_info.cpp_obj.get().getStructInfo()
        )
        for name, value in kwargs.items():
            field_index = mutable_struct_info.name_to_index.get(name)
            if field_index is None:
                raise TypeError(
                    f"{type(self)} initialization error: unknown keyword argument "
                    f"'{name}'."
                )

            if value is None:
                continue

            self._fbthrift_set_field_value(field_index, value)

        # If any fields remain unset, initialize them with their respective
        # default values.
        populateMutableStructListUnsetFieldsWithDefaultValues(
                self._fbthrift_data,
                mutable_struct_info.cpp_obj.get().getStructInfo()
        )

    cdef _fbthrift_set_field_value(self, int16_t index, object value):
        cdef MutableStructInfo mutable_struct_info = self._fbthrift_mutable_struct_info
        cdef FieldInfo field_info = mutable_struct_info.fields[index]

        try:
            if field_info.adapter_info is not None:
                adapter_class, transitive_annotation = field_info.adapter_info
                value = adapter_class.to_thrift_field(
                    value,
                    field_info.id,
                    self,
                    transitive_annotation=transitive_annotation(),
                )

            set_mutable_struct_field(
                self._fbthrift_data,
                index,
                (
                    <TypeInfoBase>mutable_struct_info.type_infos[index]
                ).to_internal_data(value),
            )
        except Exception as exc:
            raise type(exc)(
                f"{type(self)}: error setting Thrift struct field "
                f"'{field_info.py_name}': {exc}"
            ) from exc


    cdef _fbthrift_get_field_value(self, int16_t index):
        cdef MutableStructInfo mutable_struct_info = self._fbthrift_mutable_struct_info
        cdef TypeInfoBase field_type_info = mutable_struct_info.type_infos[index]
        cdef FieldInfo field_info = mutable_struct_info.fields[index]

        data = self._fbthrift_data[index + 1]
        if field_info.adapter_info is not None:
            py_value = field_type_info.to_python_value(data)
            adapter_class, transitive_annotation = field_info.adapter_info
            return adapter_class.from_thrift_field(
                py_value,
                field_info.id,
                self,
                transitive_annotation=transitive_annotation(),
            )

        return field_type_info.to_python_value(data) if data is not None else None

    cdef _fbthrift_get_cached_field_value(MutableStruct self, int16_t index):
        cached = self._fbthrift_field_cache[index]
        if cached is not None:
            return cached

        value = self._fbthrift_get_field_value(index)
        self._fbthrift_field_cache[index] = value
        return value

    def __eq__(MutableStruct self, other):
        if other is self:
            return True

        if type(other) != type(self):
            return False

        for name, value in self:
            if value != getattr(other, name):
                return False

        return True

    def __lt__(self, other):
        return _fbthrift_compare_struct_less(self, other, False)

    def __le__(self, other):
        return _fbthrift_compare_struct_less(self, other, True)

    def __iter__(self):
        cdef MutableStructInfo info = self._fbthrift_mutable_struct_info
        for name in info.name_to_index:
            yield name, getattr(self, name)

    def __dir__(self):
        return dir(type(self))

    def __repr__(self):
        fields = ", ".join(f"{name}={repr(value)}" for name, value in self)
        return f"{type(self).__name__}({fields})"

    def __reduce__(self):
        return (_unpickle_struct, (type(self), b''.join(self._fbthrift_serialize(Protocol.COMPACT))))

    cdef _fbthrift_reset_field_to_standard_default(self, int16_t index):
        cdef MutableStructInfo mutable_struct_info = self._fbthrift_mutable_struct_info
        resetFieldToStandardDefault(
            self._fbthrift_data,
            mutable_struct_info.cpp_obj.get().getStructInfo(),
            index,
        )

    cdef IOBuf _fbthrift_serialize(self, Protocol proto):
        cdef MutableStructInfo info = self._fbthrift_mutable_struct_info
        return from_unique_ptr(
            std_move(
                c_mutable_serialize(deref(info.cpp_obj), self._fbthrift_data, proto)
            )
        )

    cdef uint32_t _fbthrift_deserialize(self, IOBuf buf, Protocol proto) except? 0:
        cdef MutableStructInfo info = self._fbthrift_mutable_struct_info
        cdef uint32_t length = c_mutable_deserialize(
            deref(info.cpp_obj), buf._this, self._fbthrift_data, proto
        )
        return length

    def _fbthrift_internal_resetFieldToStandardDefault(self, field_name: str):
        """
        This method is the mechanism to reset the field to its standard default
        value. For optional fields, this method sets the field to `None`.
        """
        cdef MutableStructInfo mutable_struct_info = self._fbthrift_mutable_struct_info
        field_index = mutable_struct_info.name_to_index.get(field_name)
        if field_index is None:
            raise TypeError(f"got an unexpected field_name: '{field_name}'")
        self._fbthrift_reset_field_to_standard_default(field_index)
        self._fbthrift_field_cache[field_index] = None

    @classmethod
    def _fbthrift_create(cls, data):
        cdef MutableStruct inst = cls.__new__(cls)
        inst._fbthrift_data = data
        return inst


cdef class MutableStructInfo:
    """
    Stores information for a specific Thrift Struct class.

    Instance Variables:
        fields: tuple[FieldInfo, ...] (from `_fbthrift_SPEC`).

        cpp_obj: cDynamicStructInfo for this struct.

        type_infos: Tuple whose size matches the number of fields in the Thrift
            struct. Initialized by calling `fill()`.

        name_to_index: Dict[str (field name), int (index in `fields`).].
            Initialized by calling `fill()`.
    """

    def __cinit__(self, name: str, fields: tuple[FieldInfo, ...]):
        """
        Stores information for a Thrift Struct class with the given name.

        Args:
            name (str): Name of the Thrift Struct (as specified in IDL)
            fields (Set[Tuple]): Field spec tuples. See class docstring above.
        """
        self.fields = fields
        cdef int16_t num_fields = len(fields)
        self.cpp_obj = make_unique[cDynamicStructInfo](
            PyUnicode_AsUTF8(name),
            num_fields,
            False, # isUnion
            True, # isMutable
        )
        self.type_infos = PyTuple_New(num_fields)
        self.name_to_index = {}

    cdef void fill(self) except *:
        """
        Completes the initialization of this instance by populating all
        information relative to this Struct's fields.

        Must be called exactly once, after `__cinit__()` but before any other
        method.

        Upon successful return, the following attributes are fully initialized:
          - `self.type_infos`
          - `self.name_to_index`
          - field infos in the `self.cpp_obj` (see
            `DynamicStructInfo::addFieldInfo()`)
        """

        cdef cDynamicStructInfo* dynamic_struct_info = self.cpp_obj.get()
        type_infos = self.type_infos
        for idx, field_info in enumerate(self.fields):
            # field_type_info can be a lambda function so types with dependencies
            # won't need to be defined in order, see class docstring above.
            field_type_info = field_info.type_info
            if PyCallable_Check(field_type_info):
                field_type_info = field_type_info()

            # The rest of the code assumes that all the `TypeInfo` classes extend
            # from `TypeInfoBase`. Instances are typecast to `TypeInfoBase` before
            # the `to_internal_data()` and `to_python_value()` methods are called.
            if not isinstance(field_type_info, TypeInfoBase):
                raise TypeError(f"{type(field_type_info).__name__} is not subclass of TypeInfoBase.")

            Py_INCREF(field_type_info)
            PyTuple_SET_ITEM(type_infos, idx, field_type_info)
            field_info.type_info = field_type_info
            self.name_to_index[field_info.py_name] = idx
            dynamic_struct_info.addMutableFieldInfo(
                field_info.id,
                field_info.qualifier,
                PyUnicode_AsUTF8(field_info.name),
                getCTypeInfo(field_type_info),
            )

    cdef void _initialize_default_values(self) except *:
        """
        Initializes the default values of fields in this Struct.

        Upon successful return, the field value(s) in `self.cpp_obj` are
        iniitalized (see `DynamicStructInfo::addFieldValue()`).
        """
        cdef cDynamicStructInfo* dynamic_struct_info = self.cpp_obj.get()
        for idx, field in enumerate(self.fields):
            default_value = field.default_value
            if default_value is None:
                continue
            if callable(default_value):
                default_value = default_value()

            type_info = self.type_infos[idx]
            if isinstance(type_info, AdaptedTypeInfo):
                type_info = (<AdaptedTypeInfo>type_info)._orig_type_info

            if field.idl_type == ThriftIdlType.List:
                default_value = to_thrift_list(default_value)

            default_value = (<TypeInfoBase>type_info).to_internal_data(default_value)
            dynamic_struct_info.addFieldValue(idx, default_value)


cdef object _mutable_struct_meta_new(cls, cls_name, bases, dct):
    """
    See the `MutableStructMeta.__new__()` method for documentation.
    """
    # Set[Tuple (field spec)]. See `MutableStructInfo` class docstring for the
    # contents of the field spec tuples.
    fields = dct.pop('_fbthrift_SPEC')
    dct["_fbthrift_mutable_struct_info"] = MutableStructInfo(cls_name, fields)

    # List[Tuple[int (index in fields), str (field name), int (IDL Type)]
    primitive_types = []
    # List[Tuple[int (index in fields), str (field name), int (IDL Type), Optional[AdapterInfo]]
    non_primitive_types = []

    slots = ['_fbthrift_field_cache']
    for field_index, field_info in enumerate(fields):
        slots.append(field_info.py_name)

        # if field has an adapter or is not primitive type, consider
        # as "non-primitive"
        if field_info.adapter_info is not None or not field_info.is_primitive:
            non_primitive_types.append((field_index,
                                        field_info.py_name,
                                        field_info.qualifier,
                                        field_info.idl_type,
                                        field_info.adapter_info)) 
        else:
            primitive_types.append((field_index,
                                    field_info.py_name,
                                    field_info.qualifier,
                                    field_info.idl_type))

    dct["__slots__"] = slots
    klass = type.__new__(cls, cls_name, bases, dct)

    for field_index, field_name, field_qualifier, *_ in primitive_types:
        type.__setattr__(
            klass,
            field_name,
            _MutableStructField(field_index, field_qualifier == FieldQualifier.Optional),
        )

    for field_index, field_name, field_qualifier, idl_type, adapter_info in non_primitive_types:
        field_descriptor = _MutableStructField(field_index, field_qualifier == FieldQualifier.Optional)
        if is_container(idl_type) or adapter_info is not None or is_cacheable_non_primitive(idl_type):
            field_descriptor = _MutableStructCachedField(field_index, field_qualifier == FieldQualifier.Optional)

        type.__setattr__(
            klass,
            field_name,
            field_descriptor,
        )

    return klass


class MutableStructMeta(type):
    """Metaclass for all generated (mutable) thrift-python Struct types."""

    def __new__(cls, cls_name, bases, dct):
        """
        Returns a new Thrift Struct class with the given name and members.

        Args:
            cls_name (str): Name of the Thrift Struct, as specified in the
                Thrift IDL.
            bases: Unused, expected to always be empty. If not, this method
                will raise a `TypeError`.
            dct (Dict): Class members, including the SPEC for this class under
                the key '_fbthrift_SPEC'.

        Returns:
            A new class, with the given `cls_name`, corresponding to a Thrift
            Struct. The returned class inherits from `MutableStruct` and
            provides properties for all primitive and non-primitive fields
            (including any adapted fields) specified in the SPEC.

            The returned class will also have the following additional class
            attribute, meant for internal (Thrift) processing:

                _fbthrift_mutable_struct_info: MutableStructInfo
        """
        if bases:
            raise TypeError("Inheriting from thrift-python data types is forbidden: "
                           f"'{cls_name}' cannot inherit from '{bases[0].__name__}'")

        return _mutable_struct_meta_new(cls, cls_name, (MutableStruct,), dct)

    def _fbthrift_fill_spec(cls):
        """
        Completes the initialization of all specs for this Struct class.

        This should be called once, after all generated classes (including
        unions and structs) for a given module have been created.
        """
        (<MutableStructInfo>cls._fbthrift_mutable_struct_info).fill()

    def _fbthrift_store_field_values(cls):
        """
        Initializes the default values of fields (if any) for this Struct.

        This should be called once, after `_fbthrift_fill_spec()` has been
        called for all generated classes (including unions and structs) in
        a module.
        """
        (<MutableStructInfo>cls._fbthrift_mutable_struct_info)._initialize_default_values()

    def __dir__(cls):
        return tuple(name for name, _ in cls) + (
            "__iter__",
        )

    def __iter__(cls):
        """
        Iterating over Thrift-generated Struct classes yields the names of the
        fields in the struct.

        Should not be called prior to `_fbthrift_fill_spec()`.
        """
        cdef MutableStructInfo mutable_struct_info = cls._fbthrift_mutable_struct_info
        for name in mutable_struct_info.name_to_index.keys():
            yield name, None


cdef class MutableUnionInfo:
    def __cinit__(self, union_name: str, field_infos: tuple[FieldInfo, ...]):
        self.fields = field_infos
        self.cpp_obj = make_unique[cDynamicStructInfo](
            PyUnicode_AsUTF8(union_name),
            len(field_infos),
            True, # isUnion
            True, # isMutable
        )
        self.type_infos = {}
        self.id_to_adapter_info = {}
        self.name_to_index = {}

    cdef void _fill_mutable_union_info(self) except *:
        cdef cDynamicStructInfo* dynamic_struct_info = self.cpp_obj.get()
        for idx, field_info in enumerate(self.fields):
            # type_info can be a lambda function so types with dependencies
            # won't need to be defined in order
            if callable(field_info.type_info):
                field_info.type_info = field_info.type_info()
            self.type_infos[field_info.id] = field_info.type_info
            self.id_to_adapter_info[field_info.id] = field_info.adapter_info
            self.name_to_index[field_info.py_name] = idx
            dynamic_struct_info.addMutableFieldInfo(
                field_info.id,
                field_info.qualifier,
                PyUnicode_AsUTF8(field_info.name),
                getCTypeInfo(field_info.type_info)
            )


cdef object _fbthrift_compare_union_less(
    object lhs,
    object rhs,
    return_if_same_value
):
    if type(lhs) != type(rhs):
        return NotImplemented

    lhs_union = <MutableUnion>(lhs)
    rhs_union = <MutableUnion>(rhs)

    lhs_tuple = (
        lhs_union.fbthrift_current_field.value,
        lhs_union.fbthrift_current_value,
    )
    rhs_tuple = (
        rhs_union.fbthrift_current_field.value,
        rhs_union.fbthrift_current_value,
    )

    if return_if_same_value:
        return lhs_tuple <= rhs_tuple
    else:
        return lhs_tuple < rhs_tuple


cdef class MutableUnion(MutableStructOrUnion):
    def __cinit__(self):
        self._fbthrift_data = createMutableUnionDataHolder()

    def __init__(self, **kwargs):
        self_type = type(self)
        field_enum, field_python_value = _validate_union_init_kwargs(
            self_type, self_type.FbThriftUnionFieldEnum, kwargs
        )
        cdef int field_id = field_enum.value

        # If no field is specified, exit early.
        if field_id == 0:
            self._fbthrift_update_current_field_attributes()
            return

        self._fbthrift_set_mutable_union_value(field_id, field_python_value)

    cdef void _fbthrift_set_mutable_union_value(
        self, int field_id, object field_python_value
    ) except *:
        """
        Updates this union to hold the given value (corresponding to the given field).

        If the corresponding field has an adapter, the given value should be in the
        adapted type, and will be "adapted back" to the underlying Thrift type.

        Args:
            field_id: Thrift field ID of the field being set (or 0 to "clear" this union
                and mark it as empty).
            field_python_value: Value for the given field, in "python value" format (as
                opposed to "internal data", see `TypeInfoBase`). If `field_id` is 0,
                this must be `None`. If field is adapted, should be in the adapted type.
        """
        try:
            field_internal_value = (
                self._fbthrift_convert_field_python_value_to_internal_data(
                    field_id, field_python_value
                )
            )

            # For immutable types, `._fbthrift_set_union_value()` method is more
            # complicated because we use `tuple` as the internal data container.
            # Since `tuple` is immutable, to update an immutable container, we
            # use `PyTuple_SET_ITEM()`. We also need to manually handle `INCREF`
            # and `DECREF`. After switching to `list` as the internal data
            # container for mutable types, we simply update the `list` as usual.
            self._fbthrift_data[0] = field_id
            self._fbthrift_data[1] = field_internal_value

            self._fbthrift_update_current_field_attributes()
        except Exception as exc:
            raise type(exc)(
                f"{type(self)}: error setting Thrift union field with id {field_id}: "
                f"{exc}"
            ) from exc

    cdef object _fbthrift_convert_field_python_value_to_internal_data(
        self, int field_id, object field_python_value
    ):
        """
        Converts the given python value to its "internal data" representation, assuming
        it is a value for a field of this Thrift union with id `field_id`.

        If `field_id` is the special value 0 (corresponding to the case where the Thrift
        union is "empty", i.e. does not have any field set), `field_python_value` MUST
        be `None` and the returned value will always be `None`.

        If the field corresponding to `field_id` has an adapter, the given
        `field_python_value` should be in the adapted type, and will be "adapted back"
        to the underlying Thrift type.

        Raises:
            AssertionError if `field_id` is 0 (i.e., EMPTY) but
            `field_python_value` is not `None`.

            Exception if the operation cannot be completely successfully.
        """
        if field_id == 0:
            assert field_python_value is None
            return None

        cdef MutableUnionInfo union_info = <MutableUnionInfo>(
            type(self)._fbthrift_mutable_struct_info
        )

        adapter_info = union_info.id_to_adapter_info[field_id]
        if adapter_info:
            adapter_class, transitive_annotation = adapter_info
            field_python_value = adapter_class.to_thrift_field(
                field_python_value,
                field_id,
                self,
                transitive_annotation=transitive_annotation(),
            )

        cdef TypeInfoBase type_info = <TypeInfoBase>union_info.type_infos[field_id]
        return type_info.to_internal_data(field_python_value)

    cdef void _fbthrift_update_current_field_attributes(self) except *:
        """
        Updates the `fbthrift_current_*` attributes of this instance with the
        information of the current field for this union (or the corresponding field enum
        value if this union is empty).
        """
        cdef int current_field_enum_value = self._fbthrift_data[0]
        self.fbthrift_current_field  = type(self).FbThriftUnionFieldEnum(
            current_field_enum_value
        )
        self.fbthrift_current_value = self._fbthrift_get_current_field_python_value(
            current_field_enum_value
        )

    cdef object _fbthrift_get_current_field_python_value(
        self, int current_field_enum_value
    ):
        """
        Returns the current value for this union, in "python" format (as opposed to
        "internal data", see `TypeInfoBase`).

        This method DOES NOT handle adapted fields, i.e. if the current field has an
        adapter, the returned value will NOT be the adapted value, but rather that of
        the underlying Thrift type.

        Args:
            current_field_enum_value: the field ID of the current field, read from
                `self._fbthrift_data[0]` and passed in to avoid unnecessarily reading it
                again.
        """
        field_internal_data = self._fbthrift_data[1]

        if current_field_enum_value == 0:
            assert field_internal_data is None

        # DO_BEFORE(aristidis,20240701): Determine logic for accessing adapted fields
        # in unions (there seems to be a discrepancy with structs, see 
        # `MutableStruct._fbthrift_get_field_value()`).
        if field_internal_data is None:
            return None

        cdef MutableUnionInfo union_info = (
            <MutableUnionInfo>type(self)._fbthrift_mutable_struct_info
        )
        cdef TypeInfoBase type_info = (
            <TypeInfoBase>union_info.type_infos[current_field_enum_value]
        )
        return type_info.to_python_value(field_internal_data)


    cdef object _fbthrift_get_field_value(self, int16_t field_id):
        """
        Returns the value of the field with the given `field_id` if it is indeed the
        field that is (currently) set for this union.

        This method DOES NOT handle adapted fields, i.e. if the field corresponding to
        the given `field_id` has an adapter (and satisfies the conditions above), the
        returned value will NOT be the adapted value, but rather that of the underlying
        Thrift type (in "python value" format, not internal data).

        Raises:
            ValueError if `field_id` does not correspond to a valid field id for this
                Thrift union.

            AttributeError if this union does not currently hold a value for the given
                `field_id` (i.e., it either holds a value for another field, or is
                empty).
        """
        current_field_enum = self.fbthrift_current_field
        cdef int current_field_enum_value = current_field_enum.value
        if (current_field_enum_value == field_id):
            return self.fbthrift_current_value

        # ERROR: Requested field_id does not match current field.
        union_class = type(self)
        requested_field_enum = union_class.FbThriftUnionFieldEnum(field_id)
        raise AttributeError(
            f"Error retrieving Thrift union ({union_class.__name__}) field: requested "
            f"'{requested_field_enum.name}', but currently holds "
            f"'{current_field_enum.name}'."
        )

    def __copy__(self):
        return self._fbthrift_create(copy.copy(self._fbthrift_data))

    @classmethod
    def _fbthrift_create(cls, data):
        cdef MutableUnion inst = cls.__new__(cls)
        inst._fbthrift_data = data
        inst._fbthrift_update_current_field_attributes()
        return inst

    __hash__ = None

    def __eq__(MutableUnion self, other):
        if other is self:
            return True

        if type(other) != type(self):
            return False

        other_union = <MutableUnion>(other)
        if other_union.fbthrift_current_field != self.fbthrift_current_field:
            return False

        return other_union.fbthrift_current_value == self.fbthrift_current_value

    def __lt__(self, other):
        return _fbthrift_compare_union_less(
            self,
            other,
            False, # return_if_same_value
        )

    def __le__(self, other):
        return _fbthrift_compare_union_less(
            self,
            other,
            True, # return_if_same_value
        )

    cdef IOBuf _fbthrift_serialize(self, Protocol proto):
        cdef MutableUnionInfo info = self._fbthrift_mutable_struct_info
        return from_unique_ptr(
            std_move(c_mutable_serialize(deref(info.cpp_obj), self._fbthrift_data, proto))
        )

    cdef uint32_t _fbthrift_deserialize(self, IOBuf buf, Protocol proto) except? 0:
        cdef MutableUnionInfo info = self._fbthrift_mutable_struct_info
        cdef uint32_t size = c_mutable_deserialize(deref(info.cpp_obj), buf._this, self._fbthrift_data, proto)
        self._fbthrift_update_current_field_attributes()
        return size

    def __dir__(self):
        return dir(type(self))

    def get_type(self):
        return self.fbthrift_current_field

    def __repr__(self):
        return f"{type(self).__name__}({self.fbthrift_current_field.name}={self.fbthrift_current_value!r})"

def _gen_mutable_union_field_enum_members(field_infos):
    yield ("EMPTY", 0)
    for f in field_infos:
        yield (f.py_name, f.id)


cdef class _MutableUnionFieldDescriptor:
    """Descriptor for a field of a (mutable) Thrift Union. """

    def __cinit__(self, FieldInfo field_info):
        """
        Args:
            field_info (FieldInfo): of the target field in the Thrift union that will be
                exposed by this descriptor.
        """
        self._field_info = field_info

    def __get__(self, union_instance, unused_union_class):
        """
        Returns the value of the Thrift union field corresponding to this descriptor,
        for the given `union_instance`, iff it is currently set.

        On success, the returned value will be the adapted (if needed) value of this
        field in "python value" format (as opposed to "internal data", see
        `TypeInfoBase`).

        Args:
            union_instance (MutableUnion): the Thrift union instance whose field
                (corresponding to `self._field_info`) is being retrieved.

        Raises error if this field is not currently set on the given `union_instance`.
        """
        if union_instance is None:
            return None

        field_info = self._field_info
        cdef int field_id = self._field_info.id

        field_python_value = (
            (<MutableUnion>union_instance)._fbthrift_get_field_value(field_id)
        )

        adapter_info = field_info.adapter_info
        if not adapter_info:
            return field_python_value

        adapter_class, transitive_annotation = adapter_info
        return adapter_class.from_thrift_field(
            field_python_value,
            field_id,
            union_instance,
            transitive_annotation=transitive_annotation(),
        )

    def __set__(self, union_instance, field_python_value):
        """
        Sets the field corresponding to this descriptor on the given `union_instance`,
        with the given value.

        Args:
            union_instance (MutableUnion): the Thrift union instance on which to set the
                field (corresponding to `self._field_info`).
            field_python_value (object): The value to set for this field, given in
                "python value" format (as opposed to "internal data", see
                `TypeInfoBase`). If this field is adapted, this value should be in the
                adapted type (as opposed to the underlying Thrift type).
        """
        if union_instance is None:
            return

        field_info = self._field_info
        cdef int field_id = self._field_info.id

        # NOTE: ._fbthrift_set_mutable_union_value() handles adapted fields, i.e. it
        # takes care of "adapting back" `field_python_value` from the adapted type to
        # the underlying Thrift type if needed.
        (<MutableUnion>union_instance)._fbthrift_set_mutable_union_value(
            field_id, field_python_value
        )


class MutableUnionMeta(type):
    """Metaclass for all generated (mutable) thrift-python Union types."""

    def __new__(cls, union_name, bases, union_class_namespace):
        """
        Returns a new Thrift Union class with the given name and members.
        """
        if bases:
            raise TypeError(
                "Inheriting from thrift-python data types is forbidden: "
                f"'{union_name}' cannot inherit from '{bases[0].__name__}'"
            )

        field_infos = union_class_namespace.pop('_fbthrift_SPEC')
        num_fields = len(field_infos)

        union_class_namespace["_fbthrift_mutable_struct_info"] = MutableUnionInfo(
            union_name, field_infos
        )

        union_class_namespace["FbThriftUnionFieldEnum"] = enum.Enum(
            f"FbThriftUnionFieldEnum_{union_name}",
            _gen_mutable_union_field_enum_members(field_infos)
        )

        slots = [field_info.py_name for field_info in field_infos]
        union_class_namespace["__slots__"] = slots

        type_obj = super().__new__(cls, union_name, (MutableUnion,), union_class_namespace)

        for field_info in field_infos:
            type.__setattr__(
                type_obj,
                field_info.py_name,
                _MutableUnionFieldDescriptor(field_info),
            )

        return type_obj

    def _fbthrift_fill_spec(cls):
        (<MutableUnionInfo>cls._fbthrift_mutable_struct_info)._fill_mutable_union_info()

    def __dir__(cls):
        return (
            tuple((<MutableUnionInfo>cls._fbthrift_mutable_struct_info).name_to_index.keys())
            +
            ('fbthrift_current_field', 'fbthrift_current_value')
        )
