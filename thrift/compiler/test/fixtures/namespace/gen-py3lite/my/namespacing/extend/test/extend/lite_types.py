#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#
import thrift.py3lite.types as _fbthrift_py3lite_types
import thrift.py3lite.exceptions as _fbthrift_py3lite_exceptions

import my.namespacing.test.hsmodule.lite_types


class _fbthrift_ExtendTestService_check_args(metaclass=_fbthrift_py3lite_types.StructMeta):
    _fbthrift_SPEC = (
        (
            1,  # id
            True,  # isUnqualified
            "struct1",  # name
            lambda: _fbthrift_py3lite_types.StructTypeInfo(my.namespacing.test.hsmodule.lite_types.HsFoo),  # typeinfo
            None,  # default value
        ),
    )

class _fbthrift_ExtendTestService_check_result(metaclass=_fbthrift_py3lite_types.StructMeta):
    _fbthrift_SPEC = (
        (
            0,  # id
            False,  # isUnqualified
            "success",  # name
            _fbthrift_py3lite_types.typeinfo_bool,  # typeinfo
            None,  # default value

        ),
    )


_fbthrift_py3lite_types.fill_specs(
    _fbthrift_ExtendTestService_check_args,
    _fbthrift_ExtendTestService_check_result,
)
