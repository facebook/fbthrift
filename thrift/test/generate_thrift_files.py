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

import argparse
import doctest
import os
import shutil
from subprocess import check_output
from typing import Dict, List


HEADER = f"""# {'@' + 'generated'}
# To regenerate `fbcode/thrift/test/gen_if`, invoke
#   `buck run //thrift/test:generate_thrift_files`
"""

THRIFT_NAMESPACE = """
namespace cpp2 apache.thrift.test
"""

DEFAULT_INSTALL_DIR = "thrift/test/gen_if"

FIELD_COUNT = 2  # Number of fields per structs

TARGETS = """\
thrift_library(
    name = "all",
    languages = ["cpp2"],
    thrift_cpp2_options = ['json', 'reflection', 'visitation'],
    thrift_srcs = {f: None for f in glob(['*.thrift'])},
)
"""


def format_dict(
    d: Dict[str, str], key_format: str, value_format: str
) -> Dict[str, str]:
    """ Format key/value of dict
    >>> result = format_dict({"foo_k": "foo_v", "bar_k": "bar_v"}, 'prefix_{}', "{}_suffix")
    >>> result == {'prefix_foo_k': 'foo_v_suffix', 'prefix_bar_k': 'bar_v_suffix'}
    True
    """
    return {key_format.format(k): value_format.format(d[k]) for k in d}


def generate_names_to_types() -> Dict[str, str]:
    """ Generate display name to thrift type mapping. Display name will be used in file name, rule name, etc """
    ret = {"i64": "i64", "string": "string"}
    ret.update(format_dict(ret, "set_{}", "set<{}>"))
    ret.update(format_dict(ret, "map_string_{}", "map<string, {}>"))
    ret.update(
        **format_dict(ret, "optional_{}", "optional {}"),
        **format_dict(ret, "required_{}", "required {}"),
    )
    ret.update(format_dict(ret, "{}_cpp_ref", "{} (cpp.ref = 'true')"))
    return ret


def generate_struct(name: str, types: List[str]) -> str:
    """ Generate thrift struct from types
    >>> print(generate_struct("Foo", ["i64", "optional string", "set<i32> (cpp.ref = 'true')"]))
    struct Foo {
      1: i64 field_1;
      2: optional string field_2;
      3: set<i32> (cpp.ref = 'true') field_3;
    }
    """
    lines = ["struct {} {{".format(name)]
    for i, t in enumerate(types):
        lines.append("  {0}: {1} field_{0};".format(i + 1, t))
    lines.append("}")
    return "\n".join(lines)


def gen_files(install_dir: str):
    if os.path.exists(install_dir):
        shutil.rmtree(install_dir)
    os.makedirs(install_dir)

    with open(os.path.join(install_dir, "TARGETS"), "w") as file:
        print(HEADER, file=file)
        print(TARGETS, file=file)

        structs = []
        for name, type in generate_names_to_types().items():
            name = "struct_" + name
            structs.append(name)
            with open(os.path.join(install_dir, f"{name}.thrift"), "w") as file:
                print(HEADER, file=file)
                print(THRIFT_NAMESPACE, file=file)
                print(generate_struct(name, [type] * FIELD_COUNT), file=file)

        # Generate thrift files that contains all generated structures
    with open(os.path.join(install_dir, "struct_all.thrift"), "w") as file:
        print(HEADER, file=file)
        for i in structs:
            print(f'include "{i}.thrift"', file=file)
        print(THRIFT_NAMESPACE, file=file)
        print(generate_struct("struct_all", [f"{x}.{x}" for x in structs]), file=file)


if __name__ == "__main__":
    doctest.testmod()
    os.chdir(check_output(["buck", "root"]).strip())
    parser = argparse.ArgumentParser()
    parser.add_argument("--install_dir", default=DEFAULT_INSTALL_DIR)
    args = parser.parse_args()
    gen_files(args.install_dir)
    print("Generated thrift files under", os.path.realpath(args.install_dir))
