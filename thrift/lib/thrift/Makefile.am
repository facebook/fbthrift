#
# Common thrift code generation rules
#
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

all: gen-cpp/reflection_constants.cpp gen-cpp/frozen_constants.cpp

check: all

clean:
	rm -rf gen-cpp
