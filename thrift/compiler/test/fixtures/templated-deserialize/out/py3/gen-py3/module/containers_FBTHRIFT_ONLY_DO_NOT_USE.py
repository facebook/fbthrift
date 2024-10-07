#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/templated-deserialize/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import thrift.py3.types
import importlib
from collections.abc import Sequence

"""
    This is a helper module to define py3 container types.
    All types defined here are re-exported in the parent `.types` module.
    Only `import` types defined here via the parent `.types` module.
    If you `import` them directly from here, you will get nasty import errors.
"""

_fbthrift__module_name__ = "module.types"

import module.types as _module_types

def get_types_reflection():
    return importlib.import_module(
        "module.types_reflection"
    )

__all__ = []

class List__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, int)
        ):
            raise TypeError(f"{item!r} is not of type int")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, int):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__i32()


Sequence.register(List__i32)
__all__.append('List__i32')

class List__List__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__List__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__List__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__List__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Sequence[int]")
        if not isinstance(item, _module_types.List__i32):
            item = _module_types.List__i32(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.List__i32):
            return item
        try:
            return _module_types.List__i32(item)
        except:
            pass

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__List__i32()


Sequence.register(List__List__i32)
__all__.append('List__List__i32')

class List__List__List__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__List__List__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__List__List__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__List__List__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Sequence[_typing.Sequence[int]]")
        if not isinstance(item, _module_types.List__List__i32):
            item = _module_types.List__List__i32(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.List__List__i32):
            return item
        try:
            return _module_types.List__List__i32(item)
        except:
            pass

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__List__List__i32()


Sequence.register(List__List__List__i32)
__all__.append('List__List__List__i32')

class List__Set__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__Set__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__Set__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__Set__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.AbstractSet[int]")
        if not isinstance(item, _module_types.Set__i32):
            item = _module_types.Set__i32(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.Set__i32):
            return item
        try:
            return _module_types.Set__i32(item)
        except:
            pass

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__Set__i32()


Sequence.register(List__Set__i32)
__all__.append('List__Set__i32')

class List__List__List__List__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__List__List__List__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__List__List__List__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__List__List__List__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Sequence[_typing.Sequence[_typing.Sequence[int]]]")
        if not isinstance(item, _module_types.List__List__List__i32):
            item = _module_types.List__List__List__i32(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.List__List__List__i32):
            return item
        try:
            return _module_types.List__List__List__i32(item)
        except:
            pass

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__List__List__List__i32()


Sequence.register(List__List__List__List__i32)
__all__.append('List__List__List__List__i32')

class List__Set__string(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__Set__string):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__Set__string._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__Set__string)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.AbstractSet[str]")
        if not isinstance(item, _module_types.Set__string):
            item = _module_types.Set__string(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.Set__string):
            return item
        try:
            return _module_types.Set__string(item)
        except:
            pass

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__Set__string()


Sequence.register(List__Set__string)
__all__.append('List__Set__string')

class List__Foo__i64(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__Foo__i64):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__Foo__i64._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__Foo__i64)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, int)
        ):
            raise TypeError(f"{item!r} is not of type int")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, int):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__Foo__i64()


Sequence.register(List__Foo__i64)
__all__.append('List__Foo__i64')

class List__Bar__double(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__Bar__double):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__Bar__double._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__Bar__double)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, (float, int))
        ):
            raise TypeError(f"{item!r} is not of type float")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, float):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__Bar__double()


Sequence.register(List__Bar__double)
__all__.append('List__Bar__double')

class List__Baz__i32(thrift.py3.types.List):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_list_private_ctor:
            _py_obj = items
        elif isinstance(items, List__Baz__i32):
            _py_obj = list(items)
        elif items is None:
            _py_obj = []
        else:
            check_method = List__Baz__i32._check_item_type_or_raise
            _py_obj = [check_method(item) for item in items]

        super().__init__(_py_obj, List__Baz__i32)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, int)
        ):
            raise TypeError(f"{item!r} is not of type int")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, int):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__List__Baz__i32()


Sequence.register(List__Baz__i32)
__all__.append('List__Baz__i32')

