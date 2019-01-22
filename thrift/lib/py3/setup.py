#!/usr/bin/env python3

from setuptools import setup, Extension
from Cython.Build import cythonize
from Cython.Compiler import Options
import os

Options.fast_fail = True

common_libs=['folly_pic',
             'glog',
             'thrift',
             'protocol',
             'gflags',
             'thriftcpp2',
             'async',
             'transport',
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
   Extension("folly.iobuf",
             sources=['folly/iobuf.pyx'],
             libraries=common_libs),
   Extension("thrift.py3.common",
             sources=['thrift/py3/common.pyx'],
             libraries=common_libs),
   Extension("thrift.py3.types",
             sources=['thrift/py3/types.pyx'],
             libraries=common_libs),
   Extension("thrift.py3.exceptions",
             sources=['thrift/py3/exceptions.pyx'],
             libraries=common_libs),
   Extension("thrift.py3.serializer",
             sources=['thrift/py3/serializer.pyx'],
             libraries=common_libs),
]

setup(name="thrift",
      version='0.0.1',
      packages=["thrift.py3"],
      package_data={ "py3": ['iobuf.pxd'] },
      zip_safe=False,
      ext_modules = cythonize(extensions,
                              compiler_directives={'language_level': 3,}))

