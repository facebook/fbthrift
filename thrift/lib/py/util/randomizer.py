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

def deep_dict_update(base, update):
    """Similar to dict.update(base, update), but if any values in base are
    dictionaries, they are updated too instead of replaced.

    Destructive on base, but non-destructive on base's values.
    """
    for key, val in six.iteritems(update):
        if (key in base and
                isinstance(base[key], dict) and isinstance(val, dict)):
            # Copy base[key] (non-destructive on base's values)
            updated = dict(base[key])
            deep_dict_update(updated, val)
            val = updated
        base[key] = val

class BaseRandomizer(object):
    """
    The BaseRandomizer class is an abstract class whose subclasses implement
    a randomizer for a specific Thrift Type. Instances of a class may have
    different spec_args and constraints.

    Class Attributes:

    name (str): The name of the thrift type.

    ttype (int (enum)): The attribute of Thrift.TTypes corresponding to the type

    default_constraints (dict): Default values for randomizers' constraint
    dictionary. Constraints affect the behavior of the randomize() method.

    Instance Attributes:

    spec_args (tuple): The Thrift spec_args tuple. Provides additional
    information about the field beyond thrift type.

    state (RandomizerState): State attributes to be preserved across randomizer
    components in recursive and nested randomizer structures. Includes
    initialization cache and recursion depth trace.

    constraints (dict): Map of constraint names to constraint values. This
    is equivalent to cls.default_constraints if an empty constraint dictionary
    is passed to __init__. Otherwise, it is equal to cls.default_constraints
    recursively updated with the key/value pairs in the constraint dict passed
    to __init__.
    """
    name = None
    ttype = None

    def __init__(self, spec_args, state, constraints):
        """
        spec_args: thrift arguments for this field
        state: RandomizerState instance
        constraints: dict of constraints specific to this randomizer
        """
        self.spec_args = spec_args
        self.state = state
        self.constraints = self.flatten_constraints(constraints)
        self.preprocessing_done = False

    def _preprocess_constraints(self):
        pass

    def _init_subrandomizers(self):
        pass

    def preprocess(self):
        if self.preprocessing_done:
            return

        self._preprocess_constraints()
        self._init_subrandomizers()

        self.preprocessing_done = True

    def flatten_constraints(self, constraints):
        """Return a single constraint dictionary by combining default
        constraints with overriding constraints."""
        cls = self.__class__

        flattened = {}
        deep_dict_update(flattened, cls.default_constraints)
        deep_dict_update(flattened, constraints)

        return flattened

    def __eq__(self, other):
        """Check if this randomizer is equal to `other` randomizer. If two
        randomizers are equal, they have the same type and constraints and
        are expected to behave identically (up to random number generation.)"""
        return ((self.spec_args == other.spec_args) and
                (self.constraints == other.constraints))

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

    default_constraints = {
        'p_true': 1 / 2
    }

    @property
    def universe_size(self):
        return 2

    def randomize(self):
        return (random.random() < self.constraints['p_true'])

def _random_int_factory(k):
    """Return a function that generates a random k-bit signed int"""
    min_ = -(1 << (k - 1))   # -2**(k-1)
    max_ = 1 << (k - 1) - 1  # +2**(k-1)-1

    def random_int_k_bits():
        return random.randint(min_, max_)
    return random_int_k_bits

class EnumRandomizer(BaseRandomizer):
    name = "enum"
    ttype = Thrift.TType.I32

    random_int_32 = staticmethod(_random_int_factory(32))

    default_constraints = {
        # Probability of generating an i32 with no corresponding Enum name
        'p_invalid': 0.01,
    }

    def _preprocess_constraints(self):
        self.ttype = self.spec_args

        self._whiteset = set()
        for name, val in six.iteritems(self.ttype._NAMES_TO_VALUES):
            self._whiteset.add(val)

        self._whitelist = list(self._whiteset)

    def randomize(self):
        cls = self.__class__

        if random.random() < self.constraints['p_invalid']:
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

        default_constraints = {}

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
    unreals = [float('nan'), float('inf'), float('-inf')]

    default_constraints = {
        'p_zero': 0.01,
        'p_unreal': 0.01,
        'mean': 0.0,
        'std_deviation': 1e8
    }

    @property
    def universe_size(self):
        return self.__class__._universe_size

    def randomize(self):
        cls = self.__class__

        if random.random() < self.constraints['p_unreal']:
            return random.choice(cls.unreals)

        if random.random() < self.constraints['p_zero']:
            return 0.

        return random.normalvariate(self.constraints['mean'],
                                    self.constraints['std_deviation'])

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

    @property
    def universe_size(self):
        return INFINITY

    def _get_length(self):
        mean = self.constraints['mean_length']
        if mean == 0:
            return 0
        else:
            return int(random.expovariate(1 / mean))

class StringRandomizer(CollectionTypeRandomizer):
    name = "string"
    ttype = Thrift.TType.STRING

    ascii_range = (0, 127)

    default_constraints = {
        'mean_length': 12
    }

    def randomize(self):
        cls = self.__class__

        length = self._get_length()
        chars = []

        for _ in sm.xrange(length):
            chars.append(chr(random.randint(*cls.ascii_range)))

        return ''.join(chars)

class NonAssociativeContainerRandomizer(CollectionTypeRandomizer):
    """Randomizer class for lists and sets"""

    def _init_subrandomizers(self):
        elem_ttype, elem_spec_args = self.spec_args
        elem_constraints = self.constraints['element']

        self._element_randomizer = self.state.get_randomizer(
            elem_ttype, elem_spec_args, elem_constraints)

