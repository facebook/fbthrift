#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

from __future__ import absolute_import
import sys
from thrift.util.Recursive import fix_spec
from thrift.Thrift import TType, TMessageType, TPriority, TRequestContext, TProcessorEventHandler, TServerInterface, TProcessor, TException, TApplicationException, UnimplementedTypedef
from thrift.protocol.TProtocol import TProtocolException

from json import loads
import sys
if sys.version_info[0] >= 3:
  long = int


import pprint
import warnings
from thrift import Thrift
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.protocol import TCompactProtocol
from thrift.protocol import THeaderProtocol
fastproto = None
try:
  from thrift.protocol import fastproto
except ImportError:
  pass

def __EXPAND_THRIFT_SPEC(spec):
    next_id = 0
    for item in spec:
        item_id = item[0]
        if next_id >= 0 and item_id < 0:
            next_id = item_id
        if item_id != next_id:
            for _ in range(next_id, item_id):
                yield None
        yield item
        next_id = item_id + 1

class ThriftEnumWrapper(int):
  def __new__(cls, enum_class, value):
    return super().__new__(cls, value)
  def __init__(self, enum_class, value):    self.enum_class = enum_class
  def __repr__(self):
    return self.enum_class.__name__ + '.' + self.enum_class._VALUES_TO_NAMES[self]

all_structs = []
UTF8STRINGS = bool(0) or sys.version_info.major >= 3

__all__ = ['UTF8STRINGS', 'Name', 'Tag', 'MinimizePadding']

