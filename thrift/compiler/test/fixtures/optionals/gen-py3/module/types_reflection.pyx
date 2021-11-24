#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#


import folly.iobuf as _fbthrift_iobuf

from thrift.py3.reflection cimport (
    NumberType as __NumberType,
    StructType as __StructType,
    Qualifier as __Qualifier,
)


cimport module.types as _module_types

from thrift.py3.types cimport (
    constant_shared_ptr,
    default_inst,
)


cdef __StructSpec get_reflection__Color():
    cdef _module_types.Color defaults = _module_types.Color._fbthrift_create(
        constant_shared_ptr[_module_types.cColor](
            default_inst[_module_types.cColor]()
        )
    )
    cdef __StructSpec spec = __StructSpec._fbthrift_create(
        name="Color",
        kind=__StructType.STRUCT,
        annotations={
        },
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=1,
            name="red",
            type=float,
            kind=__NumberType.DOUBLE,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=2,
            name="green",
            type=float,
            kind=__NumberType.DOUBLE,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=3,
            name="blue",
            type=float,
            kind=__NumberType.DOUBLE,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=4,
            name="alpha",
            type=float,
            kind=__NumberType.DOUBLE,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    return spec
cdef __StructSpec get_reflection__Vehicle():
    cdef _module_types.Vehicle defaults = _module_types.Vehicle._fbthrift_create(
        constant_shared_ptr[_module_types.cVehicle](
            default_inst[_module_types.cVehicle]()
        )
    )
    cdef __StructSpec spec = __StructSpec._fbthrift_create(
        name="Vehicle",
        kind=__StructType.STRUCT,
        annotations={
        },
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=1,
            name="color",
            type=_module_types.Color,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=2,
            name="licensePlate",
            type=str,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=3,
            name="description",
            type=str,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=4,
            name="name",
            type=str,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=5,
            name="hasAC",
            type=bool,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=defaults.hasAC,
            annotations={
            },
        ),
    )
    return spec
cdef __StructSpec get_reflection__Person():
    cdef _module_types.Person defaults = _module_types.Person._fbthrift_create(
        constant_shared_ptr[_module_types.cPerson](
            default_inst[_module_types.cPerson]()
        )
    )
    cdef __StructSpec spec = __StructSpec._fbthrift_create(
        name="Person",
        kind=__StructType.STRUCT,
        annotations={
        },
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=1,
            name="id",
            type=int,
            kind=__NumberType.I64,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=2,
            name="name",
            type=str,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.UNQUALIFIED,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=3,
            name="age",
            type=int,
            kind=__NumberType.I16,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=4,
            name="address",
            type=str,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=5,
            name="favoriteColor",
            type=_module_types.Color,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=6,
            name="friends",
            type=_module_types.Set__i64,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=7,
            name="bestFriend",
            type=int,
            kind=__NumberType.I64,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=8,
            name="petNames",
            type=_module_types.Map__Animal_string,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=9,
            name="afraidOfAnimal",
            type=_module_types.Animal,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    spec.add_field(
        __FieldSpec._fbthrift_create(
            id=10,
            name="vehicles",
            type=_module_types.List__Vehicle,
            kind=__NumberType.NOT_A_NUMBER,
            qualifier=__Qualifier.OPTIONAL,
            default=None,
            annotations={
            },
        ),
    )
    return spec
cdef __SetSpec get_reflection__Set__i64():
    return __SetSpec._fbthrift_create(
        value=int,
        kind=__NumberType.I64,
     )

cdef __MapSpec get_reflection__Map__Animal_string():
    return __MapSpec._fbthrift_create(
        key=_module_types.Animal,
        key_kind=__NumberType.NOT_A_NUMBER,
        value=str,
        value_kind=__NumberType.NOT_A_NUMBER,
    )

cdef __ListSpec get_reflection__List__Vehicle():
    return __ListSpec._fbthrift_create(
        value=_module_types.Vehicle,
        kind=__NumberType.NOT_A_NUMBER,
    )

