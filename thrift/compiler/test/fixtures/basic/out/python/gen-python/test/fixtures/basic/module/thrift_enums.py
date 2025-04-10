#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import apache.thrift.metadata.thrift_types as _fbthrift_metadata
import thrift.python.types as _fbthrift_python_types
import typing as _std_python_typing



class MyEnum(_fbthrift_python_types.Enum, int):
    MyValue1 = 0
    MyValue2 = 1
    @staticmethod
    def __get_thrift_name__() -> str:
        return "module.MyEnum"

    @staticmethod
    def __get_thrift_uri__() -> _std_python_typing.Optional[str]:
        return "test.dev/fixtures/basic/MyEnum"

    @staticmethod
    def __get_metadata__() -> _fbthrift_metadata.ThriftMetadata:
        return gen_metadata_enum_MyEnum()

    def _to_python(self) -> "MyEnum":
        return self

    def _to_py3(self) -> "test.fixtures.basic.module.types.MyEnum": # type: ignore
        import importlib
        py3_types = importlib.import_module("test.fixtures.basic.module.types")
        return py3_types.MyEnum(self.value)

    def _to_py_deprecated(self) -> int:
        return self.value
import typing as _std_python_typing



class HackEnum(_fbthrift_python_types.Enum, int):
    Value1 = 0
    Value2 = 1
    @staticmethod
    def __get_thrift_name__() -> str:
        return "module.HackEnum"

    @staticmethod
    def __get_thrift_uri__() -> _std_python_typing.Optional[str]:
        return "test.dev/fixtures/basic/HackEnum"

    @staticmethod
    def __get_metadata__() -> _fbthrift_metadata.ThriftMetadata:
        return gen_metadata_enum_HackEnum()

    def _to_python(self) -> "HackEnum":
        return self

    def _to_py3(self) -> "test.fixtures.basic.module.types.HackEnum": # type: ignore
        import importlib
        py3_types = importlib.import_module("test.fixtures.basic.module.types")
        return py3_types.HackEnum(self.value)

    def _to_py_deprecated(self) -> int:
        return self.value

def _fbthrift_gen_metadata_enum_MyEnum(metadata_struct: _fbthrift_metadata.ThriftMetadata) -> _fbthrift_metadata.ThriftMetadata:
    qualified_name = "module.MyEnum"

    if qualified_name in metadata_struct.enums:
        return metadata_struct
    elements = {
        0: "MyValue1",
        1: "MyValue2",
    }
    structured_annotations = [
    ]
    enum_dict = dict(metadata_struct.enums)
    enum_dict[qualified_name] = _fbthrift_metadata.ThriftEnum(name=qualified_name, elements=elements, structured_annotations=structured_annotations)
    new_struct = metadata_struct(enums=enum_dict)

    return new_struct

def gen_metadata_enum_MyEnum() -> _fbthrift_metadata.ThriftMetadata:
    return _fbthrift_gen_metadata_enum_MyEnum(
        _fbthrift_metadata.ThriftMetadata(structs={}, enums={}, exceptions={}, services={})
    )

def _fbthrift_gen_metadata_enum_HackEnum(metadata_struct: _fbthrift_metadata.ThriftMetadata) -> _fbthrift_metadata.ThriftMetadata:
    qualified_name = "module.HackEnum"

    if qualified_name in metadata_struct.enums:
        return metadata_struct
    elements = {
        0: "Value1",
        1: "Value2",
    }
    structured_annotations = [
        _fbthrift_metadata.ThriftConstStruct(type=_fbthrift_metadata.ThriftStructType(name="hack.Name"), fields= { "name": _fbthrift_metadata.ThriftConstValue(cv_string="RenamedEnum"),  }),
    ]
    enum_dict = dict(metadata_struct.enums)
    enum_dict[qualified_name] = _fbthrift_metadata.ThriftEnum(name=qualified_name, elements=elements, structured_annotations=structured_annotations)
    new_struct = metadata_struct(enums=enum_dict)

    return new_struct

def gen_metadata_enum_HackEnum() -> _fbthrift_metadata.ThriftMetadata:
    return _fbthrift_gen_metadata_enum_HackEnum(
        _fbthrift_metadata.ThriftMetadata(structs={}, enums={}, exceptions={}, services={})
    )

