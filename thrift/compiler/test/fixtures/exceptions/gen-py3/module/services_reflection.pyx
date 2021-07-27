#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from thrift.py3.reflection cimport (
  MethodSpec as __MethodSpec,
  ArgumentSpec as __ArgumentSpec,
  NumberType as __NumberType,
)

import folly.iobuf as _fbthrift_iobuf


cimport module.types as _module_types


cdef __InterfaceSpec get_reflection__Raiser(bint for_clients):
    cdef __InterfaceSpec spec = __InterfaceSpec.create(
        name="Raiser",
        annotations={
        },
    )
    spec.add_method(
        __MethodSpec.create(
            name="doBland",
            arguments=(
            ),
            result=None,
            result_kind=__NumberType.NOT_A_NUMBER,
            exceptions=(
            ),
            annotations={
            },
        )
    )
    spec.add_method(
        __MethodSpec.create(
            name="doRaise",
            arguments=(
            ),
            result=None,
            result_kind=__NumberType.NOT_A_NUMBER,
            exceptions=(
                _module_types.Banal,
                _module_types.Fiery,
                _module_types.Serious,
            ),
            annotations={
            },
        )
    )
    spec.add_method(
        __MethodSpec.create(
            name="get200",
            arguments=(
            ),
            result=str,
            result_kind=__NumberType.NOT_A_NUMBER,
            exceptions=(
            ),
            annotations={
            },
        )
    )
    spec.add_method(
        __MethodSpec.create(
            name="get500",
            arguments=(
            ),
            result=str,
            result_kind=__NumberType.NOT_A_NUMBER,
            exceptions=(
                _module_types.Fiery,
                _module_types.Banal,
                _module_types.Serious,
            ),
            annotations={
            },
        )
    )
    return spec
