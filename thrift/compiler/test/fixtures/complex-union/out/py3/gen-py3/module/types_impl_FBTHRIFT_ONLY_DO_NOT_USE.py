#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/complex-union/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import enum
import thrift.py3.types
import module.thrift_enums as _fbthrift_python_enums

_fbthrift__module_name__ = "module.types"




class __ComplexUnionType(enum.Enum):
    intValue = 1
    stringValue = 5
    intListValue = 2
    stringListValue = 3
    typedefValue = 9
    stringRef = 14
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

class __ListUnionType(enum.Enum):
    intListValue = 2
    stringListValue = 3
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

class __DataUnionType(enum.Enum):
    binaryData = 1
    stringData = 2
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

class __ValUnionType(enum.Enum):
    v1 = 1
    v2 = 2
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

class __VirtualComplexUnionType(enum.Enum):
    thingOne = 1
    thingTwo = 2
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()

class __NonCopyableUnionType(enum.Enum):
    s = 1
    EMPTY = 0

    __module__ = _fbthrift__module_name__
    __slots__ = ()
