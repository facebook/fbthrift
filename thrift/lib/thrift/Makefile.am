#
# Common thrift code generation rules
#
THRIFT = $(top_builddir)/compiler/thrift1

gen-cpp/reflection_constants.cpp: reflection.thrift
	$(THRIFT) --gen cpp:templates,cob_style -r $<
all: gen-cpp/reflection_constants.cpp

check: all

clean:
	rm -rf gen-cpp
