#
# Autogenerated by Thrift for shared.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import thrift.py3.builder


import test.fixtures.another_interactions.shared.types as _test_fixtures_another_interactions_shared_types


_fbthrift_struct_type__DoSomethingResult = _test_fixtures_another_interactions_shared_types.DoSomethingResult
class DoSomethingResult_Builder(thrift.py3.builder.StructBuilder):
    _struct_type = _fbthrift_struct_type__DoSomethingResult

    def __init__(self):
        self.s_res: _typing.Optional[str] = None
        self.i_res: _typing.Optional[int] = None

    def __iter__(self):
        yield "s_res", self.s_res
        yield "i_res", self.i_res
