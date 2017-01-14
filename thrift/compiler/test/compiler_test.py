from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import pkg_resources
import re
import unittest
import shlex
import shutil
import subprocess
import sys
import tempfile
import traceback

def ascend_find_exe(path, target):
    if not os.path.isdir(path):
        path = os.path.dirname(path)
    while True:
        test = os.path.join(path, target)
        if os.access(test, os.X_OK):
            return test
        parent = os.path.dirname(path)
        if os.path.samefile(parent, path):
            return None
        path = parent

def ascend_find_dir(path, target):
    if not os.path.isdir(path):
        path = os.path.dirname(path)
    while True:
        test = os.path.join(path, target)
        if os.path.isdir(test):
            return test
        parent = os.path.dirname(path)
        if os.path.samefile(parent, path):
            return None
        path = parent

def read_file(path):
    with open(path, 'r') as f:
        return f.read()

def write_file(path, content):
    with open(path, 'w') as f:
        f.write(content)

def read_resource(path):
    return pkg_resources.resource_string(__name__, path)

def read_lines(path):
    with open(path, 'r') as f:
        return f.readlines()

def mkdir_p(path, mode):
    if not os.path.isdir(path):
        os.makedirs(path, mode)

def parse_manifest(raw):
    manifest = {}
    for line in raw.splitlines():
        fixture, filename = line.split('/', 1)
        if fixture not in manifest:
            manifest[fixture] = []
        manifest[fixture].append(filename)
    return manifest

thrift = os.getenv('THRIFT_COMPILER_BIN')
fixtureDir = 'fixtures'

exe = os.path.join(os.getcwd(), sys.argv[0])
templateDir = ascend_find_dir(exe, 'thrift/compiler/generate/templates')

manifest = parse_manifest(read_resource(os.path.join(fixtureDir, 'MANIFEST')))
fixtureNames = manifest.keys()

class CompilerTest(unittest.TestCase):

    MSG = " ".join([
        "One or more fixtures are out of sync with the thrift compiler.",
        "To sync them, build thrift and then run:",
        "`thrift/compiler/test/build_fixtures <build-dir>`, where",
        "<build-dir> is a path where the program `thrift/compiler/thrift`",
        "may be found.",
    ])

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.maxDiff = None

    def runTest(self, name):
        fixtureChildDir = os.path.join(fixtureDir, name)
        cmdc = read_resource(os.path.join(fixtureChildDir, 'cmd'))
        write_file(os.path.join(self.tmp, "cmd"), cmdc)
        for fn in manifest[name]:
            if fn.startswith('src/'):
                out = os.path.join(self.tmp, fn)
                mkdir_p(os.path.dirname(out), 0o700)
                srcc = read_resource(os.path.join(fixtureChildDir, fn))
                write_file(out, srcc)
        cmds = read_lines(os.path.join(self.tmp, 'cmd'))
        for cmd in cmds:
            args = shlex.split(cmd.strip())
            base_args = [thrift, '-r', '--templates', templateDir, '--gen']
            if "cpp" in args[0]:
                path = os.path.join("thrift/compiler/test/fixtures", name)
                extra = "include_prefix=" + path
                join = "," if ":" in args[0] else ":"
                args[0] = args[0] + join + extra
            if ("cpp2" in args[0] and "mstch" not in args[0]) or \
               ("schema" in args[0]):
                # do not recurse in py generators. This is a hack because there
                # is a bug in the python generator that appears hard to fix.
                # This is a workaround. In future, we will use mstch, and the
                # mstch generator does not have this issue.
                base_args.remove('-r')

            subprocess.check_call(
                base_args + args,
                cwd=self.tmp,
                close_fds=True,
            )
        gens = subprocess.check_output(
            ["find", ".", "-type", "f"],
            cwd=self.tmp,
            close_fds=True,
        ).splitlines()
        gens = [gen.split('/', 1)[1] for gen in gens]

        # Strip \n
        cmds = [s.strip() for s in cmds]
        # Make list into a single string
        cmd_string = ''.join(cmds)
        # Match only those who have cpp2 and mstch_cpp2
        if (re.search('(^|[^#_])cpp2', cmd_string) is not None and
                re.search('(^|[^#])mstch_cpp2', cmd_string) is not None):
            self.compare_cpp2_and_mstchcpp2(gens, name, fixtureChildDir)

    # Compare that mstch_cpp2 and cpp2 generate equal files with equal content
    def compare_cpp2_and_mstchcpp2(self, gens, name, fixtureChildDir):
        try:
            cpp2_gens = [g for g in gens if "gen-cpp2" in g]
            cpp2_gens_names = [re.sub(".*\/", "", e) for e in cpp2_gens]
            mstch_cpp2_gens = [g for g in gens if "gen-mstch_cpp2" in g]
            mstch_cpp2_gens_names = [re.sub(".*\/", "", e)
                                     for e in mstch_cpp2_gens]
            # Compare that both generate the same named files
            self.assertEqual(sorted(cpp2_gens_names),
                             sorted(mstch_cpp2_gens_names))
            for gen in cpp2_gens_names:
                genc = read_file(os.path.join(self.tmp, 'gen-cpp2/', gen))
                genm = read_file(os.path.join(self.tmp, 'gen-mstch_cpp2/', gen))
                if genc != genm:
                    print(os.path.join(name, gen), file=sys.stderr)
                # Compare that the file contents are equal
                self.assertMultiLineEqual(genc, genm)
        except Exception as e:
            print(self.MSG, file=sys.stderr)
            traceback.print_exc(file=sys.stderr)
            raise e

def add_fixture(klazz, name):
    def test_method(self):
        self.runTest(name)
    test_method.__name__ = str('test_' + re.sub('[^0-9a-zA-Z]', '_', name))
    setattr(klazz, test_method.__name__, test_method)

for name in fixtureNames:
    add_fixture(CompilerTest, name)
