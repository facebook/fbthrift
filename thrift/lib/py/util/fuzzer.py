"""
Fuzz Testing for Thrift Services
"""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import argparse
import collections
import imp
import itertools
import json
import logging
import os
import pprint
import random
import sys
import time
import types

import six
import six.moves as sm
from six.moves.urllib.parse import urlparse

try:
    from ServiceRouter import ConnConfigs, ServiceOptions, ServiceRouter
    SR_AVAILABLE = True
except ImportError:
    SR_AVAILABLE = False

from thrift import Thrift
from thrift.transport import TTransport, TSocket, TSSLSocket, THttpClient
from thrift.protocol import TBinaryProtocol, TCompactProtocol, THeaderProtocol

def positive_int(s):
    """Typechecker for positive integers"""
    try:
        n = int(s)
        if not n > 0:
            raise argparse.ArgumentTypeError(
                "%s is not positive." % s)
        return n
    except:
        raise argparse.ArgumentTypeError(
            "Cannot convert %s to an integer." % s)

def prob_float(s):
    """Typechecker for probability values"""
    try:
        x = float(s)
        if not 0 <= x <= 1:
            raise argparse.ArgumentTypeError(
                "%s is not a valid probability." % x)
        return x
    except:
        raise argparse.ArgumentTypeError(
            "Cannot convert %s to a float." % s)

