"""
Classes for generating random values for thrift types
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import collections
import random

import six
import six.moves as sm

from thrift import Thrift

INFINITY = float('inf')

class BaseRandomizer(object):
    """
    The BaseRandomizer class is an abstract class representing a randomizer for
    a specific Thrift Type. Instances of a class may have different spec_args.
    If the class represents a generic type, instances may also have different
    ttypes.

    Class Attributes:

    name (str): The name of the thrift type.

    ttype (int (enum)): The attribute of Thrift.TTypes corresponding to the type

    Instance Attributes:

    spec_args (tuple): The Thrift spec_args tuple for an instance of a generic
    type. If this is not a generic type, spec_args is None.

    state (RandomizerState): State attributes to be preserved across randomizer
    components in recursive and nested randomizer structures. Includes
    initialization cache and recursion depth trace.
    """
    name = None
    ttype = None

    def __init__(self, spec_args, state):
        """spec_args are the thrift arguments for this field.

        State is an instance of RandomizerState.
        """
        self.spec_args = spec_args
        self.state = state
        self.state.register_randomizer(self)

    @property
    def universe_size(self):
        """
        Return (or estimate) the range of the random variable. If this
        randomizer is used for sets or map keys, the size of the container
        will be limited to this value.
        """
        raise NotImplementedError("_universe_size not implemented for %s" % (
            self.__class__.__name__))

    def randomize(self):
        """Generate a random value of the type, given the spec args"""
        raise NotImplementedError("randomize not implemented for %s" % (
            self.__class__.__name__))


class BoolRandomizer(BaseRandomizer):
    name = "bool"
    ttype = Thrift.TType.BOOL

    p_true = 0.5

    @property
    def universe_size(self):
        return 2

    def randomize(self):
        return (random.random() < self.__class__.p_true)

def _random_int_factory(k):
    """Return a function that generates a random k-bit signed int"""
    min_ = -(1 << (k - 1))   # -2**(k-1)
    max_ = 1 << (k - 1) - 1  # +2**(k-1)-1

    def random_int_k_bits():
        return random.randint(min_, max_)
    return random_int_k_bits

class EnumRandomizer(BaseRandomizer):
    name = "enum"

    random_int_32 = staticmethod(_random_int_factory(32))

    # Probability of generating an invalid i32
    p_invalid = 0.01

    def __init__(self, spec_args, state):
        super(EnumRandomizer, self).__init__(spec_args, state)
        self.ttype = spec_args

        self._whiteset = set()
        for name, val in six.iteritems(self.ttype._NAMES_TO_VALUES):
            self._whiteset.add(val)

        self._whitelist = list(self._whiteset)

    def randomize(self):
        cls = self.__class__

        if random.random() < cls.p_invalid:
            # Generate an i32 value that does not correspond to an enum member
            n = None
            while (n in self._whiteset) or (n is None):
                n = cls.random_int_32()
            return n
        else:
            return random.choice(self._whitelist)

    @property
    def universe_size(self):
        return len(self._whitelist)


def _integer_randomizer_factory(name, ttype, n_bits):
    _universe_size = 2 ** n_bits
    _name = name
    _ttype = ttype
    _n_bits = n_bits

    class NBitIntegerRandomizer(BaseRandomizer):
        name = _name
        ttype = _ttype

        randomize = staticmethod(_random_int_factory(_n_bits))

        @property
        def universe_size(self):
            return _universe_size

    NBitIntegerRandomizer.__name__ = six.binary_type("%sRandomizer" % _name)

    return NBitIntegerRandomizer

ByteRandomizer = _integer_randomizer_factory("byte", Thrift.TType.BYTE, 8)
I16Randomizer = _integer_randomizer_factory("i16", Thrift.TType.I16, 16)
I32Randomizer = _integer_randomizer_factory("i32", Thrift.TType.I32, 32)
I64Randomizer = _integer_randomizer_factory("i64", Thrift.TType.I64, 64)

del _integer_randomizer_factory

class FloatingPointRandomizer(BaseRandomizer):
    """Abstract class for floating point types"""
    p_zero = 0.01

    unreals = [float('nan'), float('inf'), float('-inf')]
    p_unreal = 0.01

    mean = 0
    std_deviation = 1e8

    @property
    def universe_size(self):
        return self.__class__._universe_size

    def randomize(self):
        cls = self.__class__

        if random.random() < cls.p_unreal:
            return random.choice(cls.unreals)

        if random.random() < cls.p_zero:
            return 0.

        return random.normalvariate(cls.mean, cls.std_deviation)

class SinglePrecisionFloatRandomizer(FloatingPointRandomizer):
    name = "float"
    ttype = Thrift.TType.FLOAT

    _universe_size = 2 ** 32

class DoublePrecisionFloatRandomizer(FloatingPointRandomizer):
    name = "double"
    ttype = Thrift.TType.DOUBLE

    _universe_size = 2 ** 64

class CollectionTypeRandomizer(BaseRandomizer):
    """Superclass for ttypes with lengths"""

    mean_length = 12

    @property
    def universe_size(self):
        return INFINITY

    def _get_length(self):
        cls = self.__class__
        return int(random.expovariate(1 / cls.mean_length))

class StringRandomizer(CollectionTypeRandomizer):
    name = "string"
    ttype = Thrift.TType.STRING

    ascii_range = (0, 127)

    def randomize(self):
        cls = self.__class__

        length = self._get_length()
        chars = []

        for _ in sm.xrange(length):
            chars.append(chr(random.randint(*cls.ascii_range)))

        return ''.join(chars)

class NonAssociativeContainerRandomizer(CollectionTypeRandomizer):
    """Randomizer class for lists and sets"""

    def __init__(self, spec_args, state):
        super(NonAssociativeContainerRandomizer, self).__init__(
            spec_args, state)

        if self.spec_args is None:
            return

        elem_ttype, elem_spec_args = self.spec_args

        self._element_randomizer = self.state.get_randomizer(
            elem_ttype, elem_spec_args)

class ListRandomizer(NonAssociativeContainerRandomizer):
    name = "list"
    ttype = Thrift.TType.LIST

    def randomize(self):
        length = self._get_length()
        elements = []

        for _ in sm.xrange(length):
            element = self._element_randomizer.randomize()
            if element is not None:
                elements.append(element)

        return elements

class SetRandomizer(NonAssociativeContainerRandomizer):
    name = "set"
    ttype = Thrift.TType.SET

    def randomize(self):
        element_randomizer = self._element_randomizer

        length = self._get_length()
        length = min(length, element_randomizer.universe_size)

        elements = set()

        # If it is possible to get `length` unique elements,
        # in N = k*length iterations we will reach `length`
        # with high probability.
        i = 0
        k = 10
        N = k * length
        while len(elements) < length and i < N:
            element = element_randomizer.randomize()
            if element is not None:
                elements.add(element)
            i += 1

        return elements

class MapRandomizer(CollectionTypeRandomizer):
    name = "map"
    ttype = Thrift.TType.MAP

    def __init__(self, spec_args, state):
        super(MapRandomizer, self).__init__(spec_args, state)

        key_ttype, key_spec_args, val_ttype, val_spec_args = self.spec_args

        self._key_randomizer = self.state.get_randomizer(
            key_ttype, key_spec_args)
        self._val_randomizer = self.state.get_randomizer(
            val_ttype, val_spec_args)

    def randomize(self):
        key_randomizer = self._key_randomizer
        val_randomizer = self._val_randomizer

        length = self._get_length()
        length = min(length, key_randomizer.universe_size)

        elements = {}

        i = 0
        k = 10
        N = k * length
        while len(elements) < length and i < N:
            key = key_randomizer.randomize()
            val = val_randomizer.randomize()
            if key is not None and val is not None:
                elements[key] = val
            i += 1

        return elements


class StructRandomizer(BaseRandomizer):
    name = "struct"
    ttype = Thrift.TType.STRUCT

    p_include = 0.99
    max_recursion_depth = 3

    @property
    def universe_size(self):
        return INFINITY

    def __init__(self, spec_args, state):
        super(StructRandomizer, self).__init__(spec_args, state)

        ttype, specs, is_union = self.spec_args

        self.type_name = ttype.__name__

        self._init_field_rules(ttype, specs, is_union)

    def _field_is_required(self, required_value):
        """Enum defined in /thrift/compiler/parse/t_field.h:

        T_REQUIRED = 0
        T_OPTIONAL = 1
        T_OPT_IN_REQ_OUT = 2

        Return True iff required_value is T_REQUIRED
        """
        return required_value == 0

    def _init_field_rules(self, ttype, specs, is_union):
        field_rules = {}
        for spec in specs:
            if spec is None:
                continue
            (key, field_ttype, name, field_spec_args, default_value, req) = spec

            field_required = self._field_is_required(req)
            field_randomizer = self.state.get_randomizer(
                field_ttype, field_spec_args)

            field_rules[name] = {
                'required': field_required,
                'randomizer': field_randomizer
            }

        self._field_rules = field_rules
        self._is_union = is_union
        self._ttype = ttype

    @property
    def recursion_depth(self):
        return self.state.recursion_trace[self.type_name]

    @recursion_depth.setter
    def recursion_depth(self, value):
        self.state.recursion_trace[self.type_name] = value

    def randomize(self):
        """Return a random instance of the struct. If it is impossible
        to generate, return None. If ttype is None (e.g., the struct is
        actually a method,) return a dictionary of kwargs instead.
        """
        cls = self.__class__

        # Check if we have reached the maximum recursion depth for this type.
        depth = self.recursion_depth
        max_depth = cls.max_recursion_depth
        if depth >= max_depth:
            return None

        # Haven't reached the maximum depth yet - increment depth and continue
        self.recursion_depth = depth + 1

        # Generate the individual fields within the struct
        fields = self._randomize_fields()

        # Done with any recursive calls - revert depth
        self.recursion_depth = depth

        if fields is None:
            # Unable to generate fields for this struct
            return None
        else:
            # Create an instance of the struct type
            return self._ttype(**fields)

    def _randomize_fields(self):
        """Get random values for each field in the order `order`.

        Return fields as a dict of {field_name: value}

        If fields cannot be generated due to an unsatisfiable
        constraint, return None.
        """
        cls = self.__class__

        fields = {}
        fields_to_randomize = list(self._field_rules)
        p_include = cls.p_include

        if self._is_union:
            if random.random() < p_include:
                return {}
            else:
                fields_to_randomize = [random.choice(fields_to_randomize)]
                p_include = 1.0

        for field_name in fields_to_randomize:
            rule = self._field_rules[field_name]
            required = rule['required']

            if not required and not (random.random() < p_include):
                continue

            randomizer = rule['randomizer']

            value = randomizer.randomize()

            if value is None:
                # Randomizer was unable to generate a value
                if required:
                    # Cannot generate the struct
                    return None
                else:
                    # Ignore the field
                    continue
            else:
                fields[field_name] = value

        return fields

_ttype_to_randomizer = {}

def _init_types():
    # Find classes that subclass BaseRandomizer
    global_names = globals().keys()

    for name in global_names:
        value = globals()[name]
        if not isinstance(value, type):
            continue
        cls = value

        if issubclass(cls, BaseRandomizer):
            if cls.ttype is not None:
                _ttype_to_randomizer[cls.ttype] = cls

_init_types()

class RandomizerState(object):
    """A wrapper around randomizer_map and recursion_trace

    All randomizers are initialized with a state. If a state is not explicitly
    specified, a clean one will be created. When randomizers create sub-
    randomizers, they should pass on their state object in order to share
    memoization and recursion trace information.

    randomizer_map maps (ttype, spec_args) combinations to already-constructed
    randomizer instances. This allows for memoization: calls to get_randomizer
    with identical arguments will always return the same randomizer instance.

    Randomizer instances must call state.register_randomizer BEFORE creating
    any sub-randomizers. Otherwise, randomizer_map will have duplicates and
    if there is a recursive structure it will never finish initializing.

    recursion_trace maps a struct name to an int indicating the current
    depth of recursion for the struct with that name. Struct randomizers
    use this information to bound the recursion depth of generated structs.
    """
    def __init__(self):
        self.randomizer_map = collections.defaultdict(list)
        self.recursion_trace = collections.defaultdict(int)

    def get_randomizer(self, ttype, spec_args):
        """Check the randomizer_map for a cached randomizer.
        Return an already-initialized randomizer if available and create a new
        one otherwise"""
        for (identifier, randomizer) in self.randomizer_map[ttype]:
            if identifier == spec_args:
                return randomizer

        randomizer_class = _ttype_to_randomizer[ttype]

        # i32 and enum have separate classes but the same ttype
        if ttype == Thrift.TType.I32 and spec_args is not None:
            randomizer_class = EnumRandomizer

        randomizer = randomizer_class(spec_args, self)

        return randomizer

    def register_randomizer(self, randomizer):
        type_randomizer_map = self.randomizer_map[randomizer.__class__.ttype]
        type_randomizer_map.append((randomizer.spec_args, randomizer))
