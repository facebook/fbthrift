#
# Common thrift code generation rules
#
THRIFT = $(top_builddir)/compiler/thrift1

gen-cpp/reflection_constants.cpp: reflection.thrift
	$(THRIFT) --gen cpp:templates,cob_style -r $<

gen-cpp/frozen_constants.cpp: frozen.thrift
	$(THRIFT) --gen cpp:templates -r $<

all: gen-cpp/reflection_constants.cpp gen-cpp/frozen_constants.cpp

check: all

clean:
	rm -rf gen-cpp
