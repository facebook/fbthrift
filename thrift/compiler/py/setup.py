#!/usr/bin/env python

import sys
import shutil
try:
    from setuptools import setup, Extension
except:
    from distutils.core import setup, Extension, Command

def run_setup():
    shutil.copy('.libs/libfrontend.so.0.0.0', 'frontend.so')
    
    setup(name = 'thrift-py',
        version = '0.9.0',
        description = 'Thrift python compiler',
        author = ['Thrift Developers'],
        author_email = ['dev@thrift.apache.org'],
        url = 'http://thrift.apache.org',
        license = 'Apache License 2.0',
        packages = [
            'thrift',
            'thrift.compiler',
            'thrift.compiler.generate',
        ],
        package_dir = {'thrift.compiler' : '.',
                       'thrift' : '..'},
        package_data = {'thrift.compiler':['frontend.so']},
        classifiers = [
            'Development Status :: 5 - Production/Stable',
            'Environment :: Console',
            'Intended Audience :: Developers',
            'Programming Language :: Python',
            'Programming Language :: Python :: 2',
            'Topic :: Software Development :: Libraries',
            'Topic :: System :: Networking'
        ],
    )

run_setup()