class FuzzerConfiguration(object):
    """Container for Fuzzer configuration options"""

    argspec = {
        'allow_application_exceptions': {
            'description': 'Do not flag TApplicationExceptions as errors',
            'type': bool,
            'flag': '-a',
            'argparse_kwargs': {
                'action': 'store_const',
                'const': True
            },
            'default': False,
        },
        'compact': {
            'description': 'Use TCompactProtocol',
            'type': bool,
            'flag': '-c',
            'argparse_kwargs': {
                'action': 'store_const',
                'const': True
            },
            'default': False
        },
        'container_max_size_bits': {
            'description': ('Max bits in container size '
                            '(e.g., 8 bits => up to 511 elements in list).'),
            'type': positive_int,
            'flag': '-m',
            'default': 8
        },
        'default_probability': {
            'description': ('Probability of using the default value of a '
                            'field, if it exists.'),
            'type': prob_float,
            'flag': '-d',
            'default': 0.05,
            'attr_name': 'p_default'
        },
        'exclude_field_probability': {
            'description': 'Probability of excluding a non-required field.',
            'type': prob_float,
            'flag': '-e',
            'default': 0.15,
            'attr_name': 'p_exclude_field'
        },
        'exclude_arg_probability': {
            'description': 'Probability of excluding an argument.',
            'type': prob_float,
            'flag': '-E',
            'default': 0.01,
            'attr_name': 'p_exclude_arg'
        },
        'framed': {
            'description': 'Use framed transport.',
            'type': bool,
            'flag': '-f',
            'argparse_kwargs': {
                'action': 'store_const',
                'const': True
            },
            'default': False
        },
        'functions': {
            'description': 'Which functions to test. If excluded, test all',
            'type': str,
            'flag': '-F',
            'argparse_kwargs': {
                'nargs': '*',
            },
            'default': None
        },
        'host': {
            'description': 'The host and port to connect to',
            'type': str,
            'flag': '-h',
            'argparse_kwargs': {
                'metavar': 'HOST[:PORT]'
            },
            'default': None
        },
        'iterations': {
            'description': 'Number of calls per method.',
            'type': positive_int,
            'flag': '-n',
            'attr_name': 'n_iterations',
            'default': 1000
        },
        'invalid_enum_probability': {
            'description': 'Probability of invalid enum.',
            'type': prob_float,
            'flag': '-i',
            'attr_name': 'p_invalid_enum',
            'default': 0.01
        },
        'logfile': {
            'description': 'File to write output logs.',
            'type': str,
            'flag': '-l',
            'default': None
        },
        'loglevel': {
            'description': 'Level of verbosity to write logs.',
            'type': str,
            'flag': '-L',
            'argparse_kwargs': {
                'choices': ['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
            },
            'default': 'INFO'
        },
        'recursion_depth': {
            'description': 'Maximum struct recursion depth. Default 4.',
            'type': positive_int,
            'flag': '-r',
            'default': 4
        },
        'service': {
            'description': 'Path to file of Python service module.',
            'type': str,
            'flag': '-S',
            'attr_name': 'service_path',
            'default': None
        },
        'ssl': {
            'description': 'Use SSL socket.',
            'type': bool,
            'flag': '-s',
            'argparse_kwargs': {
                'action': 'store_const',
                'const': True
            },
            'default': False
        },
        'string_max_size_bits': {
            'description': ('Max bits in string length '
                            '(e.g., 10 bits => up to 2047 chars in string.)'),
            'type': positive_int,
            'flag': '-M',
            'default': 10
        },
        'unframed': {
            'description': 'Use unframed transport.',
            'type': bool,
            'flag': '-U',
            'argparse_kwargs': {
                'action': 'store_const',
                'const': True
            },
            'default': False
        },
        'unicode_ranges': {
            'description': ('JSON Array of arrays indicating ranges of '
                            'characters to include in unicode strings. '
                            'Default includes ASCII characters.'),
            'type': str,
            'flag': '-ur',
            'default': "[[0, 127]]",
            'is_json': True
        },
        'unicode_out_of_range_probability': {
            'description': ('Probability a unicode string will include '
                            'characters outside `unicode_ranges`.'),
            'type': prob_float,
            'flag': '-up',
            'default': 0.05,
            'attr_name': 'p_unicode_oor'
        },
        'url': {
            'description': 'The URL to connect to for HTTP transport',
            'type': str,
            'flag': '-u',
            'default': None
        },
    }
    if SR_AVAILABLE:
        argspec['tier'] = {
            'description': 'The SMC tier to connect to',
            'type': str,
            'flag': '-t',
            'default': None
        }
        argspec['conn_configs'] = {
            'description': 'ConnConfigs to use for ServiceRouter connection',
            'type': str,
            'flag': '-Conn',
            'default': {},
            'is_json': True
        }
        argspec['service_options'] = {
            'description': 'ServiceOptions to use for ServiceRouter connection',
            'type': str,
            'flag': '-SO',
            'default': {},
            'is_json': True
        }

    def __init__(self, service=None):
        cls = self.__class__

        if service is not None:
            self.service = service

        parser = argparse.ArgumentParser(description='Fuzzer Configuration',
                                         add_help=False)
        parser.add_argument('-C', '--config', dest='config_filename',
                            help='JSON Configuration file. '
                            'All settings can be specified as commandline '
                            'args and config file settings. Commandline args '
                            'override config file settings.')

        parser.add_argument('-?', '--help', action='help',
                            help='Show this help message and exit.')

        for name, arg in six.iteritems(cls.argspec):
            kwargs = arg.get('argparse_kwargs', {})

            if kwargs.get('action', None) != 'store_const':
                # Pass type to argparse. With store_const, type can be inferred
                kwargs['type'] = arg['type']

            # If an argument is not passed, don't put a value in the namespace
            kwargs['default'] = argparse.SUPPRESS

            # Use the argument's description and default as a help message
            kwargs['help'] = "%s Default: %s" % (arg.get('description', ''),
                                                 arg['default'])

            kwargs['dest'] = arg.get('attr_name', name)

            if hasattr(self, kwargs['dest']):
                # Attribute already assigned (e.g., service passed to __init__)
                continue

            parser.add_argument(arg['flag'], '--%s' % name, **kwargs)

            # Assign the default value to config namespace
            setattr(self, kwargs['dest'], arg['default'])

        args = parser.parse_args()

        # Read settings in config file
        self.__dict__.update(cls._config_file_settings(args))

        # Read settings in args
        self.__dict__.update(cls._args_settings(args))

        valid, message = self._validate_config()
        if not valid:
            print(message, file=sys.stderr)
            sys.exit(os.EX_USAGE)

    @classmethod
    def _try_parse_type(cls, name, type_, val):
        try:
            val = type_(val)
        except:
            raise TypeError(("Expected type %s for setting %s, "
                             "but got type %s (%s)") % (
                                 type_, name, type(val), val))
        return val

    @classmethod
    def _try_parse(cls, name, arg, val):
        if arg.get('is_json', False):
            return val

        type_ = arg['type']

        nargs = arg.get('argparse_kwargs', {}).get('nargs', None)

        if nargs is None:
            return cls._try_parse_type(name, type_, val)
        else:
            if not isinstance(val, list):
                raise TypeError(("Expected list of length %s "
                                 "for setting %s, but got type %s (%s)") % (
                                     nargs, name, type(val), val))
            ret = []
            for elem in val:
                ret.append(cls._try_parse_type(name, type_, elem))
            return ret

    @classmethod
    def _config_file_settings(cls, args):
        """Read settings from a configuration file"""
        if args.config_filename is None:
            return {}  # No config file
        if not os.path.exists(args.config_filename):
            raise OSError(os.EX_NOINPUT,
                "Config file does not exist: %s" % args.config_filename)
        with open(args.config_filename, "r") as fd:
            try:
                settings = json.load(fd)
            except ValueError as e:
                raise ValueError("Error parsing config file: %s" % e)

        # Make sure settings are well-formatted
        renamed_settings = {}
        if not isinstance(settings, dict):
            raise TypeError("Invalid config file. Top-level must be Object.")
        for name, val in six.iteritems(settings):
            if name not in cls.argspec:
                raise ValueError(("Unrecognized configuration "
                                  "option: %s") % name)
            arg = cls.argspec[name]
            val = cls._try_parse(name, arg, val)
            attr_name = arg.get('attr_name', name)
            renamed_settings[attr_name] = val
        return renamed_settings

    @classmethod
    def _args_settings(cls, args):
        """Read settings from the args namespace returned by argparse"""
        settings = {}
        for name, arg in six.iteritems(cls.argspec):
            attr_name = arg.get('attr_name', name)
            if not hasattr(args, attr_name):
                continue
            value = getattr(args, attr_name)
            if arg.get('is_json', False):
                settings[attr_name] = json.loads(value)
            else:
                settings[attr_name] = value
        return settings

    def __str__(self):
        return 'Configuration(\n%s\n)' % pprint.pformat(self.__dict__)

    def load_service(self):
        if self.service is not None:
            if self.service_path is not None:
                raise ValueError("Cannot specify a service path when the "
                                 "service is input programmatically")
            # Service already loaded programmatically. Just load methods.
            self.service.load_methods()
            return self.service

        if self.service_path is None:
            raise ValueError("Error: No service specified")

        service_path = self.service_path

        if not os.path.exists(service_path):
            raise OSError("Service module does not exist: %s" % service_path)

        if not service_path.endswith('.py'):
            raise OSError("Service module is not a Python module: %s" %
                          service_path)

        parent_path, service_filename = os.path.split(service_path)
        service_name = service_filename[:-3]  # Truncate extension

        logging.info("Service name: %s" (service_name))

        parent_path = os.path.dirname(service_path)
        ttypes_path = os.path.join(parent_path, 'ttypes.py')
        constants_path = os.path.join(parent_path, 'constants.py')

        imp.load_source('module', parent_path)
        ttypes_module = imp.load_source('module.ttypes', ttypes_path)
        constants_module = imp.load_source('module.constants', constants_path)
        service_module = imp.load_source('module.%s' % (service_name),
                                         service_path)

        service = Service(ttypes_module, constants_module, service_module)
        service.load_methods()
        return service

    def _validate_config(self):
        # Verify there is one valid connection flag
        specified_flags = []
        connection_flags = FuzzerClient.connection_flags
        for flag in connection_flags:
            if hasattr(self, flag) and getattr(self, flag) is not None:
                specified_flags.append(flag)

        if not len(specified_flags) == 1:
            message = "Exactly one of [%s] must be specified. Got [%s]." % (
                (', '.join('--%s' % flag for flag in connection_flags)),
                (', '.join('--%s' % flag for flag in specified_flags)))
            return False, message

        connection_method = specified_flags[0]
        self.connection_method = connection_method

        if connection_method == 'url':
            if not (self.compact or self.framed or self.unframed):
                message = ("A protocol (compact, framed, or unframed) "
                           "must be specified for HTTP Transport.")
                return False, message

        if connection_method in {'url', 'host'}:
            if connection_method == 'url':
                try:
                    url = urlparse(self.url)
                except:
                    return False, "Unable to parse url %s" % self.url
                else:
                    connection_str = url[1]
            elif connection_method == 'host':
                connection_str = self.host
            if ':' in connection_str:
                # Get the string after the colon
                port = connection_str[connection_str.index(':') + 1:]
                try:
                    int(port)
                except ValueError:
                    message = "Port is not an integer: %s" % port
                    return False, message

        return True, None


class Service(object):
    """Wrapper for a thrift service"""
    def __init__(self, ttypes_module, constants_module, service_module):
        self.ttypes = ttypes_module
        self.constants = constants_module
        self.service = service_module
        self.methods = None

    def __str__(self):
        return 'Service(%s)' % self.service.__name__

    def load_methods(self):
        """Load a service's methods"""
        service_module = self.service

        method_inheritance_chain = []
        while service_module is not None:
            interface = service_module.Iface
            if_attrs = [getattr(interface, a) for a in dir(interface)]
            if_methods = {m.__name__ for m in if_attrs if
                          isinstance(m, types.MethodType)}
            method_inheritance_chain.append((service_module, if_methods))
            if interface.__bases__:
                # Can only have single inheritance in thrift
                service_module = __import__(interface.__bases__[0].__module__,
                                            {}, {}, ['Iface'])
            else:
                service_module = None

        # Map method names to the module the method is defined in
        method_to_module = {}

        # Iterate starting at the top of the tree
        for (module, if_methods) in method_inheritance_chain[::-1]:
            for method_name in if_methods:
                if method_name not in method_to_module:
                    method_to_module[method_name] = module

        methods = {}
        for method_name, module in six.iteritems(method_to_module):
            args_class_name = "%s_args" % (method_name)
            result_class_name = "%s_result" % (method_name)

            if hasattr(module, args_class_name):
                args = getattr(module, args_class_name)
            else:
                raise AttributeError(
                    "Method arg spec not found: %s.%s" % (
                        module.__name__, method_name))

            if hasattr(module, result_class_name):
                result = getattr(module, result_class_name)
            else:
                result = None

            thrift_exceptions = []
            if result is not None:
                for res_spec in result.thrift_spec:
                    if res_spec[2] != 'success':
                        # This is an exception return type
                        spec_args = res_spec[3]
                        exception_type = spec_args[0]
                        thrift_exceptions.append(exception_type)

            methods[method_name] = {
                'args_spec': args,
                'result_spec': result,
                'thrift_exceptions': tuple(thrift_exceptions)
            }

        self.methods = methods

    @property
    def client_class(self):
        return self.service.Client

    def get_methods(self, include=None):
        """Get a dictionary of methods provided by the service.

        If include is not None, it should be a collection and only
        the method names in that collection will be included."""

        if self.methods is None:
            raise ValueError("Service.load_methods must be "
                             "called before Service.get_methods")
        if include is None:
            return self.methods

        included_methods = {}
        for method_name in include:
            if method_name not in self.methods:
                raise NameError("Function does not exist: %s" % method_name)
            included_methods[method_name] = self.methods[method_name]

        return included_methods

class FuzzerClient(object):
    """Client wrapper used to make calls based on configuration settings"""

    connection_flags = ['host', 'url', 'tier']
    default_port = 9090

    def __init__(self, config, client_class):
        self.config = config
        self.client_class = client_class

    def _get_client_by_transport(self, config, transport, socket=None):
        # Create the protocol and client
        if config.compact:
            protocol = TCompactProtocol.TCompactProtocol(transport)
        # No explicit option about protocol is specified. Try to infer.
        elif config.framed or config.unframed:
            protocol = TBinaryProtocol.TBinaryProtocolAccelerated(transport)
        elif socket is not None:
            protocol = THeaderProtocol.THeaderProtocol(socket)
            transport = protocol.trans
        else:
            raise ValueError("No protocol specified for HTTP Transport")
        transport.open()
        self._transport = transport

        client = self.client_class(protocol)
        return client

    def _parse_host_port(self, value, default_port):
        parts = value.rsplit(':', 1)
        if len(parts) == 1:
            return (parts[0], default_port)
        else:
            # FuzzerConstraints ensures parts[1] is an int
            return (parts[0], int(parts[1]))

    def _get_client_by_host(self):
        config = self.config
        host, port = self._parse_host_port(config.host, self.default_port)
        socket = (TSSLSocket.TSSLSocket(host, port) if config.ssl
                  else TSocket.TSocket(host, port))
        if config.framed:
            transport = TTransport.TFramedTransport(socket)
        else:
            transport = TTransport.TBufferedTransport(socket)
        return self._get_client_by_transport(config, transport, socket=socket)

    def _get_client_by_url(self):
        config = self.config
        url = urlparse(config.url)
        host, port = self._parse_host_port(url[1], 80)
        transport = THttpClient.THttpClient(config.url)
        return self._get_client_by_transport(config, transport)

    def _get_client_by_tier(self):
        """Get a client that uses ServiceRouter"""
        config = self.config
        serviceRouter = ServiceRouter()

        overrides = ConnConfigs()
        for key, val in six.iteritems(config.conn_configs):
            key = six.binary_type(key)
            val = six.binary_type(val)
            overrides[key] = val

        sr_options = ServiceOptions()
        for key, val in six.iteritems(config.service_options):
            key = six.binary_type(key)
            if not isinstance(val, list):
                raise TypeError("Service option %s expected list; got %s (%s)"
                                % (key, val, type(val)))
            val = [six.binary_type(elem) for elem in val]
            sr_options[key] = val

        service_name = config.tier

        # Obtain a normal client connection using SR2
        client = serviceRouter.getClient2(self.client_class,
                                          service_name, sr_options,
                                          overrides, False)

        if client is None:
            raise NameError('Failed to lookup host for tier %s' % service_name)

        return client

    def _get_client(self):
        if self.config.connection_method == 'host':
            client = self._get_client_by_host()
        elif self.config.connection_method == 'url':
            client = self._get_client_by_url()
        elif self.config.connection_method == 'tier':
            client = self._get_client_by_tier()
        else:
            raise NameError("Unknown connection type: %s" %
                            self.config.connection_method)
        return client

    def _close_client(self):
        if self.config.connection_method in {'host', 'url'}:
            self._transport.close()

    def __enter__(self):
        self.client = self._get_client()
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        self._close_client()
        self.client = None

    def reset(self):
        self._close_client()
        try:
            self.client = self._get_client()
            return True
        except TTransport.TTransportException as e:
            logging.error("Unable to reset connection: %r" % e)
            return False

    def make_call(self, method_name, kwargs, is_oneway=False):
        method = getattr(self.client, method_name)
        ret = method(**kwargs)

        if is_oneway:
            self.reset()

        return ret


class Req:
    """Enum defined in /thrift/compiler/parse/t_field.h"""
    T_REQUIRED = 0
    T_OPTIONAL = 1
    T_OPT_IN_REQ_OUT = 2

def random_int_factory(k):
    """Return a function that generates a random k-bit signed int"""
    min_ = -(1 << (k - 1))   # -2**(k-1)
    max_ = 1 << (k - 1) - 1  # +2**(k-1)-1

    def random_int_k_bits(spec_args):
        return random.randint(min_, max_)
    return random_int_k_bits


FLOAT_EDGE_CASES = [0.0, float('nan'), float('inf'), float('-inf')]
P_FLOAT_EDGE_CASE = 0.0

def random_float_factory(e, m):
    """Return a function that generates a random floating point value

    e = number of exponent bits
    m = number of mantissa bits
    """
    def random_float(spec_args):
        if random.random() < P_FLOAT_EDGE_CASE:
            return random.choice(FLOAT_EDGE_CASES)
        else:
            # Distribute exponents logarithmically
            # (Exponents in [2**k, 2**(k+1)) are twice as likely
            #  to be chosen as exponents in [2**(k+1), 2**(k+2))
            exponent_bits = random.randint(1, e)
            exponent = random.randint(0, (1 << exponent_bits) - 1)

            # Get a fraction uniform over [0, 1]
            fraction = random.random()
            if exponent != 0:
                # Not gradual underflow
                fraction += 1

            # Include negative exponents
            exponent -= 1 << (exponent_bits - 1)

            # Possibly negate fraction
            if random.choice([True, False]):
                fraction *= -1

            return fraction * (2 ** exponent)
    return random_float


class TTypeRandomizer:
    """Functions to generate random values for thrift types"""

    def __init__(self, config):
        self.randomizer_by_ttype = {
            Thrift.TType.BOOL: self.random_bool,
            Thrift.TType.BYTE: random_int_factory(8),
            Thrift.TType.I08: random_int_factory(8),
            Thrift.TType.DOUBLE: random_float_factory(11, 52),
            Thrift.TType.I16: random_int_factory(16),
            Thrift.TType.I32: self.random_i32,
            Thrift.TType.I64: random_int_factory(64),
            Thrift.TType.STRING: self.random_string,
            Thrift.TType.STRUCT: self.random_struct,
            Thrift.TType.MAP: self.random_map,
            Thrift.TType.SET: self.random_set,
            Thrift.TType.LIST: self.random_list,
            Thrift.TType.UTF8: self.random_utf8,
            Thrift.TType.UTF16: self.random_utf16,
            Thrift.TType.FLOAT: random_float_factory(8, 23)
        }

        self.container_max_size_bits = config.container_max_size_bits
        self.string_max_size_bits = config.string_max_size_bits
        self.p_exclude_arg = config.p_exclude_arg
        self.p_exclude_field = config.p_exclude_field
        self.p_default = config.p_default
        self.p_invalid_enum = config.p_invalid_enum
        self.unicode_ranges = config.unicode_ranges
        self.p_unicode_oor = config.p_unicode_oor
        self.max_recursion_depth = config.recursion_depth

    def random_bool(self, spec_args):
        return random.choice([True, False])

    random_int_32 = random_int_factory(32)

    def random_i32(self, spec_args):
        if (spec_args is None or
            (self.p_invalid_enum != 0.0 and
             random.random() < self.p_invalid_enum)):
            # Regular i32 field, or a (probably) invalid enum
            return self.random_int_32()
        else:
            # Valid enum field
            return random.choice(spec_args._VALUES_TO_NAMES.keys())

    def _random_exp_distribution(self, max_bits):
        """Return an int with up to max_bits bits

        Values generated here are distributed step-wise exponentially.
        For 1 < k <= max_bits, if x is the return value, then:

        P(2**(k-2) <= x < 2**(k-1)) == P(2**(k-1) <= x < 2**k)

        The range [2**(k-1), 2**k) is the set of k-bit intgeters
        (that is, the most significant `1` is at position k.)

        Since each consecutive [2**(k-1), 2**k) bucket has twice
        as many elements as the last, each individual element has
        half the probability of being returned. In general:

        P(return=x) == (1 / (n+1)) * (1 / (2 ** floor(log(x))))

        1/(n+1) is the probability the bucket containing x is chosen.
        (2**(floor(log(x)))) is the probability x is chosen among its
        bucket, since there are floor(log(x)) elements in its bucket.

        For example, if x=11, floor(log(x)) = 3, and there are 2**3=8
        elements in the bucket of 4-bit numbers (8 through 15.)

        The one exception to the probability given above is if x=0.
        log(0) is undefined, but P(return=0) = (1 / n+1).
        """
        n_bits = random.randint(0, max_bits)
        if n_bits == 0:
            return 0
        return random.randint(1 << (n_bits - 1),
                              (1 << n_bits) - 1)

    def _random_string_length(self):
        return self._random_exp_distribution(self.string_max_size_bits)

    def _random_container_length(self):
        return self._random_exp_distribution(self.container_max_size_bits)

    def random_string(self, spec_args):
        return ''.join(chr(random.randint(0, 127)) for
                       _ in sm.xrange(self._random_string_length()))

    def random_struct(self, spec_args):
        ttype, specs, is_union = spec_args

        # Check if we have exceeded the maximum recursion depth for this type
        depth = self.recursion_depth[ttype.__name__]
        max_depth = self.max_recursion_depth
        if depth >= max_depth:
            if depth == max_depth:
                # Already at maximum recursion depth
                # Cannot generate new struct of this type
                return None
            else:
                raise ValueError("Maximum recursion depth exceeded for %s" % (
                    ttype.__name__))

        self.recursion_depth[ttype.__name__] += 1

        specs = [spec for spec in specs if spec is not None]
        if is_union:
            # Only populate one field
            specs = [random.choice(specs)]
        fields = self.random_fields(specs)

        self.recursion_depth[ttype.__name__] -= 1

        if fields is None:
            # Unable to populate fields
            return None
        else:
            return ttype(**fields)

    def _random_container(self, elem_ttype, elem_spec_args, length=None):
        length = self._random_container_length() if length is None else length

        randomizer_fn = self[elem_ttype]
        elements = []

        for _ in sm.xrange(length):
            value = randomizer_fn(elem_spec_args)
            if value is not None:
                elements.append(value)
        return elements

    def random_map(self, spec_args):
        key_type, key_spec_args, val_type, val_spec_args = spec_args

        keys = self._random_container(key_type, key_spec_args)
        vals = self._random_container(val_type, val_spec_args, len(keys))

        return dict(itertools.izip(keys, vals))

    def random_set(self, spec_args):
        return set(self._random_container(*spec_args))

    def random_list(self, spec_args):
        return self._random_container(*spec_args)

    def random_utf8(self, spec_args):
        length = self._random_string_length()
        if (len(self.unicode_ranges) == 0 or
                random.random() < self.p_unicode_oor):

            # Out of specified ranges
            return ''.join(six.unichr(random.randint(0, 0x110000)) for
                           _ in sm.xrange(length))
        else:
            chars = []
            for _ in sm.xrange(length):
                range_ = random.choice(self.unicode_ranges)
                chars.append(six.unichr(random.randint(range_[0], range_[1])))
            return ''.join(chars)

    # The only difference between UTF8 and UTF16 is how the protocol will
    # encode the unicode string
    random_utf16 = random_utf8

    def __getitem__(self, ttype):
        if ttype in self.randomizer_by_ttype:
            return self.randomizer_by_ttype[ttype]
        else:
            raise TypeError("Unsupported field type: %d" % ttype)

    def random_fields(self, specs, is_toplevel=False):
        """Get random fields. Works for argument fields and struct fields.

        Fields are returned as a dictionary of {field_name: value}

        If fields cannot be generated for these specs (due to
        "required" fields making the recursion depth limit
        unsatisfiable,) then None will be returned instead.

        is_toplevel should be True if the fields are to be generated
        for a function call rather than a struct.
        """
        if is_toplevel:
            self.recursion_depth = collections.defaultdict(int)
            exclude_probability = self.p_exclude_arg
        else:
            exclude_probability = self.p_exclude_field

        fields = {}
        for spec in specs:
            if spec is None:
                continue

            (key, ttype, name, spec_args, default_value, req) = spec

            if (req != Req.T_REQUIRED and
                    random.random() < exclude_probability):
                continue

            if (default_value is not None and
                    random.random() < self.p_default):
                fields[name] = default_value
                continue

            random_fn = self[ttype]
            value = random_fn(spec_args)

            if value is None:
                # Could not generate a value for this field
                if req == Req.T_REQUIRED:
                    # Field was required -- cannot generate struct
                    return None
                else:
                    # Ignore this field
                    continue
            else:
                fields[name] = value

        return fields

class Timer(object):
    def __init__(self, aggregator, category, action):
        self.aggregator = aggregator
        self.category = category
        self.action = action

    def __enter__(self):
        self.start_time = time.time()

    def __exit__(self, exc_type, exc_value, traceback):
        end_time = time.time()
        time_elapsed = end_time - self.start_time
        self.aggregator.add(self.category, self.action, time_elapsed)

class TimeAggregator(object):
    def __init__(self):
        self.total_time = collections.defaultdict(
            lambda: collections.defaultdict(float))

    def time(self, category, action):
        return Timer(self, category, action)

    def add(self, category, action, time_elapsed):
        self.total_time[category][action] += time_elapsed

    def summarize(self):
        max_category_name_length = max(len(name) for name in self.total_time)
        max_action_name_length = max(max(len(action_name) for action_name in
                                         self.total_time[name]) for name in
                                     self.total_time)
        category_format = "%%%ds: %%s" % max_category_name_length
        action_format = "%%%ds: %%4.3fs" % max_action_name_length

        category_summaries = []
        for category_name, category_actions in sorted(self.total_time.items()):
            timing_items = []
            for action_name, action_time in sorted(category_actions.items()):
                timing_items.append(action_format % (action_name, action_time))
            all_actions = " | ".join(timing_items)
            category_summaries.append(category_format % (
                category_name, all_actions))
        summaries = "\n".join(category_summaries)
        logging.info("Timing Summary:\n%s" % summaries)


class FuzzTester(object):
    def __init__(self, config):
        self.config = config
        self.service = None
        self.randomizer = None
        self.client = None

    def start_logging(self):
        logfile = self.config.logfile
        if self.config.logfile is None:
            logfile = '/dev/null'
        log_level = getattr(logging, self.config.loglevel)
        if logfile == 'stdout':
            logging.basicConfig(stream=sys.stdout, level=log_level)
        else:
            logging.basicConfig(filename=self.config.logfile, level=log_level)

    def start_timing(self):
        self.timer = TimeAggregator()

    def _call_string(self, method_name, kwargs):
        kwarg_str = ', '.join('%s=%s' % (k, v)
                              for k, v in six.iteritems(kwargs))
        return "%s(%s)" % (method_name, kwarg_str)

    def run_test(self, method_name, kwargs, expected_output,
                 is_oneway, thrift_exceptions):
        """
        Make an RPC with given arguments and check for exceptions.
        """
        try:
            with self.timer.time(method_name, "Thrift"):
                self.client.make_call(method_name, kwargs,
                                            is_oneway)
        except thrift_exceptions as e:
            if self.config.loglevel == "DEBUG":
                with self.timer.time(method_name, "Logging"):
                    logging.debug("Got thrift exception: %r" % e)
                    logging.debug("Exception thrown by call: %s" % (
                        self._call_string(method_name, kwargs)))

        except Thrift.TApplicationException as e:
            if self.config.allow_application_exceptions:
                if self.config.loglevel == "DEBUG":
                    with self.timer.time(method_name, "Logging"):
                        logging.debug("Got TApplication exception %s (%r)" % (
                            e, e))
                        logging.debug("Exception thrown by call: %s" % (
                            self._call_string(method_name, kwargs)))
            else:
                with self.timer.time(method_name, "Logging"):
                    self.n_exceptions += 1
                    logging.error("Got application exception: %r" % e)
                    logging.error("Offending call: %s" % (
                        self._call_string(method_name, kwargs)))

        except TTransport.TTransportException as e:
            self.n_exceptions += 1

            with self.timer.time(method_name, "Logging"):
                logging.error("Got TTransportException: (%s, %r)" % (e, e))
                logging.error("Offending call: %s" % (
                    self._call_string(method_name, kwargs)))

            if "errno = 111: Connection refused" in e.args[0]:
                # Unable to connect to server - server may be down
                return False

            if not self.client.reset():
                logging.error("Inferring server crash.")
                return False

        except Exception as e:
            with self.timer.time(method_name, "Logging"):
                self.n_exceptions += 1
                logging.error("Got exception %s (%r)" % (e, e))
                logging.error("Offending call: %s" % (
                    self._call_string(method_name, kwargs)))
                if hasattr(self, 'previous_kwargs'):
                    logging.error("Previous call: %s" % (
                        self._call_string(method_name, self.previous_kwargs)))

        else:
            if self.config.loglevel == "DEBUG":
                with self.timer.time(method_name, "Logging"):
                    logging.debug("Successful call: %s" % (
                        self._call_string(method_name, kwargs)))
        finally:
            self.n_tests += 1

        return True

    def fuzz_kwargs(self, method_name, args_spec, n_iterations):
        # For now, just yield n random sets of args
        # In future versions, fuzz fields more methodically based
        # on feedback and seeds
        for _ in sm.xrange(n_iterations):
            with self.timer.time(method_name, "Randomizing"):
                kwargs = self.randomizer.random_fields(args_spec.thrift_spec,
                                                       is_toplevel=True)

            if kwargs is None:
                logging.error("Unable to produce valid arguments for %s" %
                              method_name)
            else:
                yield kwargs

    def run(self):
        self.start_logging()
        self.start_timing()

        logging.info("Starting Fuzz Tester")
        logging.info(str(self.config))

        self.service = self.config.load_service()
        self.randomizer = TTypeRandomizer(self.config)

        client_class = self.service.client_class
        methods = self.service.get_methods(self.config.functions)

        logging.info("Fuzzing methods: %s" % methods.keys())

        with FuzzerClient(self.config, client_class) as self.client:
            for method_name, spec in six.iteritems(methods):
                args_spec = spec['args_spec']
                result_spec = spec.get('result_spec', None)
                thrift_exceptions = spec['thrift_exceptions']
                is_oneway = result_spec is None
                logging.info("Fuzz testing method %s" % (method_name))
                self.n_tests = 0
                self.n_exceptions = 0
                did_crash = False
                for kwargs in self.fuzz_kwargs(method_name, args_spec,
                                                    self.config.n_iterations):
                    if not self.run_test(method_name, kwargs, None,
                                         is_oneway, thrift_exceptions):
                        did_crash = True
                        break
                    self.previous_kwargs = kwargs

                if did_crash:
                    logging.error(("Method %s caused the "
                                   "server to crash.") % (
                                       method_name))
                    break
                else:
                    logging.info(("Method %s raised unexpected "
                                  "exceptions in %d/%d tests.") % (
                                      method_name, self.n_exceptions,
                                      self.n_tests))

        self.timer.summarize()

def run_fuzzer(config):
    fuzzer = FuzzTester(config)
    fuzzer.run()

def fuzz_service(service, ttypes, constants):
    """Run the tester with required modules input programmatically"""
    service = Service(ttypes, constants, service)
    config = FuzzerConfiguration(service)
    run_fuzzer(config)

if __name__ == '__main__':
    config = FuzzerConfiguration()
    run_fuzzer(config)
