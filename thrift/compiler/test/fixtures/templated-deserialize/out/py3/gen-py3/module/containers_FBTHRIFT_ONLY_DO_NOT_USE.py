#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/templated-deserialize/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import thrift.py3.types
import importlib
from collections.abc import Mapping, Sequence, Set

"""
    This is a helper module to define py3 container types.
    All types defined here are re-exported in the parent `.types` module.
    Only `import` types defined here via the parent `.types` module.
    If you `import` them directly from here, you will get nasty import errors.
"""

_fbthrift__module_name__ = "module.types"

import module.types as _module_types
from thrift.py3.types import _ensure_py3_or_raise

def get_types_reflection():
    return importlib.import_module(
        "module.types_reflection"
    )

__all__ = []

class Map__string_bool(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__string_bool):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__string_bool._check_key_type_or_raise
            check_val = Map__string_bool._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__string_bool)

    @staticmethod
    def _check_key_type_or_raise(key):
        if not (
            isinstance(key, str)
        ):
            raise TypeError(f"{key!r} is not of type str")
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, str):
            return key

    @staticmethod
    def _check_val_type_or_raise(item):
        if not (
            isinstance(item, bool)
        ):
            raise TypeError(f"{item!r} is not of type bool")
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__string_bool()


Mapping.register(Map__string_bool)
__all__.append('Map__string_bool')


class Set__i32(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__i32):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            check_method = Set__i32._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__i32)

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
        return get_types_reflection().get_reflection__Set__i32()


Set.register(Set__i32)

__all__.append('Set__i32')


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


class Map__string_i32(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__string_i32):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__string_i32._check_key_type_or_raise
            check_val = Map__string_i32._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__string_i32)

    @staticmethod
    def _check_key_type_or_raise(key):
        if not (
            isinstance(key, str)
        ):
            raise TypeError(f"{key!r} is not of type str")
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, str):
            return key

    @staticmethod
    def _check_val_type_or_raise(item):
        if not (
            isinstance(item, int)
        ):
            raise TypeError(f"{item!r} is not of type int")
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__string_i32()


Mapping.register(Map__string_i32)
__all__.append('Map__string_i32')


class Map__string_Map__string_i32(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__string_Map__string_i32):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__string_Map__string_i32._check_key_type_or_raise
            check_val = Map__string_Map__string_i32._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__string_Map__string_i32)

    @staticmethod
    def _check_key_type_or_raise(key):
        if not (
            isinstance(key, str)
        ):
            raise TypeError(f"{key!r} is not of type str")
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, str):
            return key

    @staticmethod
    def _check_val_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Mapping[str, int]")
        if not isinstance(item, _module_types.Map__string_i32):
            item = _module_types.Map__string_i32(item)
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__string_Map__string_i32()


Mapping.register(Map__string_Map__string_i32)
__all__.append('Map__string_Map__string_i32')


class Map__string_Map__string_Map__string_i32(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__string_Map__string_Map__string_i32):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__string_Map__string_Map__string_i32._check_key_type_or_raise
            check_val = Map__string_Map__string_Map__string_i32._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__string_Map__string_Map__string_i32)

    @staticmethod
    def _check_key_type_or_raise(key):
        if not (
            isinstance(key, str)
        ):
            raise TypeError(f"{key!r} is not of type str")
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, str):
            return key

    @staticmethod
    def _check_val_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Mapping[str, _typing.Mapping[str, int]]")
        if not isinstance(item, _module_types.Map__string_Map__string_i32):
            item = _module_types.Map__string_Map__string_i32(item)
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__string_Map__string_Map__string_i32()


Mapping.register(Map__string_Map__string_Map__string_i32)
__all__.append('Map__string_Map__string_Map__string_i32')


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


class Map__string_List__i32(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__string_List__i32):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__string_List__i32._check_key_type_or_raise
            check_val = Map__string_List__i32._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__string_List__i32)

    @staticmethod
    def _check_key_type_or_raise(key):
        if not (
            isinstance(key, str)
        ):
            raise TypeError(f"{key!r} is not of type str")
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, str):
            return key

    @staticmethod
    def _check_val_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Sequence[int]")
        if not isinstance(item, _module_types.List__i32):
            item = _module_types.List__i32(item)
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__string_List__i32()


Mapping.register(Map__string_List__i32)
__all__.append('Map__string_List__i32')


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


class Set__bool(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__bool):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            check_method = Set__bool._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__bool)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, bool)
        ):
            raise TypeError(f"{item!r} is not of type bool")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, bool):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Set__bool()


Set.register(Set__bool)

__all__.append('Set__bool')


class Set__Set__bool(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__Set__bool):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            check_method = Set__Set__bool._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__Set__bool)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.AbstractSet[bool]")
        if not isinstance(item, _module_types.Set__bool):
            item = _module_types.Set__bool(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.Set__bool):
            return item
        try:
            return _module_types.Set__bool(item)
        except:
            return None

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Set__Set__bool()


