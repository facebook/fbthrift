#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/namespace_from_package/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import thrift.py3.types
import importlib
from collections.abc import Sequence, Set

"""
    This is a helper module to define py3 container types.
    All types defined here are re-exported in the parent `.types` module.
    Only `import` types defined here via the parent `.types` module.
    If you `import` them directly from here, you will get nasty import errors.
"""

_fbthrift__module_name__ = "test.namespace_from_package.module.types"

import test.namespace_from_package.module.types as _test_namespace_from_package_module_types

def get_types_reflection():
    return importlib.import_module(
        "test.namespace_from_package.module.types_reflection"
    )

__all__ = []

