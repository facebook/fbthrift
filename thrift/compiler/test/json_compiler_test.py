#!/usr/bin/env/ python3

import json
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


def check_run_thrift(*args):
    argsx = [thrift, "--gen", "json"] + list(args)
    pipe = subprocess.PIPE
    return subprocess.check_call(argsx, stdout=pipe, stderr=pipe)


def gen(path, content):
    base, ext = os.path.splitext(path)
    if ext != ".thrift":
        raise Exception("bad path: {}".format(path))
    write_file(path, content)
    check_run_thrift("foo.thrift")
    return json.loads(read_file(os.path.join("gen-json", base + ".json")))


class JsonCompilerTest(unittest.TestCase):

    def setUp(self):
        tmp = tempfile.mkdtemp()
        self.addCleanup(shutil.rmtree, tmp, True)
        self.tmp = tmp
        self.addCleanup(os.chdir, os.getcwd())
        os.chdir(self.tmp)
        self.maxDiff = None

    def test_empty(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
        """))
        self.assertEqual(obj, {
            "thrift_module": "foo",
        })

    def test_one_struct_empty(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataHolder {
            }
        """))
        self.assertEqual(obj, {
            "thrift_module": "foo",
            "structs": {
                "DataHolder": {
                    "fields": {
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
            },
        })

    def test_one_struct_one_field_dep_struct(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataItem {
            }
            struct DataHolder {
                1: DataItem item,
            }
        """))
        self.assertEqual(obj, {
            "thrift_module": "foo",
            "structs": {
                "DataItem": {
                    "fields": {
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
                "DataHolder": {
                    "fields": {
                        "item": {
                            "required": True,
                            "type_enum": "STRUCT",
                            "spec_args": "DataItem",
                        },
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 3,
                },
            },
        })

    def test_one_struct_one_field_dep_coll_struct(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataItem {
            }
            struct DataHolder {
                1: map<string, DataItem> item,
            }
        """))
        self.assertEqual(obj, {
            "thrift_module": "foo",
            "structs": {
                "DataItem": {
                    "fields": {
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
                "DataHolder": {
                    "fields": {
                        "item": {
                            "required": True,
                            "type_enum": "MAP",
                            "spec_args": {
                                "key_type": {
                                    "type_enum": "STRING",
                                    "spec_args": None,
                                },
                                "val_type": {
                                    "type_enum": "STRUCT",
                                    "spec_args": "DataItem",
                                },
                            },
                        },
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 3,
                },
            },
        })

    def test_one_struct_one_field_dep_struct_bug_out_of_order(self):
        # when a field appeared with an unknown type, it is listed as typedef
        # in spec_args, even if later that type was defined as struct
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataHolder {
                1: DataItem item,
            }
            struct DataItem {
            }
        """))
        self.assertEqual(obj, {
            "thrift_module": "foo",
            "structs": {
                "DataItem": {
                    "fields": {
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 4,
                },
                "DataHolder": {
                    "fields": {
                        "item": {
                            "required": True,
                            "type_enum": "TYPEDEF",
                            "spec_args": "DataItem",
                        },
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
            },
        })
