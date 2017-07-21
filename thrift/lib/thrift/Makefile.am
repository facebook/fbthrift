#
# Common thrift code generation rules
#

noinst_LIBRARIES = libfrozenschema.a

THRIFT = $(top_builddir)/compiler/thrift1

gen-cpp/reflection_constants.cpp: reflection.thrift
	$(THRIFT) --gen cpp:templates,cob_style -r $<

gen-cpp/reflection_constants.h: gen-cpp/reflection_constants.cpp
gen-cpp/reflection_types.h: gen-cpp/reflection_constants.h
gen-cpp/reflection_reflection.h: gen-cpp/reflection_constants.cpp
gen-cpp/reflection_types.tcc: gen-cpp/reflection_constants.h

gen-cpp/frozen_constants.cpp: frozen.thrift
	$(THRIFT) --gen cpp:templates -r $<

gen-cpp/frozen_constants.h: gen-cpp/frozen_constants.cpp
gen-cpp/frozen_types.h: gen-cpp/frozen_constants.h
gen-cpp/frozen_reflection.h: gen-cpp/frozen_constants.cpp
gen-cpp/frozen_types.tcc: gen-cpp/frozen_constants.h

include_reflectiondir = $(includedir)/thrift/lib/thrift/gen-cpp

include_reflection_HEADERS = \
	gen-cpp/reflection_types.h \
	gen-cpp/reflection_types.tcc \
	gen-cpp/reflection_constants.h \
	gen-cpp/reflection_reflection.h \
	gen-cpp/frozen_types.h \
	gen-cpp/frozen_types.tcc \
	gen-cpp/frozen_constants.h \
	gen-cpp/frozen_reflection.h

BUILT_SOURCES = \
	gen-cpp2/frozen_constants.cpp \
	gen-cpp2/frozen_data.cpp \
	gen-cpp2/frozen_types.cpp

gen-cpp2/frozen_data.h gen-cpp2/frozen_types.h: frozen.thrift
	PYTHONPATH=$(PY_LOCAL_PATH) $(PYTHON) -mthrift_compiler.main --gen cpp2 $<

gen-cpp2/frozen_constants.cpp: gen-cpp2/frozen_types.h

gen-cpp2/frozen_data.cpp: gen-cpp2/frozen_data.h gen-cpp2/frozen_types.h

gen-cpp2/frozen_types.cpp: gen-cpp2/frozen_data.h gen-cpp2/frozen_types.h

libfrozenschema_a_SOURCES = \
	gen-cpp2/frozen_constants.cpp \
	gen-cpp2/frozen_data.cpp \
	gen-cpp2/frozen_types.cpp

libfrozenschema_a_CPPFLAGS = -fPIC $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp

all: gen-cpp/reflection_constants.cpp gen-cpp/frozen_constants.cpp

check: all

clean:
	rm -rf gen-cpp
	rm -rf gen-cpp2