class ListRandomizer(NonAssociativeContainerRandomizer):
    name = "list"
    ttype = Thrift.TType.LIST

    default_constraints = {
        'mean_length': 12,
        'element': {}
    }

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

    default_constraints = {
        'mean_length': 12,
        'element': {}
    }

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

    default_constraints = {
        'mean_length': 12,
        'key': {},
        'value': {}
    }

    def _init_subrandomizers(self):
        key_ttype, key_spec_args, val_ttype, val_spec_args = self.spec_args

        key_constraints = self.constraints['key']
        val_constraints = self.constraints['value']

        self._key_randomizer = self.state.get_randomizer(
            key_ttype, key_spec_args, key_constraints)
        self._val_randomizer = self.state.get_randomizer(
            val_ttype, val_spec_args, val_constraints)

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

    default_constraints = {
        'p_include': 0.99,
        'max_recursion_depth': 3
    }

    @property
    def universe_size(self):
        return INFINITY

    def _field_is_required(self, required_value):
        """Enum defined in /thrift/compiler/parse/t_field.h:

        T_REQUIRED = 0
        T_OPTIONAL = 1
        T_OPT_IN_REQ_OUT = 2

        Return True iff required_value is T_REQUIRED
        """
        return required_value == 0

    def _init_subrandomizers(self):
        ttype, specs, is_union = self.spec_args

        self.type_name = ttype.__name__

        field_rules = {}
        for spec in specs:
            if spec is None:
                continue
            (key, field_ttype, name, field_spec_args, default_value, req) = spec

            field_required = self._field_is_required(req)
            field_constraints = self.constraints.get(name, {})

            field_randomizer = self.state.get_randomizer(
                field_ttype, field_spec_args, field_constraints)

            field_rules[name] = {
                'required': field_required,
                'randomizer': field_randomizer
            }

        self._field_rules = field_rules
        self._is_union = is_union
        self._ttype = ttype

    def _increase_recursion_depth(self):
        """Increase the depth in the recursion trace for this struct type.

        Returns:
        (is_top_level, max_depth_reached)

        If is_top_level is True, when decrease_recursion_depth is called
        the entry in the trace dictionary will be removed to indicate
        that this struct type is no longer being recursively generated.

        If max_depth_reached is True, the call to increase_recursion_depth
        has "failed" indicating that this randomizer is trying to generate
        a value that is too deep in the recursive tree and should return None.
        In this case, the recursion trace dictionary is not modified.
        """
        trace = self.state.recursion_trace
        name = self.type_name

        if name in trace:
            # There is another struct of this type higher up in
            # the generation tree
            is_top_level = False
        else:
            is_top_level = True
            trace[name] = self.constraints['max_recursion_depth']

        depth = trace[name]

        if depth == 0:
            # Reached maximum depth
            if is_top_level:
                del trace[name]
            max_depth_reached = True
        else:
            depth -= 1
            trace[name] = depth
            max_depth_reached = False

        return (is_top_level, max_depth_reached)

    def _decrease_recursion_depth(self, is_top_level):
        """Decrease the depth in the recursion trace for this struct type.

        If is_top_level is True, the entry in the recursion trace is deleted.
        Otherwise, the entry is incremented.
        """
        trace = self.state.recursion_trace
        name = self.type_name

        if is_top_level:
            del trace[name]
        else:
            trace[name] += 1

    def randomize(self):
        """Return a random instance of the struct. If it is impossible
        to generate, return None. If ttype is None (e.g., the struct is
        actually a method,) return a dictionary of kwargs instead.
        """
        cls = self.__class__

        (is_top_level, max_depth_reached) = self._increase_recursion_depth()
        if max_depth_reached:
            return None

        # Generate the individual fields within the struct
        fields = self._randomize_fields()

        # Done with any recursive calls - revert depth
        self._decrease_recursion_depth(is_top_level)

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
        p_include = self.constraints['p_include']

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

    --

    `randomizers` maps ttype to a list of already-constructed randomizer
    instances. This allows for memoization: calls to get_randomizer with
    identical arguments and state will always return the same randomizer
    instance.

    --

    `recursion_trace` maps a struct name to an int indicating the current
    remaining depth of recursion for the struct with that name.
    Struct randomizers use this information to bound the recursion depth
    of generated structs.
    If a struct name has no entry in the recursion trace, that struct
    is not currently being generated at any depth in the generation tree.

    When the top level randomizer for a struct type is entered, that
    randomizer's constraints are used to determine the maximum recursion
    depth and the maximum depth is inserted into the trace dictionary.
    At each level of recursion, the entry in the trace dictionary is
    decremented. When it reaches zero, the maximum depth has been reached
    and no more structs of that type are generated.
    """

    def __init__(self):
        self.randomizers = collections.defaultdict(list)
        self.recursion_trace = {}

    def _get_randomizer_class(self, ttype, spec_args):
        # Special case: i32 and enum have separate classes but the same ttype
        if ttype == Thrift.TType.I32:
            if spec_args is None:
                return I32Randomizer
            else:
                return EnumRandomizer

        return _ttype_to_randomizer[ttype]

    def get_randomizer(self, ttype, spec_args, constraints):
        """Get a randomizer object.
        Return an already-preprocessed randomizer if available and create a new
        one and preprocess it otherwise"""
        randomizer_class = self._get_randomizer_class(ttype, spec_args)
        randomizer = randomizer_class(spec_args, self, constraints)

        # Check if this randomizer is already in self.randomizers
        randomizers = self.randomizers[randomizer.__class__.ttype]
        for other in randomizers:
            if other == randomizer:
                return other

        # No match. Register and preprocess this randomizer
        randomizers.append(randomizer)

        randomizer.preprocess()
        return randomizer
