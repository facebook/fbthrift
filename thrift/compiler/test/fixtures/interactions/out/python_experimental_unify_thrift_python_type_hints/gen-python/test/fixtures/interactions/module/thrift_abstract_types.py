
# EXPERIMENTAL - DO NOT USE !!!
# See `experimental_unify_thrift_python_type_hints` documentation in thrift compiler

#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import abc as _abc
import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import test.fixtures.another_interactions.shared.thrift_abstract_types


class CustomException(_abc.ABC):
    @property
    @_abc.abstractmethod
    def message(self) -> str: ...
#    @_abc.abstractmethod
#    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str]]]: ...
#    @_abc.abstractmethod
#    def _to_python(self) -> "test.fixtures.interactions.module.thrift_types.CustomException": ...  # type: ignore
#    @_abc.abstractmethod
#    def _to_py3(self) -> "test.fixtures.interactions.module.types.CustomException": ...  # type: ignore
#    @_abc.abstractmethod
#    def _to_py_deprecated(self) -> "test.fixtures.interactions.ttypes.CustomException": ...  # type: ignore