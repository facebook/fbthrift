#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/qualified/src/module2.thrift
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

_fbthrift__module_name__ = "module2.types"

import module2.types as _module2_types
import module0.types as _module0_types
import module1.types as _module1_types

def get_types_reflection():
    return importlib.import_module(
        "module2.types_reflection"
    )

__all__ = []

