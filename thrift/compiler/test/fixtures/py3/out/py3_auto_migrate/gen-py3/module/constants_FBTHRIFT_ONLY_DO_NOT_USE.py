#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/py3/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#
import thrift.py3.types

import module.types

from module.containers_FBTHRIFT_ONLY_DO_NOT_USE import (
    List__i16,
    List__i32,
    List__i64,
    List__string,
    List__SimpleStruct,
    Set__i32,
    Set__string,
    Map__string_string,
    Map__string_SimpleStruct,
    Map__string_i16,
    List__List__i32,
    Map__string_i32,
    Map__string_Map__string_i32,
    List__Set__string,
    Map__string_List__SimpleStruct,
    List__List__string,
    List__Set__i32,
    List__Map__string_string,
    List__binary,
    Set__binary,
    List__AnEnum,
    _std_unordered_map__Map__i32_i32,
    _MyType__List__i32,
    _MyType__Set__i32,
    _MyType__Map__i32_i32,
    _py3_simple_AdaptedList__List__i32,
    _py3_simple_AdaptedSet__Set__i32,
    _py3_simple_AdaptedMap__Map__i32_i32,
    Map__i32_double,
    List__Map__i32_double,
    Map__AnEnumRenamed_i32,
)

A_BOOL = True
A_BYTE = 8
THE_ANSWER = 42
A_NUMBER = 84
A_BIG_NUMBER = 102
A_REAL_NUMBER = 3.14
A_FAKE_NUMBER = 3.0
A_WORD = "Good word"
SOME_BYTES = b"bytes"
A_STRUCT = module.types.SimpleStruct(is_on=True, tiny_int=5, small_int=6, nice_sized_int=7, big_int=8, real=9.9)
EMPTY = module.types.SimpleStruct()
WORD_LIST = List__string(("the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog", ))
SOME_MAP = List__Map__i32_double((Map__i32_double( { 1: 1.1, 2: 2.2 }), Map__i32_double( { 3: 3.3 }), ))
DIGITS = Set__i32((1, 2, 3, 4, 5, ))
A_CONST_MAP = Map__string_SimpleStruct( { "simple": module.types.SimpleStruct(is_on=False, tiny_int=50, small_int=61, nice_sized_int=72, big_int=83, real=99.9) })
ANOTHER_CONST_MAP = Map__AnEnumRenamed_i32( { module.types.AnEnumRenamed.name_: 0, module.types.AnEnumRenamed.value_: 1, module.types.AnEnumRenamed.renamed_: 2 })