Set.register(Set__Set__bool)

__all__.append('Set__Set__bool')


class Set__Set__Set__bool(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__Set__Set__bool):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            check_method = Set__Set__Set__bool._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__Set__Set__bool)

    @staticmethod
    def _check_item_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.AbstractSet[_typing.AbstractSet[bool]]")
        if not isinstance(item, _module_types.Set__Set__bool):
            item = _module_types.Set__Set__bool(item)
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, _module_types.Set__Set__bool):
            return item
        try:
            return _module_types.Set__Set__bool(item)
        except:
            return None

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Set__Set__Set__bool()


Set.register(Set__Set__Set__bool)

__all__.append('Set__Set__Set__bool')


class Set__List__i32(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__List__i32):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            check_method = Set__List__i32._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__List__i32)

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
            return None

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Set__List__i32()


Set.register(Set__List__i32)

__all__.append('Set__List__i32')


class Set__string(thrift.py3.types.Set):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_set_private_ctor:
            _py_obj = items
        elif isinstance(items, Set__string):
            _py_obj = frozenset(items)
        elif items is None:
            _py_obj = frozenset()
        else:
            if isinstance(items, str):
                raise TypeError("If you really want to pass a string into a _typing.AbstractSet[str] field, explicitly convert it first.")
            check_method = Set__string._check_item_type_or_raise
            _py_obj = frozenset(check_method(item) for item in items)

        super().__init__(_py_obj, Set__string)

    @staticmethod
    def _check_item_type_or_raise(item):
        if not (
            isinstance(item, str)
        ):
            raise TypeError(f"{item!r} is not of type str")
        return item

    @staticmethod
    def _check_item_type_or_none(item):
        if item is None:
            return None
        if isinstance(item, str):
            return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Set__string()


Set.register(Set__string)

__all__.append('Set__string')


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


class Map__List__Set__string_string(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__List__Set__string_string):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__List__Set__string_string._check_key_type_or_raise
            check_val = Map__List__Set__string_string._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__List__Set__string_string)

    @staticmethod
    def _check_key_type_or_raise(key):
        if key is None:
            raise TypeError("None is not of the type _typing.Sequence[_typing.AbstractSet[str]]")
        if not isinstance(key, _module_types.List__Set__string):
            key = _module_types.List__Set__string(key)
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, _module_types.List__Set__string):
            return key
        try:
            return _module_types.List__Set__string(key)
        except:
            return None

    @staticmethod
    def _check_val_type_or_raise(item):
        if not (
            isinstance(item, str)
        ):
            raise TypeError(f"{item!r} is not of type str")
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__List__Set__string_string()


Mapping.register(Map__List__Set__string_string)
__all__.append('Map__List__Set__string_string')


class Map__Set__List__i32_Map__List__Set__string_string(thrift.py3.types.Map):
    __module__ = _fbthrift__module_name__
    __slots__ = ()

    _FBTHRIFT_USE_SORTED_REPR = True

    def __init__(self, items=None, private_ctor_token=None) -> None:
        if private_ctor_token is thrift.py3.types._fbthrift_map_private_ctor:
            _py_obj = items
        elif isinstance(items, Map__Set__List__i32_Map__List__Set__string_string):
            _py_obj = dict(items)
        elif items is None:
            _py_obj = dict()
        else:
            check_key = Map__Set__List__i32_Map__List__Set__string_string._check_key_type_or_raise
            check_val = Map__Set__List__i32_Map__List__Set__string_string._check_val_type_or_raise
            _py_obj = {check_key(k) : check_val(v) for k, v in items.items()}

        super().__init__(_py_obj, Map__Set__List__i32_Map__List__Set__string_string)

    @staticmethod
    def _check_key_type_or_raise(key):
        if key is None:
            raise TypeError("None is not of the type _typing.AbstractSet[_typing.Sequence[int]]")
        if not isinstance(key, _module_types.Set__List__i32):
            key = _module_types.Set__List__i32(key)
        return key

    @staticmethod
    def _check_key_type_or_none(key):
        if key is None:
            return None
        if isinstance(key, _module_types.Set__List__i32):
            return key
        try:
            return _module_types.Set__List__i32(key)
        except:
            return None

    @staticmethod
    def _check_val_type_or_raise(item):
        if item is None:
            raise TypeError("None is not of the type _typing.Mapping[_typing.Sequence[_typing.AbstractSet[str]], str]")
        if not isinstance(item, _module_types.Map__List__Set__string_string):
            item = _module_types.Map__List__Set__string_string(item)
        return item

    @staticmethod
    def __get_reflection__():
        return get_types_reflection().get_reflection__Map__Set__List__i32_Map__List__Set__string_string()


Mapping.register(Map__Set__List__i32_Map__List__Set__string_string)
__all__.append('Map__Set__List__i32_Map__List__Set__string_string')


