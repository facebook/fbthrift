#!/usr/bin/env/ python3
# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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


def check_run_thrift(annotate, *args):
    gen_str = "json"
    if annotate:
        gen_str = "json:annotate"
    argsx = [thrift, "--gen", gen_str] + list(args)
    pipe = subprocess.PIPE
    return subprocess.check_call(argsx, stdout=pipe, stderr=pipe)


def gen(path, content, annotate=False):
    base, ext = os.path.splitext(path)
    if ext != ".thrift":
        raise Exception("bad path: {}".format(path))
    write_file(path, content)
    check_run_thrift(annotate, "foo.thrift")
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
            "__fbthrift": {"@" + "generated": 0},
            "thrift_module": "foo",
        })

    def test_one_struct_empty(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataHolder {
            }
        """))
        self.assertEqual(obj, {
            "__fbthrift": {"@" + "generated": 0},
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
            "__fbthrift": {"@" + "generated": 0},
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
            "__fbthrift": {"@" + "generated": 0},
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
            "__fbthrift": {"@" + "generated": 0},
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
                            "type_enum": "STRUCT",
                            "spec_args": "DataItem",
                        },
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
            },
        })

    def test_struct_field_annotate(self):
        obj = gen("foo.thrift", textwrap.dedent("""\
            struct DataHolder {
              1: string data (foo="bar", ignore)
            }
        """), annotate=True)
        self.assertEqual(obj, {
            "__fbthrift": {"@" + "generated": 0},
            "thrift_module": "foo",
            "structs": {
                "DataHolder": {
                    "fields": {
                        "data": {
                            "required": True,
                            "type_enum": "STRING",
                            "spec_args": None,
                            "annotations": {
                                "foo": "bar",
                                "ignore": "1",
                            },
                        },
                    },
                    "is_exception": False,
                    "is_union": False,
                    "lineno": 1,
                },
            },
        })
