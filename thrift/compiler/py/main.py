#!/usr/bin/env python -tt
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

import sys
import os
import itertools

# this calls the __init__ of generate, and populates the registry with all the
# generator_factories.
# Then we grab the single instance of generator_registry
from thrift_compiler.generate.t_generator \
    import registry as generator_registry

from thrift_compiler import frontend


class ArgumentError(Exception):
    pass


def init_parser():
    from optparse import OptionParser, OptionGroup
    from optparse import TitledHelpFormatter, IndentedHelpFormatter

    class MyParser(OptionParser):
        def format_epilog(self, formatter):
            return self.epilog

    # construct the epilogue
    tmp = []
    for lang, opts in generator_registry.reference.iteritems():
        tmp.append('  {0} ({1[long]}):'.format(lang, opts))
        for key, help in opts.get('options', {}).items():
            tmp.append(''.join(('    ', '{0}:'.format(key).ljust(20), help)))

    epilogue = '\n'.join([
        '',
        'Available generators (and options):',
        ] + tmp +
        [''] * 2)

    # construct the parser
    parser = MyParser(usage="%prog [options] file", description="", version="",
                      epilog=epilogue, formatter=IndentedHelpFormatter())

    # defaults
    parser.set_defaults(strict=127, warn=1, debug=False)

    # callback for --strict
    def strict_cob(option, opt_str, value, parser):
        if opt_str == '--strict':
            parser.values.strict = 255
            parser.values.warn = 2
        else:
            return NotImplemented

    # parser rules
    rules = [
        (['-o', '--install_dir'], dict(metavar='dir',
            dest="outputDir", default='.',
            help='Set the output directory for gen-* packages (default:'
            'current directory)')),
        (['--out'], {}),
        (['-I'], dict(metavar='dir', dest="includeDirs", action="append",
            help='Add a directory to the list of directories searched for'
            'include directives')),
        (['--nowarn'], dict(action='store_const', const=0, dest='warn',
            help='Suppress all compiler warnings (BAD!)')),
        (['--strict'], dict(action='callback', callback=strict_cob,
            help='Strict compiler warnings on')),
        (['-v', '--verbose'], dict(action='store_true', default=False,
            help='Verbose mode')),
        (['-r', '--recurse'], dict(action='store_true', default=False,
            help='Also generate included files')),
        (['--debug'], dict(action='store_true',
            help='Parse debug trace to stdout')),
        (['--allow-neg-keys'], dict(action='store_true', default=False,
            help='Allow negative field keys (Used to preserve '
            'protocol compatibility with older .thrift files')),
        (['--allow-neg-enum-vals'], dict(action='store_true', default=False,
            help='Allow negative enum vals')),
        (['--allow-64bit-consts'], dict(action='store_true', default=False,
            help='Do not print warnings about using 64-bit constants')),
        (['--gen'], dict(metavar='STR', dest='generate', default=[],
            action='append', help='Generate code with a dynamically-'
            'registered generator. STR has the form language[:key1=val1[,'
            'key2,[key3=val3]]]. Keys and values are options passed to the '
            'generator. Many options will not require values.')),
        (['--fbcode_dir'], {}),
        (['--record-genfiles'], {}),
    ]

    for i, j in rules:
        parser.add_option(*i, **j)
    return parser


def toDict(string):
    'Turns a string of the form a[,c[=d]]... to a dict'
    d = {}
    items = string.split(',')
    for item in itertools.ifilter(None, items):
        item = item.split('=', 1)
        if len(item) == 1:
            key = item[0]
            d[key] = ''
        elif len(item) == 2:
            key, value = item
            d[key] = value
    return d


def parseParameters(parser, args):
    # do the actual parsing
    (opts, args) = parser.parse_args(args)

    if len(args) != 1:
        raise ArgumentError('Must provide exactly one thrift definition file.')
    thrift_file = args[0]

    # languages to generate
    to_generate = {}

    # parse generating options
    generate = opts.generate
    if len(generate) == 0:
        raise ArgumentError('Please specify at least one language to '
                            'generate.')
    for desc in generate:
        tmp = desc.split(':')
        if not (0 < len(tmp) <= 2):
            raise ArgumentError('Incorrect language description.'
            'Syntax: language[:key1=val1[,key2,[key3=val3]]].')
        # add empty switches array
        if len(tmp) == 1:
            tmp.append('')
        lang, switches = tmp
        if lang not in generator_registry.generator_factory_map:
            raise ArgumentError('Language {0} not defined.'.format(lang))
        switches = toDict(switches)
        # save it to a resultant dict
        to_generate[lang] = switches

    # sanity checks
    if not os.path.isfile(thrift_file):
        raise ArgumentError('Thrift file not found.')
    if not os.access(thrift_file, os.R_OK):
        raise ArgumentError('Cannot read thrift file.')
    if not os.path.isdir(opts.outputDir):
        raise ArgumentError('Output directory is not a directory.')
    if not os.access(opts.outputDir, os.W_OK | os.X_OK):
        raise ArgumentError('Output directory is not writeable.')

    return dict(
        to_generate=to_generate,
        options=opts,
        thrift_file=thrift_file,
    )


class Configuration(object):

    def __init__(self, opts):
        self._opts = opts
        if not opts.verbose:
            # kill the verbose function
            self.pverbose = self.pverbose_dummy
        # set to True to debug docstring parsing
        self.dump_docs = False

    # Nice little wrapper to the opts for easy access
    def __getattr__(self, key):
        return getattr(self._opts, key)

    def pverbose_dummy(self, msg):
        pass

    def pverbose(self, msg):
        sys.stderr.write(msg)

    def pwarning(self, level, msg):
        if self.warn < level:
            return
        print >>sys.stderr, msg

    def generate(self, program, languages):
        'Oooohh, recursively generate program, hot!!'

        from thrift_compiler.frontend import t_program
        assert isinstance(program, t_program)
        assert isinstance(languages, dict)

        if self.recurse:
            for inc in program.includes:
                # Propagate output path from parent to child programs
                inc.out_path = program.out_path
                self.generate(inc, languages)

        # Generate code

        self.pverbose("Program: {0}\n".format(program.path))

        if self.dump_docs:
            frontend.dump_docstrings(program)

        for language, flags in languages.iteritems():
            for flag in flags:
                if flag not in generator_registry.generator_factory_map[ \
                        language].supported_flags:
                    self.pwarning(1, "Language {0} doesn't recognize flag {1}"\
                            .format(language, flag))
            g = generator_registry.get_generator(program, language, flags)
            if g is None:
                self.pwarning(1, "Unable to get a generator for " \
                    "{0}:{1}".format(language, ','.join(flags)))
            # do it!
            g.generate_program()

    def process(self, params):
        def generate_wrapper(*args):
            self.generate(*args)

        frontend.process(params, generate_wrapper)


def main():
    # parse
    parser = init_parser()
    params = None
    try:
        params = parseParameters(parser, sys.argv[1:])
    except ArgumentError as e:
        print 'Argument Error:', e
        # print usage
        # parser.print_help()
        return

    # instantiate a Configuration that will hold the compilation flags
    conf = Configuration(params['options'])
    conf.process(params)

if __name__ == '__main__':
    main()