class Name:
  r"""
  Attributes:
   - name
  """

  thrift_spec = None
  thrift_field_annotations = None
  thrift_struct_annotations = None
  __init__ = None
  @staticmethod
  def isUnion():
    return False

  def read(self, iprot):
    if (isinstance(iprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0)
      return
    if (isinstance(iprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2)
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.STRING:
          self.name = iprot.readString().decode('utf-8') if UTF8STRINGS else iprot.readString()
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if (isinstance(oprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0))
      return
    if (isinstance(oprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2))
      return
    oprot.writeStructBegin('Name')
    if self.name != None:
      oprot.writeFieldBegin('name', TType.STRING, 1)
      oprot.writeString(self.name.encode('utf-8')) if UTF8STRINGS and not isinstance(self.name, bytes) else oprot.writeString(self.name)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def readFromJson(self, json, is_text=True, **kwargs):
    kwargs_copy = dict(kwargs)
    relax_enum_validation = bool(kwargs_copy.pop('relax_enum_validation', False))
    set_cls = kwargs_copy.pop('custom_set_cls', set)
    dict_cls = kwargs_copy.pop('custom_dict_cls', dict)
    wrap_enum_constants = kwargs_copy.pop('wrap_enum_constants', False)
    if wrap_enum_constants and relax_enum_validation:
        raise ValueError(
            'wrap_enum_constants cannot be used together with relax_enum_validation'
        )
    if kwargs_copy:
        extra_kwargs = ', '.join(kwargs_copy.keys())
        raise ValueError(
            'Unexpected keyword arguments: ' + extra_kwargs
        )
    json_obj = json
    if is_text:
      json_obj = loads(json)
    if 'name' in json_obj and json_obj['name'] is not None:
      self.name = json_obj['name']

  def __repr__(self):
    L = []
    padding = ' ' * 4
    if self.name is not None:
      value = pprint.pformat(self.name, indent=0)
      value = padding.join(value.splitlines(True))
      L.append('    name=%s' % (value))
    return "%s(%s)" % (self.__class__.__name__, "\n" + ",\n".join(L) if L else '')

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False

    return self.__dict__ == other.__dict__ 

  def __ne__(self, other):
    return not (self == other)

  def __dir__(self):
    return (
      'name',
    )

  __hash__ = object.__hash__

  def _to_python(self):
    import importlib
    import thrift.python.converter
    python_types = importlib.import_module("facebook.thrift.annotation.go.thrift_types")
    return thrift.python.converter.to_python_struct(python_types.Name, self)

  def _to_mutable_python(self):
    import importlib
    import thrift.python.mutable_converter
    python_mutable_types = importlib.import_module("facebook.thrift.annotation.go.thrift_mutable_types")
    return thrift.python.mutable_converter.to_mutable_python_struct_or_union(python_mutable_types.Name, self)

  def _to_py3(self):
    import importlib
    import thrift.py3.converter
    py3_types = importlib.import_module("facebook.thrift.annotation.go.types")
    return thrift.py3.converter.to_py3_struct(py3_types.Name, self)

  def _to_py_deprecated(self):
    return self

class Tag:
  r"""
  Attributes:
   - tag
  """

  thrift_spec = None
  thrift_field_annotations = None
  thrift_struct_annotations = None
  __init__ = None
  @staticmethod
  def isUnion():
    return False

  def read(self, iprot):
    if (isinstance(iprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0)
      return
    if (isinstance(iprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2)
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      if fid == 1:
        if ftype == TType.STRING:
          self.tag = iprot.readString().decode('utf-8') if UTF8STRINGS else iprot.readString()
        else:
          iprot.skip(ftype)
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if (isinstance(oprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0))
      return
    if (isinstance(oprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2))
      return
    oprot.writeStructBegin('Tag')
    if self.tag != None:
      oprot.writeFieldBegin('tag', TType.STRING, 1)
      oprot.writeString(self.tag.encode('utf-8')) if UTF8STRINGS and not isinstance(self.tag, bytes) else oprot.writeString(self.tag)
      oprot.writeFieldEnd()
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def readFromJson(self, json, is_text=True, **kwargs):
    kwargs_copy = dict(kwargs)
    relax_enum_validation = bool(kwargs_copy.pop('relax_enum_validation', False))
    set_cls = kwargs_copy.pop('custom_set_cls', set)
    dict_cls = kwargs_copy.pop('custom_dict_cls', dict)
    wrap_enum_constants = kwargs_copy.pop('wrap_enum_constants', False)
    if wrap_enum_constants and relax_enum_validation:
        raise ValueError(
            'wrap_enum_constants cannot be used together with relax_enum_validation'
        )
    if kwargs_copy:
        extra_kwargs = ', '.join(kwargs_copy.keys())
        raise ValueError(
            'Unexpected keyword arguments: ' + extra_kwargs
        )
    json_obj = json
    if is_text:
      json_obj = loads(json)
    if 'tag' in json_obj and json_obj['tag'] is not None:
      self.tag = json_obj['tag']

  def __repr__(self):
    L = []
    padding = ' ' * 4
    if self.tag is not None:
      value = pprint.pformat(self.tag, indent=0)
      value = padding.join(value.splitlines(True))
      L.append('    tag=%s' % (value))
    return "%s(%s)" % (self.__class__.__name__, "\n" + ",\n".join(L) if L else '')

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False

    return self.__dict__ == other.__dict__ 

  def __ne__(self, other):
    return not (self == other)

  def __dir__(self):
    return (
      'tag',
    )

  __hash__ = object.__hash__

  def _to_python(self):
    import importlib
    import thrift.python.converter
    python_types = importlib.import_module("facebook.thrift.annotation.go.thrift_types")
    return thrift.python.converter.to_python_struct(python_types.Tag, self)

  def _to_mutable_python(self):
    import importlib
    import thrift.python.mutable_converter
    python_mutable_types = importlib.import_module("facebook.thrift.annotation.go.thrift_mutable_types")
    return thrift.python.mutable_converter.to_mutable_python_struct_or_union(python_mutable_types.Tag, self)

  def _to_py3(self):
    import importlib
    import thrift.py3.converter
    py3_types = importlib.import_module("facebook.thrift.annotation.go.types")
    return thrift.py3.converter.to_py3_struct(py3_types.Tag, self)

  def _to_py_deprecated(self):
    return self

class MinimizePadding:
  r"""
  This annotation enables reordering of fields in the generated Go structs to minimize padding.
  This is achieved by placing the fields in the order of decreasing alignments.
  The order of fields with the same alignment is preserved.
  
  ```
  @go.MinimizePadding
  struct Padded {
    1: byte small
    2: i64 big
    3: i16 medium
    4: i32 biggish
    5: byte tiny
  }
  ```
  
  For example, the Go fields for the `Padded` Thrift struct above will be generated in the following order:
  
  ```
  int64 big;
  int32 biggish;
  int16 medium;
  int8 small;
  int8 tiny;
  ```
  
  which gives the size of 16 bytes compared to 32 bytes if `go.MinimizePadding` was not specified.
  """

  thrift_spec = None
  thrift_field_annotations = None
  thrift_struct_annotations = None
  @staticmethod
  def isUnion():
    return False

  def read(self, iprot):
    if (isinstance(iprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0)
      return
    if (isinstance(iprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(iprot, THeaderProtocol.THeaderProtocolAccelerate) and iprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and isinstance(iprot.trans, TTransport.CReadableTransport) and self.thrift_spec is not None and fastproto is not None:
      fastproto.decode(self, iprot.trans, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2)
      return
    iprot.readStructBegin()
    while True:
      (fname, ftype, fid) = iprot.readFieldBegin()
      if ftype == TType.STOP:
        break
      else:
        iprot.skip(ftype)
      iprot.readFieldEnd()
    iprot.readStructEnd()

  def write(self, oprot):
    if (isinstance(oprot, TBinaryProtocol.TBinaryProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_BINARY_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=0))
      return
    if (isinstance(oprot, TCompactProtocol.TCompactProtocolAccelerated) or (isinstance(oprot, THeaderProtocol.THeaderProtocolAccelerate) and oprot.get_protocol_id() == THeaderProtocol.THeaderProtocol.T_COMPACT_PROTOCOL)) and self.thrift_spec is not None and fastproto is not None:
      oprot.trans.write(fastproto.encode(self, [self.__class__, self.thrift_spec, False], utf8strings=UTF8STRINGS, protoid=2))
      return
    oprot.writeStructBegin('MinimizePadding')
    oprot.writeFieldStop()
    oprot.writeStructEnd()

  def readFromJson(self, json, is_text=True, **kwargs):
    kwargs_copy = dict(kwargs)
    relax_enum_validation = bool(kwargs_copy.pop('relax_enum_validation', False))
    set_cls = kwargs_copy.pop('custom_set_cls', set)
    dict_cls = kwargs_copy.pop('custom_dict_cls', dict)
    wrap_enum_constants = kwargs_copy.pop('wrap_enum_constants', False)
    if wrap_enum_constants and relax_enum_validation:
        raise ValueError(
            'wrap_enum_constants cannot be used together with relax_enum_validation'
        )
    if kwargs_copy:
        extra_kwargs = ', '.join(kwargs_copy.keys())
        raise ValueError(
            'Unexpected keyword arguments: ' + extra_kwargs
        )
    json_obj = json
    if is_text:
      json_obj = loads(json)

  def __repr__(self):
    L = []
    padding = ' ' * 4
    return "%s(%s)" % (self.__class__.__name__, "\n" + ",\n".join(L) if L else '')

  def __eq__(self, other):
    if not isinstance(other, self.__class__):
      return False

    return self.__dict__ == other.__dict__ 

  def __ne__(self, other):
    return not (self == other)

  def __dir__(self):
    return (
    )

  __hash__ = object.__hash__

  def _to_python(self):
    import importlib
    import thrift.python.converter
    python_types = importlib.import_module("facebook.thrift.annotation.go.thrift_types")
    return thrift.python.converter.to_python_struct(python_types.MinimizePadding, self)

  def _to_mutable_python(self):
    import importlib
    import thrift.python.mutable_converter
    python_mutable_types = importlib.import_module("facebook.thrift.annotation.go.thrift_mutable_types")
    return thrift.python.mutable_converter.to_mutable_python_struct_or_union(python_mutable_types.MinimizePadding, self)

  def _to_py3(self):
    import importlib
    import thrift.py3.converter
    py3_types = importlib.import_module("facebook.thrift.annotation.go.types")
    return thrift.py3.converter.to_py3_struct(py3_types.MinimizePadding, self)

  def _to_py_deprecated(self):
    return self

all_structs.append(Name)
Name.thrift_spec = tuple(__EXPAND_THRIFT_SPEC((
  (1, TType.STRING, 'name', True, None, 2, ), # 1
)))

Name.thrift_struct_annotations = {
}
Name.thrift_field_annotations = {
}

def Name__init__(self, name=None,):
  self.name = name

Name.__init__ = Name__init__

def Name__setstate__(self, state):
  state.setdefault('name', None)
  self.__dict__ = state

Name.__getstate__ = lambda self: self.__dict__.copy()
Name.__setstate__ = Name__setstate__

all_structs.append(Tag)
Tag.thrift_spec = tuple(__EXPAND_THRIFT_SPEC((
  (1, TType.STRING, 'tag', True, None, 2, ), # 1
)))

Tag.thrift_struct_annotations = {
}
Tag.thrift_field_annotations = {
}

def Tag__init__(self, tag=None,):
  self.tag = tag

Tag.__init__ = Tag__init__

def Tag__setstate__(self, state):
  state.setdefault('tag', None)
  self.__dict__ = state

Tag.__getstate__ = lambda self: self.__dict__.copy()
Tag.__setstate__ = Tag__setstate__

all_structs.append(MinimizePadding)
MinimizePadding.thrift_spec = tuple(__EXPAND_THRIFT_SPEC((
)))

MinimizePadding.thrift_struct_annotations = {
}
MinimizePadding.thrift_field_annotations = {
}

fix_spec(all_structs)
del all_structs
