#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
# Contains some contributions under the Thrift Software License.
# Please see doc/old-thrift-license.txt in the Thrift distribution for
# details.

# We don't actually want "make install" to try to install our Python module in
# /usr/local/lib. But we can't use noinst_, as that's special-cased and libtool
# doesn't actually build the shared object (it builds a .a made up of PIC
# object files instead). FUUUUUUUUUUUU.
#
# So we pretend it's a lib library, but *override* the install-exec target
# so it doesn't actually install it. This will be problematic if you add
# other libraries in this directory, so don't.
lib_LTLIBRARIES = frontend.la

frontend_la_SOURCES = compiler.cc
frontend_la_CPPFLAGS = $(AM_CPPFLAGS) $(PYTHON_CPPFLAGS)
frontend_la_LDFLAGS = \
        -module -avoid-version -no-undefined \
        -l$(BOOST_PYTHON_LIB) $(PYTHON_LDFLAGS) -lfolly
frontend_la_LIBADD = ../libparse.la ../libthriftcompilerbase.la

all-local: frontend.la
	$(PYTHON) setup.py build
	mkdir -p $(PY_LOCAL_PATH)
	PYTHONPATH=$(PY_LOCAL_PATH) $(PYTHON) setup.py install --home=$(PY_LOCAL_ROOT)

# Note, not install-exec-hook, see the comment above
install-exec-am:
	test -z "$(PY_INSTALL_HOME)" || mkdir -p $(PY_INSTALL_HOME)/lib/python
	test -z "$(PY_CUSTOM_SITE_PKGS)" || mkdir -p $(PY_CUSTOM_SITE_PKGS)
	$(PY_RUN_ENV) $(PYTHON) setup.py install --root=$(DESTDIR) $(PY_INSTALL_ARGS)

clean-local:
	rm -f frontend.so
	-$(PYTHON) setup.py clean --all
