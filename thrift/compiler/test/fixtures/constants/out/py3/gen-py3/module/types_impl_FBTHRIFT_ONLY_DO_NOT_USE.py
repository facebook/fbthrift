#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/constants/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import enum
import thrift.py3.types
import module.thrift_metadata

_fbthrift__module_name__ = "module.types"



class EmptyEnum(thrift.py3.types.CompiledEnum):

    __module__ = _fbthrift__module_name__
    __slots__ = ()

    @staticmethod
    def __get_metadata__():
        return module.thrift_metadata.gen_metadata_enum_EmptyEnum()

    @staticmethod
    def __get_thrift_name__():
        return "module.EmptyEnum"

    def _to_python(self):
        import importlib
        python_types = importlib.import_module(
            "module.thrift_types"
        )
        return python_types.EmptyEnum(self._fbthrift_value_)

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        return self._fbthrift_value_

    def __int__(self):
        return self._fbthrift_value_

    def __index__(self):
        return self._fbthrift_value_




class City(thrift.py3.types.CompiledEnum):
    NYC = 0
    MPK = 1
    SEA = 2
    LON = 3

    __module__ = _fbthrift__module_name__
    __slots__ = ()

    @staticmethod
    def __get_metadata__():
        return module.thrift_metadata.gen_metadata_enum_City()

    @staticmethod
    def __get_thrift_name__():
        return "module.City"

    def _to_python(self):
        import importlib
        python_types = importlib.import_module(
            "module.thrift_types"
        )
        return python_types.City(self._fbthrift_value_)

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        return self._fbthrift_value_

    def __int__(self):
        return self._fbthrift_value_

    def __index__(self):
        return self._fbthrift_value_




class Company(thrift.py3.types.CompiledEnum):
    FACEBOOK = 0
    WHATSAPP = 1
    OCULUS = 2
    INSTAGRAM = 3

    __module__ = _fbthrift__module_name__
    __slots__ = ()

    @staticmethod
    def __get_metadata__():
        return module.thrift_metadata.gen_metadata_enum_Company()

    @staticmethod
    def __get_thrift_name__():
        return "module.Company"

    def _to_python(self):
        import importlib
        python_types = importlib.import_module(
            "module.thrift_types"
        )
        return python_types.Company(self._fbthrift_value_)

    def _to_py3(self):
        return self

    def _to_py_deprecated(self):
        return self._fbthrift_value_

    def __int__(self):
        return self._fbthrift_value_

    def __index__(self):
        return self._fbthrift_value_





class __union1Type(enum.Enum):
    i = 1
    d = 2
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()


class __union2Type(enum.Enum):
    i = 1
    d = 2
    s = 3
    u = 4
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

