from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import re
import unittest
import shlex
import shutil
import subprocess
import sys
import tempfile

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

def read_file(path):
    with open(path, 'r') as f:
        return f.read()

def read_lines(path):
    with open(path, 'r') as f:
        return f.readlines()

def mkdir_p(path, mode):
    try:
        os.makedirs(path, mode)
    except IOError:
        pass

def parse_manifest(raw):
    manifest = {}
    for line in raw.splitlines():
        fixture, filename = line.split('/', 1)
        if fixture not in manifest:
            manifest[fixture] = []
        manifest[fixture].append(filename)
    return manifest

exe = os.path.join(os.getcwd(), sys.argv[0])
thrift = ascend_find_exe(exe, 'thrift')
fixtureDir = os.path.join(os.path.dirname(exe), 'fixtures')
manifest = parse_manifest(read_file(os.path.join(fixtureDir, 'MANIFEST')))
fixtureNames = manifest.keys()

class MyTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.maxDiff = None

    def runTest(self, name):
        fixtureChildDir = os.path.join(fixtureDir, name)
        shutil.copy2(os.path.join(fixtureChildDir, 'cmd'), self.tmp)
        for fn in manifest[name]:
            if fn.startswith('src/'):
                dn = os.path.dirname(os.path.join(self.tmp, fn))
                mkdir_p(dn, 0o700)
                shutil.copy2(os.path.join(fixtureChildDir, fn), dn)
        cmds = read_lines(os.path.join(self.tmp, 'cmd'))
        for cmd in cmds:
            subprocess.check_call(
                [thrift, '--gen'] + shlex.split(cmd.strip()),
                cwd=self.tmp,
                close_fds=True,
            )
        gens = subprocess.check_output(
            ["find", ".", "-type", "f"],
            cwd=self.tmp,
            close_fds=True,
        ).splitlines()
        print(gens, file=sys.stderr)
        gens = [gen.split('/', 1)[1] for gen in gens]
        self.assertEqual(sorted(gens), sorted(manifest[name]))
        for gen in gens:
            genc = read_file(os.path.join(self.tmp, gen))
            fixc = read_file(os.path.join(fixtureChildDir, gen))
            self.assertMultiLineEqual(genc, fixc)

def add_fixture(klazz, name):
    def test_method(self):
        self.runTest(name)
    test_method.__name__ = str('test_' + re.sub('[^0-9a-zA-Z]', '_', name))
    setattr(klazz, test_method.__name__, test_method)

for name in fixtureNames:
    add_fixture(MyTest, name)
