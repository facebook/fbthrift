from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import random
import struct

import six
import six.moves as sm
from six import StringIO

from thrift import Thrift
from thrift.util import randomizer
from thrift.protocol.TCompactProtocol import getVarint
from .THeaderTransport import THeaderTransport

class TFuzzyHeaderTransport(THeaderTransport):
    """Transport that can optionally fuzz fields in the header or payload.

    The initializer accepts `fuzz_fields`, an iterable of names of fields in
    the header (or payload) that should be fuzzed (mutated.)

    TFuzzyHeaderTransport should ONLY be used for testing purposes.
    """

    fuzzable_fields = frozenset({
        'flags',
        'header_size',
        'identity',
        'length',
        'magic',
        'num_transforms',
        'padding',
        'padding_bytes',
        'payload',
        'proto_id',
        'seq_id',
        'transform_id',
        'version',
    })

    # Randomizers for message fields by data type
    _randomizer_state = randomizer.RandomizerState()
    _i32_randomizer = _randomizer_state.get_randomizer(Thrift.TType.I32, None, {
        'fuzz_max_delta': 32
    })
    _i16_randomizer = _randomizer_state.get_randomizer(Thrift.TType.I16, None, {
        'fuzz_max_delta': 32
    })
    _byte_randomizer = _randomizer_state.get_randomizer(
        Thrift.TType.BYTE, None, {})

    @classmethod
    def _fuzz_char(cls, seed):
        """Randomly modify a (binary) character"""
        seed = six.byte2int(seed)
        res = cls._byte_randomizer.generate(seed=seed) % (2 ** 8)
        return six.int2byte(res)

    @classmethod
    def _fuzz_i16(cls, seed):
        """Randomly modify a 16-bit positive integer"""
        res = cls._i16_randomizer.generate(seed=seed)
        return res % (2 ** 16)

    @classmethod
    def _fuzz_i32(cls, seed):
        """Randomly modify a 32-bit positive integer"""
        res = cls._i32_randomizer.generate(seed=seed)
        return res % (2 ** 32)

    @classmethod
    def _fuzz_str(cls, seed):
        """Randomly modify one character of a string"""
        fuzz_index = random.randint(0, len(seed) - 1)
        fuzzed_char = six.int2byte(random.randint(0, 255))
        return seed[:fuzz_index] + fuzzed_char + seed[fuzz_index + 1:]

    # Map type names to arguments to struct.pack
    _struct_type_map = {
        'char': '!c',
        'i16': '!H',
        'i32': '!I'
    }

    @classmethod
    def _serialize_field(cls, val, type_):
        if type_ == 'str':
            return six.binary_type(val)
        if type_ in cls._struct_type_map:
            struct_type = six.binary_type(cls._struct_type_map[type_])
            return struct.pack(struct_type, val)
        else:
            raise NotImplementedError("%s serializer not implemented" % type_)

    def __init__(self, trans, client_types=None,
                 fuzz_fields=None, fuzz_all_if_empty=True, verbose=False):
        """Create a TFuzzyHeaderTransport instance.

        @param fuzz_fields(iterable) Collection of string names of fields to
        fuzz. Each name must be a member of
        TFuzzyHeaderTransport.fuzzable_fields. If the iterable is empty and
        fuzz_all_if_empty is True, all fields will be fuzzed. If the iterable
        is empty and fuzz_all_if_empty is False, no fields will be fuzzed.

        @param verbose(bool) Whether to print fuzzed fields to stdout

        @param fuzz_all_if_empty(bool) Whether to fuzz eveyr field if
        the fuzz_fields iterable is None or empty.

        trans and client_type are forwarded to THeaderProtocol.__init__
        """
        cls = self.__class__
        THeaderTransport.__init__(self, trans, client_types=client_types)

        if fuzz_fields is None:
            fuzz_fields = []

        self._fuzz_fields = set()
        for field_name in fuzz_fields:
            if field_name not in cls.fuzzable_fields:
                raise NameError("Invalid fuzz field: %s" % field_name)
            self._fuzz_fields.add(field_name)

        if not self._fuzz_fields and fuzz_all_if_empty:
            # No fields were explicitly included
            self._fuzz_fields = cls.fuzzable_fields

        self._verbose = verbose

    def _print(self, s):
        if self._verbose:
            print(s)

    def _get_fuzzy_field(self, name, val, type_):
        """Return a possibly-fuzzed version of val, depending on self._fuzz_set

        If `name` is included in the set of fields to fuzz, the field value
        will be fuzzed. Otherwise, val will be returned unmodified."""
        cls = self.__class__

        if name not in self._fuzz_fields:
            return val

        if type_ == 'char':
            fuzzed_val = cls._fuzz_char(val)
        elif type_ == 'i16':
            fuzzed_val = cls._fuzz_i16(val)
        elif type_ == 'i32':
            fuzzed_val = cls._fuzz_i32(val)
        elif type_ == 'str':
            fuzzed_val = cls._fuzz_str(val)
        else:
            raise NotImplementedError("%s fuzzing not implemented" % type_)

        self._print("Fuzzed field %s from %r to %r" % (name, val, fuzzed_val))
        return fuzzed_val

    def _write_fuzzy_field(self, buf, name, val, type_):
        """Write a message field after (possibly) mutating the value

        @param buf(StringIO): Message buffer to write to
        @param name(str): Name of the field (for checking whether to fuzz)
        @param val(type_): Value to write to field
        @param type_(type): Type of field (for fuzzing and serializing)
        """
        val = self._get_fuzzy_field(name, val, type_)
        serialized = self._serialize_field(val, type_)
        buf.write(serialized)

    def _flushHeaderMessage(self, buf, wout, wsz):
        """Write a message for self.HEADERS_CLIENT_TYPE

        This method writes a message using the same logic as
        THeaderTransport._flushHeaderMessage
        but mutates fields included in self._fuzz_fields
        """
        cls = self.__class__

        transform_data = StringIO()
        num_transforms = len(self._THeaderTransport__write_transforms)
        for trans_id in self._THeaderTransport__write_transforms:
            trans_id = self._get_fuzzy_field('transform_id', trans_id, 'i32')
            transform_data.write(getVarint(trans_id))

        # Add in special flags.
        if self._THeaderTransport__identity:
            id_version = self._get_fuzzy_field(
                'version', self.ID_VERSION, 'str')
            self._THeaderTransport__write_headers[self.ID_VERSION_HEADER] = (
                id_version)
            identity = self._get_fuzzy_field(
                'identity', self._THeaderTransport__identity, 'str')
            self._THeaderTransport__write_headers[self.IDENTITY_HEADER] = (
                identity)

        info_data = StringIO()

        # Write persistent kv-headers
        cls._flush_info_headers(
            info_data,
            self._THeaderTransport__write_persistent_headers,
            self.INFO_PKEYVALUE)

        # Write non-persistent kv-headers
        cls._flush_info_headers(
            info_data,
            self._THeaderTransport__write_headers,
            self.INFO_KEYVALUE)

        header_data = StringIO()
        proto_id = self._get_fuzzy_field(
            'proto_id', self._THeaderTransport__proto_id, 'i32')
        header_data.write(getVarint(proto_id))
        num_transforms = self._get_fuzzy_field(
            'num_transforms', num_transforms, 'i32')
        header_data.write(getVarint(num_transforms))

        header_size = (transform_data.tell() +
                       header_data.tell() +
                       info_data.tell())

        padding_size = 4 - (header_size % 4)

        # Fuzz padding size, but do not let total header size exceed 2**16 - 1
        padding_size = min((2 ** 16 - 1) - header_size,
            self._get_fuzzy_field('padding', padding_size, 'i16'))
        header_size = header_size + padding_size

        wsz += header_size + 10

        self._write_fuzzy_field(buf, 'length', wsz, 'i32')
        self._write_fuzzy_field(buf, 'magic', self.HEADER_MAGIC, 'i16')
        self._write_fuzzy_field(
            buf, 'flags', self._THeaderTransport__flags, 'i16')
        self._write_fuzzy_field(
            buf, 'seq_id', self._THeaderTransport__seq_id, 'i32')
        self._write_fuzzy_field(buf, 'header_size', header_size // 4, 'i16')

        buf.write(header_data.getvalue())
        buf.write(transform_data.getvalue())
        buf.write(info_data.getvalue())

        # Pad out the header with 0x00
        if 'padding_bytes' in self._fuzz_fields:
            # Print that padding bytes are being fuzzed in favor of printing
            # the value of each individual padding byte
            self._print("Fuzzing %d padding bytes" % padding_size)
        old_verbose, self._verbose = self._verbose, False
        for _ in sm.xrange(padding_size):
            self._write_fuzzy_field(
                buf, 'padding_bytes', six.int2byte(0), 'char')
        self._verbose = old_verbose

        self._write_fuzzy_field(buf, 'payload', wout, 'str')
