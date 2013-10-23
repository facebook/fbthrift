#!/usr/local/bin/python

import os
import shutil
import string
import sys

if __name__ == "__main__":
    dir = "."
    for arg in sys.argv:
        pair = string.split(arg, '=')
        if len(pair) == 2 and pair[0] == "--install_dir":
            dir = pair[1]

    if not os.path.exists(dir):
        os.makedirs(dir)

    shutil.copy("EnumTest.thrift", dir + "/EnumTestStrict.thrift")
