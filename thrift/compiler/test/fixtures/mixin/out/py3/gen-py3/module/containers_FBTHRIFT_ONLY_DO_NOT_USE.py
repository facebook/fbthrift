#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/mixin/src/module.thrift
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

