from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from thrift.Thrift import TType

def fix_spec(all_structs):
    for s in all_structs:
        spec = s.thrift_spec
        for t in spec:
            if t is None:
                continue
            elif t[1] == TType.STRUCT:
                t[3][1] = t[3][0].thrift_spec
            elif t[1] in (TType.LIST, TType.SET):
                _fix_list_or_set(t[3])
            elif t[1] == TType.MAP:
                _fix_map(t[3])

def _fix_list_or_set(element_type):
    if element_type[0] == TType.STRUCT:
        element_type[1][1] = element_type[1][0].thrift_spec
    elif element_type[0] in (TType.LIST, TType.SET):
        _fix_list_or_set(element_type[1])
    elif element_type[0] == TType.MAP:
        _fix_map(element_type[1])

def _fix_map(element_type):
    if element_type[0] == TType.STRUCT:
        element_type[1][1] = element_type[1][0].thrift_spec
    elif element_type[0] in (TType.LIST, TType.SET):
        _fix_list_or_set(element_type[1])
    elif element_type[0] == TType.MAP:
        _fix_map(element_type[1])

    if element_type[2] == TType.STRUCT:
        element_type[3][1] = element_type[3][0].thrift_spec
    elif element_type[2] in (TType.LIST, TType.SET):
        _fix_list_or_set(element_type[3])
    elif element_type[2] == TType.MAP:
        _fix_map(element_type[3])
