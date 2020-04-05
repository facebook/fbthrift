#!/usr/bin/env python3
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

from __future__ import absolute_import, division, print_function, unicode_literals

import argparse
import os.path
import functools
from random import choice, randint, random, seed


COMPARISON = ["<", ">", "<=", ">=", "==", "!="]
METHODS = [
    "operator=",
    "operator*",
    "operator bool",
    "assign",
    "emplace",
    "has_value",
    "reset",
    "value",
    "value_or",
]

DISCARD_RETURN = ["assign", "reset", "operator="]
HAS_ARG = ["assign", "value_or", "emplace", "operator="]
MAY_THROW = ["operator*", "value"]


def gen_define_variables(out):
    # Defining 2 variables since we want to test comparison operators
    out(f"DeprecatedOptionalField<std::string> a1, a2;")

    # control group
    out(f"Optional<std::string> b1, b2;")


def gen_test_method(out):
    # test comparison
    if random() < 0.3:
        op = choice(COMPARISON)
        out(f"EXPECT_EQ(a1 {op} a2, b1 {op} b2);")

    # test operator->
    if random() < 0.1:
        out(
            """
            if (b1) {
              EXPECT_EQ(a1->size(), b1->size());
            } else {
              EXPECT_THROW(a1->size(), folly::OptionalEmptyException);
            }
        """
        )

    # randomly test method
    var = choice([1, 2])
    method = choice(METHODS)
    call = f'("{randint(0, 2000000)}")' if method in HAS_ARG else "()"
    expr1 = f"a{var}.{method}{call}"
    expr2 = f"b{var}.{method}{call}"

    if method in DISCARD_RETURN:
        expr = f"{expr1}; {expr2};"
    else:
        expr = f"EXPECT_EQ({expr1}, {expr2});"

    if method not in MAY_THROW:
        out(expr)
    else:
        out(
            f"""
            if (b{var}) {{
              {expr}
            }} else {{
              EXPECT_THROW({expr1}, folly::OptionalEmptyException);
            }}
        """
        )


def gen_cpp_file(install_dir, idx):
    with open(os.path.join(install_dir, f"Test{idx}.cpp"), "w") as file:
        seed(idx)  # Generate deterministic code so that buck can cache
        out = functools.partial(print, file=file)
        out("#include <folly/Optional.h>")
        out("#include <gtest/gtest.h>")
        out("#include <thrift/lib/cpp2/OptionalField.h>")
        out("#include <string>")
        out("using apache::thrift::DeprecatedOptionalField;")
        out("using folly::Optional;")
        out(f"TEST(Test, Num{idx}) {{")
        gen_define_variables(out)
        for _ in range(1000):
            gen_test_method(out)
        out("}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--install_dir", type=str)
    parser.add_argument("--test_num", type=int, default=1)
    args = parser.parse_args()
    for i in range(args.test_num):
        gen_cpp_file(args.install_dir, i)
