#!/usr/bin/env python3

from setuptools import setup, Extension
from Cython.Build import cythonize
from Cython.Compiler import Options
import os

Options.fast_fail = True

common_libs = ['glog',
               'iobuf-cpp2',
               'thrift',
               'protocol',
               'gflags',
               'thriftcpp2',
               'async',
               'transport',
               'folly_pic',
               'snappy',
               'ssl',
               'lz4',
               'zstd',
               'thrift-core',
               'bz2',
               'event',
               'thriftprotocol',
               'boost_context',
               'iberty',
               'concurrency',
               'double-conversion',
               'crypto']

extensions = [
    Extension("iobuf.types",
              sources=['iobuf/types.pyx'],
              libraries=common_libs
             ),
    Extension("test.iobuf_helper",
              sources=['test/iobuf_helper.pyx'],
              libraries=common_libs
             ),
]

setup(name="test",
      version='0.0.1',
      zip_safe=False,
      ext_modules = cythonize(extensions,
                              # FIXME: I cannot seem to get lib thift-py3 build
                              # path passed from CMake as a parameter
                              include_path=["../../cybld/"],
                              compiler_directives={'language_level': 3,}),
)
