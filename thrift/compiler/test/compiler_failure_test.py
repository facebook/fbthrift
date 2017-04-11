from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import os
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest


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

exe = os.path.join(os.getcwd(), sys.argv[0])
thrift = ascend_find_exe(exe, 'thrift')


def read_file(path):
    with open(path, 'r') as f:
        return f.read()


def write_file(path, content):
    with open(path, 'w') as f:
        f.write(content)


class CompilerFailureTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.tmp)
        self.maxDiff = None

    def run_thrift(self, *args):
        argsx = [thrift, "--gen", "cpp2"]
        argsx.extend(args)
        pipe = subprocess.PIPE
        p = subprocess.Popen(argsx, stdout=pipe, stderr=pipe)
        out, err = p.communicate()
        out = out.decode(sys.getdefaultencoding())
        err = err.decode(sys.getdefaultencoding())
        err = err.replace("{}/".format(self.tmp), "")
        return p.returncode, out, err

    def test_duplicate_method_name(self):
        # tests overriding a method of the same service
        write_file("foo.thrift", textwrap.dedent("""\
            service MyS {
                void meh(),
                void meh(),
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Function MyS.meh redefines MyS.meh\n",
        )

    def test_duplicate_method_name_base_base(self):
        # tests overriding a method of the parent and the grandparent services
        write_file("foo.thrift", textwrap.dedent("""\
            service MySBB {
                void lol(),
            }
        """))
        write_file("bar.thrift", textwrap.dedent("""\
            include "foo.thrift"
            service MySB extends foo.MySBB {
                void meh(),
            }
        """))
        write_file("baz.thrift", textwrap.dedent("""\
            include "bar.thrift"
            service MyS extends bar.MySB {
                void lol(),
                void meh(),
            }
        """))
        ret, out, err = self.run_thrift("baz.thrift")
        self.assertEqual(ret, 1)
        self.assertEqual(
            err,
            "[FAILURE:baz.thrift:3] Function MyS.lol redefines service "
            "foo.MySBB.lol\n"
            "[FAILURE:baz.thrift:4] Function MyS.meh redefines service "
            "bar.MySB.meh\n",
        )

    def test_duplicate_enum_value_name(self):
        write_file("foo.thrift", textwrap.dedent("""\
            enum Foo {
                Bar = 1,
                Bar = 2,
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] Redefinition of value Bar in enum Foo\n"
        )

    def test_duplicate_enum_value(self):
        write_file("foo.thrift", textwrap.dedent("""\
            enum Foo {
                Bar = 1,
                Baz = 1,
            }
        """))
        ret, out, err = self.run_thrift("foo.thrift")
        self.assertEqual(
            err,
            "[FAILURE:foo.thrift:3] "
            "Duplicate value Baz=1 with value Bar in enum Foo. "
            "Add thrift.duplicate_values annotation to enum to suppress this "
            "error\n"
        )
